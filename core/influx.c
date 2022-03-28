
/*
Copyright (c) 2021, Stephen P. Shoecraft
All rights reserved.

This source code is licensed under the BSD-style license found in the
LICENSE file in the root directory of this source tree.
*/

#define DEBUG_INFLUX 1
#define dlevel 4

#ifdef DEBUG
#undef DEBUG
#endif
#define DEBUG DEBUG_INFLUX
#include "debug.h"

#include "common.h"
#include "influx.h"
#include <curl/curl.h>

#define SESSION_ID_SIZE 128

struct influx_session {
	influx_config_t config;
	int errcode;
	char errmsg[128];
	CURL *curl;
	char version[8];
	char session_id[SESSION_ID_SIZE];
	char *buffer;                   /* Read buffer */
	int bufsize;                    /* Buffer size */
	int bufidx;                     /* Current write pos */
	json_value_t *data;
	list results;
	jsval results_val;
	char *login_fields;
	char *read_fields;
	char *config_fields;
	bool verbose;
	bool convdt;
};
typedef struct influx_session influx_session_t;

int influx_parse_config(influx_config_t *conf, char *influx_info) {
	dprintf(dlevel,"info: %s\n", influx_info);
	strncpy(conf->endpoint,strele(0,",",influx_info),sizeof(conf->endpoint)-1);
	strncpy(conf->database,strele(1,",",influx_info),sizeof(conf->database)-1);
	strncpy(conf->username,strele(2,",",influx_info),sizeof(conf->username)-1);
	strncpy(conf->password,strele(3,",",influx_info),sizeof(conf->password)-1);
	dprintf(dlevel,"endpoint: %s, database: %s, user: %s, pass: %s\n", conf->endpoint, conf->database, conf->username, conf->password);
	return 0;
}

static size_t getheader(void *ptr, size_t size, size_t nmemb, void *ctx) {
	influx_session_t *s = ctx;
	char *version_line = "X-Influxdb-Version";
	char line[1024];
	int bytes = size*nmemb;

	memcpy(line,ptr,bytes);
	line[bytes] = 0;
	trim(line);
//	dprintf(dlevel,"header line(%d): %s\n", strlen(line), line);

	if (strncmp(line,version_line,strlen(version_line)) == 0)
		strncpy(s->version,strele(1,": ",line),sizeof(s->version)-1);
	return bytes;
}

static size_t getdata(void *ptr, size_t size, size_t nmemb, void *ctx) {
	influx_session_t *s = ctx;
	int bytes,newidx;

//	printf("data: %s\n",(char *)ptr);

	dprintf(dlevel,"bufsize: %d\n", s->bufsize);

	bytes = size*nmemb;
	dprintf(dlevel,"bytes: %d, bufidx: %d, bufsize: %d\n", bytes, s->bufidx, s->bufsize);
	newidx = s->bufidx + bytes;
	dprintf(dlevel,"newidx: %d\n", newidx);
	if (newidx > s->bufsize) {
		char *newbuf;

		dprintf(dlevel,"buffer: %p\n", s->buffer);
		newbuf = realloc(s->buffer,newidx);
		dprintf(dlevel,"newbuf: %p\n", newbuf);
		if (!newbuf) {
			log_syserror("influx_getdata: realloc(buffer,%d)",newidx);
			return 0;
		}
		s->buffer = newbuf;
		s->bufsize = newidx;
	}
	strcpy(s->buffer + s->bufidx,ptr);
	s->bufidx = newidx;

	return bytes;
}

influx_session_t *influx_new(influx_config_t *conf) {
	influx_session_t *s;

	s = calloc(sizeof(*s),1);
	if (!s) {
		log_syserror("influx_new: calloc");
		return 0;
	}
	s->config = *conf;
	dprintf(dlevel,"endpoint: %s, database: %s\n", conf->endpoint, conf->database);

	s->curl = curl_easy_init();
	if (!s->curl) {
		log_syserror("influx_new: curl_init");
		free(s);
		return 0;
	}
	s->results = list_create();

	/* init data buffer */
	s->buffer = malloc(1024);
	if (!s->buffer) {
		log_syserror("influx_new: malloc(%d)",1024);
		curl_easy_cleanup(s->curl);
		free(s);
		return 0;
	}
	s->bufsize = 1024;
	s->bufidx = 0;

	curl_easy_setopt(s->curl, CURLOPT_SSL_VERIFYPEER, 0L);
	curl_easy_setopt(s->curl, CURLOPT_SSL_VERIFYHOST, 0L);
	curl_easy_setopt(s->curl, CURLOPT_WRITEFUNCTION, getdata);
	curl_easy_setopt(s->curl, CURLOPT_WRITEDATA, s);

	if (strlen(conf->username)) {
		curl_easy_setopt(s->curl, CURLOPT_HTTPAUTH, CURLAUTH_BASIC);
		curl_easy_setopt(s->curl, CURLOPT_USERPWD, conf->username);
		curl_easy_setopt(s->curl, CURLOPT_HTTPAUTH, CURLAUTH_BASIC);
		curl_easy_setopt(s->curl, CURLOPT_USERPWD, conf->password);
	}

	/* Get the version */
//	influx_ping(s);
	influx_health(s);
	dprintf(dlevel,"version: %s\n", s->version);

	return s;
}

list influx_get_results(influx_session_t *s) {
	return s->results;
}

char *influx_mkendpoint(influx_session_t *s, char *action, char *query) {
	char *endpoint,*eq,*p;
	int size,db;

	db = 1;
	if (strcmp(action,"health")==0) db = 0;

	size = 0;
//	if (strncmp(s->endpoint,"http",4) != 0) p += sprintf(p,"https://");
	size += strlen(s->config.endpoint);
	size += strlen(action) + 1; /* + "/" */
	size += strlen(s->config.database) + 4;	/* + "?db=" */
	eq = (query ? curl_easy_escape(s->curl, query, strlen(query)) : 0);
	if (eq) size += strlen(eq) + 3; /* + "&q=" */
	dprintf(dlevel,"size: %d\n", size);
	endpoint = malloc(size+10);
	if (!endpoint) {
		log_syserror("influx_mkendpoint: malloc(%d)",size);
		return 0;
	}
	p = endpoint;
//	if (strncmp(s->endpoint,"http",4) != 0) p += sprintf(p,"https://");
	p += sprintf(p,"%s/%s",s->config.endpoint,action);
	if (db) p += sprintf(p,"?db=%s",s->config.database);
	if (eq) {
		p += sprintf(p,"&q=%s",eq);
		curl_free(eq);
	}
	dprintf(dlevel,"endpoint: %s\n", endpoint);
	return endpoint;
}

static void influx_destroy_results(influx_session_t *s) {
	s->results_val = 0;
	list_destroy(s->results);
	s->results = 0;
	return;
}

static void getval(influx_session_t *s, json_value_t *rv, influx_value_t *vp) {
	int type;
	register char *p;

	dprintf(dlevel,"vp: %p\n", vp);
	if (!vp) return;
	memset(vp,0,sizeof(*vp));

	type = json_value_get_type(rv);
	dprintf(dlevel,"type: %d (%s)\n", type, json_typestr(type));
	switch(type) {
	case JSON_TYPE_NULL:
		vp->type = DATA_TYPE_NULL;
		break;
	case JSON_TYPE_NUMBER:
	    {
		vp->type = DATA_TYPE_DOUBLE;
		vp->data = malloc(sizeof(double));
		if (!vp->data) {
			log_syserror("getval: malloc(%d)",sizeof(double));
			return;
		}
		*((double *)vp->data) = json_value_get_number(rv);
	    }
	    break;
	case JSON_TYPE_STRING:
		vp->type = DATA_TYPE_STRING;
		p = json_value_get_string(rv);
		vp->len = strlen(p);
		vp->data = malloc(vp->len+1);
		if (!vp->data) {
			log_syserror("_getval: malloc(%d)\n", vp->len+1);
			return;
		}
		strcpy((char *)vp->data,p);
		break;
	default:
		vp->type = DATA_TYPE_NULL;
		log_error("influx_getval: unhandled type: %d(%s)\n", type, json_typestr(type));
	}
}

static int getseries(influx_session_t *s, json_array_t *a) {
	int i,j,k,type;
	json_object_t *o;
	json_array_t *columns_array,*values_array;
	influx_series_t newseries;
	char *p;

	for(i=0; i < a->count; i++) {
		type = json_value_get_type(a->items[i]);
		dprintf(dlevel,"type[%d]: %s\n", i, json_typestr(type));
		if (json_value_get_type(a->items[i]) != JSON_TYPE_OBJECT) continue;
		o = json_value_get_object(a->items[i]);
		dprintf(dlevel,"count: %d\n", o->count);
		memset(&newseries,0,sizeof(newseries));
		newseries.convdt = s->convdt;
		columns_array = values_array = 0;
		for(j=0; j < o->count; j++) {
			dprintf(dlevel,"o->names[%d]: %s\n", j, o->names[j]);

			/* measurement name */
			if (strcmp(o->names[j],"name") == 0) {
				strncpy(newseries.name,o->names[j],sizeof(newseries.name));

			/* columns */
			} else if (strcmp(o->names[j],"columns") == 0 && json_value_get_type(o->values[j]) == JSON_TYPE_ARRAY) {
				columns_array = json_value_get_array(o->values[j]);

			/* values */
			} else if (strcmp(o->names[j],"values") == 0 && json_value_get_type(o->values[j]) == JSON_TYPE_ARRAY) {
				values_array = json_value_get_array(o->values[j]);
			}
		}
		if (!columns_array) {
			sprintf(s->errmsg,"no columns array");
			return 1;
		}
		if (!values_array) {
			sprintf(s->errmsg,"no values array");
			return 1;
		}

		/* Get the columns */
		newseries.column_count = columns_array->count;
		newseries.columns = calloc(newseries.column_count*sizeof(char *),1);
		if (!newseries.columns) {
			sprintf(s->errmsg,"malloc columns");
			return 1;
		}
		for(j=0; j < newseries.column_count; j++) {
			p = json_value_get_string(columns_array->items[j]);
			dprintf(dlevel,"column[%d]: %s\n", j, p);
			if (!p) {
				sprintf(s->errmsg,"error getting column name");
				return 1;
			}
			newseries.columns[j] = malloc(strlen(p)+1);
			if (!newseries.columns[j]) {
				sprintf(s->errmsg,"malloc column name");
				return 1;
			}
			strcpy(newseries.columns[j],p);
		}

		/* Get the values */
		/* Values are a mulidemential array of [values][column_count] */
		dprintf(dlevel,"value count: %d\n", values_array->count);
		newseries.value_count = values_array->count;
		dprintf(dlevel,"column_count: %d, value size: %d\n", newseries.column_count, sizeof(influx_value_t));
		newseries.values = malloc(newseries.value_count * sizeof(influx_value_t *));
		dprintf(dlevel,"values: %p\n", newseries.values);
		if (!newseries.values) {
			sprintf(s->errmsg,"malloc values");
			return 1;
		}
		for(j=0; j < newseries.value_count; j++) {
			if (json_value_get_type(values_array->items[j]) == JSON_TYPE_ARRAY) {
				json_array_t *aa = json_value_get_array(values_array->items[j]);
				dprintf(dlevel,"aa->count: %d\n", aa->count);
				if (aa->count != newseries.column_count) {
					sprintf(s->errmsg,"internal error: value item count(%d) != column count(%d)",
						(int)aa->count,newseries.column_count);
					return 1;
				}
				newseries.values[j] = malloc(aa->count * sizeof(influx_value_t));
				if (!newseries.values[j]) {
					sprintf(s->errmsg,"malloc values");
					return 1;
				}
				for(k=0; k < aa->count; k++) {
					dprintf(dlevel,"aa->item[k]: %s\n", json_typestr(json_value_get_type(aa->items[k])));
					dprintf(dlevel,"pos: %p\n",&newseries.values[j][k]);
					getval(s,aa->items[k],&newseries.values[j][k]);
				}
			}
		}
		list_add(s->results,&newseries,sizeof(newseries));
	}
	return 0;
}

static int getresults(influx_session_t *s, json_array_t *a) {
	int i,j,type;
	json_object_t *o;

	dprintf(dlevel,"count: %d\n", a->count);
	for(i=0; i < a->count; i++) {
		type = json_value_get_type(a->items[i]);
		dprintf(dlevel,"type[%d]: %s\n", i, json_typestr(type));
		if (json_value_get_type(a->items[i]) != JSON_TYPE_OBJECT) continue;
		o = json_value_get_object(a->items[i]);
		dprintf(dlevel,"count: %d\n", o->count);
		for(j=0; j < o->count; j++) {
			dprintf(dlevel,"o->names[%d]: %s\n", j, o->names[j]);
			if (strcmp(o->names[j],"series") == 0 && json_value_get_type(o->values[j]) == JSON_TYPE_ARRAY)
				getseries(s, json_value_get_array(o->values[j]));
		}
	}

	return 0;
}

int influx_request(influx_session_t *s, char *url, char *fields) {
	struct curl_slist *hs = curl_slist_append(0, "Content-Type: application/x-www-form-endpointencoded");
	CURLcode res;
	int rc,err;
	char *msg;

	dprintf(dlevel,"url: %s\n", url);

	influx_destroy_results(s);
	s->results = list_create();
	s->bufidx = 0;
	if (s->data) {
		json_destroy_value(s->data);
		s->data = 0;
	}

	/* Make the request */
	curl_easy_setopt(s->curl, CURLOPT_VERBOSE, s->verbose);
	curl_easy_setopt(s->curl, CURLOPT_URL, url);
	curl_easy_setopt(s->curl, CURLOPT_HTTPHEADER, hs);
	if (fields) curl_easy_setopt(s->curl, CURLOPT_POSTFIELDS, fields);
	else curl_easy_setopt(s->curl, CURLOPT_POST, 0L);
	res = curl_easy_perform(s->curl);
	if (res != CURLE_OK) {
		sprintf(s->errmsg,"influx_request failed: %s\n", curl_easy_strerror(res));
		return 1;
	}

	/* Get the response code */
	curl_easy_getinfo(s->curl, CURLINFO_RESPONSE_CODE, &rc);
	dprintf(dlevel,"rc: %d\n", rc);
	err = 1;
	switch(rc) {
	case 200:
		msg = "Success";
		err = 0;
		break;
	case 204:
		msg = "No content";
		err = 0;
		break;
	case 400:
		msg = "Bad Request";
		break;
	case 401:
		msg = "Unauthorized";
		break;
	case 404:
		msg = "Not found";
		break;
	case 413:
		msg = "Request Entity Too Large";
		break;
	case 422:
		msg = "Unprocessible entity";
		break;
	case 429:
		msg = "Too many requests";
		break;
	case 500:
		msg = "Internal Server Error";
		break;
	case 503:
		msg = "Service unavailable";
		break;
	default:
		dprintf(dlevel,"influx_request: unhandled rc: %d\n", rc);
		msg = "unknown response code";
		break;
	}
	if (msg) sprintf(s->errmsg,msg);

	/* Parse the buffer */
//	dprintf(dlevel,"buffer: %s\n", s->buffer);
	dprintf(dlevel,"bufidx: %d\n", s->bufidx);
	s->data = json_parse(s->buffer);
	dprintf(dlevel,"data: %p\n", s->data);
	/* May not be json data? */
	if (!s->data) return 0;

	/* If it's results, process it */
	dprintf(dlevel,"type: %s\n", json_typestr(json_value_get_type(s->data)));
	if (json_value_get_type(s->data) == JSON_TYPE_OBJECT) {
		json_object_t *o;

		o = json_value_get_object(s->data);
		if (o->count >= 1 && strcmp(o->names[0],"results") == 0 && json_value_get_type(o->values[0]) == JSON_TYPE_ARRAY)
			return getresults(s,json_value_get_array(o->values[0]));
	}

	return err;
}

static void display_results(influx_session_t *s) {
	influx_series_t *sp;
	influx_value_t *vp;
	int i,j;
	char value[258];

	dprintf(dlevel,"buffer: %s\n", s->buffer);
	dprintf(dlevel,"results count: %d\n", list_count(s->results));
	list_reset(s->results);
	while((sp = list_get_next(s->results)) != 0) {
		printf("sp->name: %s\n", sp->name);
		for(i=0; i < sp->column_count; i++) printf("column[%d]: %s\n", i, sp->columns[i]);
		for(i=0; i < sp->value_count; i++) {
			for(j=0; j < sp->column_count; j++) {
				vp = &sp->values[i][j];
				conv_type(DATA_TYPE_STRING,value,sizeof(value),vp->type,vp->data,vp->len);
				printf("value[%d][%d]: %s\n", i,j, value);
			}
		}
	}
}

int influx_health(influx_session_t *s) {
	char *endpoint;
	long rc;
	char *p;

	endpoint = influx_mkendpoint(s,"health",0);
	dprintf(dlevel,"endpoint: %p\n", endpoint);
	if (!endpoint) return 1;
	if (influx_request(s,endpoint,0)) {
		free(endpoint);
		return 1;
	}
	curl_easy_getinfo(s->curl, CURLINFO_RESPONSE_CODE, &rc);
	dprintf(dlevel,"rc: %d\n", rc);
	free(endpoint);
//	if (rc != 200) return(1);
	dprintf(dlevel,"data: %p\n", s->data);
	if (!s->data) {
		sprintf(s->errmsg,"influx_health: invalid data");
		return 1;
	}
	dprintf(dlevel,"type: %s\n", json_typestr(json_value_get_type(s->data)));
	if (json_value_get_type(s->data) != JSON_TYPE_OBJECT) return 1;
#if 0
	p = json_object_dotget_string(json_value_get_object(s->data),"status");
	if (!p) {
		sprintf(s->errmsg,"unable to get influxdb status", p);
		return 1;
	}
	if (strcmp(p,"pass") != 0) {
		p = json_object_dotget_string(json_value_get_object(s->data),"message");
		if (p) strncpy(s->errmsg,p,sizeof(s->errmsg);
		else sprintf(s->errmsg,"influxdb status is %s", p);
		return 1;
	}
#endif
	p = json_object_dotget_string(json_value_get_object(s->data),"version");
	dprintf(dlevel,"p: %s\n", p);
	if (!p) return 1;
	strncpy(s->version,p,sizeof(s->version)-1);
	return 0;
}

int influx_ping(influx_session_t *s) {
	char *endpoint;
	long rc;

	endpoint = influx_mkendpoint(s,"ping",0);
	dprintf(dlevel,"endpoint: %p\n", endpoint);
	if (!endpoint) return 1;
	curl_easy_setopt(s->curl, CURLOPT_HEADERFUNCTION, getheader);
	curl_easy_setopt(s->curl, CURLOPT_HEADERDATA, s);
	if (influx_request(s,endpoint,0)) {
		free(endpoint);
		return 1;
	}
	curl_easy_setopt(s->curl, CURLOPT_HEADERFUNCTION, 0);
	curl_easy_setopt(s->curl, CURLOPT_HEADERDATA, 0);
	curl_easy_getinfo(s->curl, CURLINFO_RESPONSE_CODE, &rc);
	dprintf(dlevel,"rc: %d\n", rc);
	free(endpoint);
	return (rc == 200 || rc == 204 ? 0 : 1);
}

int influx_query(influx_session_t *s, char *query) {
	char *endpoint;
	char *lcq;
	int r,post;

	lcq = stredit(query,"COMPRESS,LOWERCASE");
	dprintf(dlevel,"lcq: %s\n", lcq);
	if (strstr(lcq,"select into ")) post = 1;
	else if (strstr(lcq,"select ") || strstr(lcq,"show ")) post = 0;
	else post = 1;

	endpoint = influx_mkendpoint(s,"query",query);
	dprintf(dlevel,"endpoint: %p\n", endpoint);
	if (!endpoint) return 1;
	r = influx_request(s,endpoint,(post ? "" : 0));
	dprintf(dlevel,"r: %d\n", r);
	if (!r && 0) display_results(s);
	free(endpoint);
	return r;
}

int influx_write(influx_session_t *s, char *table, char *field, int type, void *ptr, int size) {
	return 1;
}

#if 0
/* ic_tags() argument is the measurement tags for influddb */
/* example: "host=vm1234"   note:the comma & hostname of the virtual machine sending the data */
/* complex: "host=lpar42,serialnum=987654,arch=power9" note:the comma separated list */
void ic_tags(char *t)	
{
    dprintf(dlevel,"t: %s\n", t);
    if( influx_tags == (char *) 0) {
        if( (influx_tags = (char *)malloc(MEGABYTE)) == (char *)-1)
           error("failed to malloc() tags buffer");
    }

    strncpy(influx_tags,t,256);
}

void ic_measure(char *section)
{
    ic_check( strlen(section) + strlen(influx_tags) + 3);

    output_char += sprintf(&output[output_char], "%s,%s ", section, influx_tags);
    strcpy(saved_section, section);
    first_sub = 1;
    subended = 0;
    dprintf(dlevel, "ic_measure(\"%s\") count=%ld\n", section, output_char);
}

void ic_measureend()
{
    ic_check( 4 );
    remove_ending_comma_if_any();
    if (!subended) {
         output_char += sprintf(&output[output_char], "   \n");
    }
    subended = 0;
    dprintf(dlevel, "ic_measureend()\n");
}

/* Note this added a further tag to the measurement of the "resource_name" */
/* measurement might be "disks" */
/* sub might be "sda1", "sdb1", etc */
void ic_sub(char *resource)
{
    int i;

    ic_check( strlen(saved_section) + strlen(influx_tags) +strlen(saved_sub) + strlen(resource) + 9);

    /* remove previously added section */
    if (first_sub) {
	for (i = output_char - 1; i > 0; i--) {
	    if (output[i] == '\n') {
		output[i + 1] = 0;
		output_char = i + 1;
		break;
	    }
	}
    }
    first_sub = 0;

    /* remove the trailing s */
    strcpy(saved_sub, saved_section);
    if (saved_sub[strlen(saved_sub) - 1] == 's') {
	saved_sub[strlen(saved_sub) - 1] = 0;
    }
    output_char += sprintf(&output[output_char], "%s,%s,%s_name=%s ", saved_section, influx_tags, saved_sub, resource);
    subended = 0;
    dprintf(dlevel, "ic_sub(\"%s\") count=%ld\n", resource, output_char);
}

void ic_subend()
{
    ic_check( 4 );
    remove_ending_comma_if_any();
    output_char += sprintf(&output[output_char], "   \n");
    subended = 1;
    dprintf(dlevel, "ic_subend()\n");
}

void ic_long(char *name, long long value)
{
    ic_check( strlen(name) + 16 + 4 );
    output_char += sprintf(&output[output_char], "%s=%lldi,", name, value);
    dprintf(dlevel, "ic_long(\"%s\",%lld) count=%ld\n", name, value, output_char);
}

void ic_double(char *name, double value)
{
    ic_check( strlen(name) + 16 + 4 );
    if (isnan(value) || isinf(value)) { /* not-a-number or infinity */
	dprintf(dlevel, "ic_double(%s,%.1f) - nan error\n", name, value);
    } else {
	output_char += sprintf(&output[output_char], "%s=%.3f,", name, value);
	dprintf(dlevel, "ic_double(\"%s\",%.1f) count=%ld\n", name, value, output_char);
    }
}

void ic_string(char *name, char *value)
{
    int i;
    int len;

    ic_check( strlen(name) + strlen(value) + 4 );
    len = strlen(value);
    for (i = 0; i < len; i++) 	/* replace problem characters and with a space */
	if (value[i] == '\n' || iscntrl(value[i]))
	    value[i] = ' ';
    output_char += sprintf(&output[output_char], "%s=\"%s\",", name, value);
    dprintf(dlevel, "ic_string(\"%s\",\"%s\") count=%ld\n", name, value, output_char);
}

void ic_push()
{
    char header[1024];
    char result[1024];
    char buffer[1024 * 8];
    int ret;
    int i;
    int total;
    int sent;
    int code;

    if (output_char == 0)	/* nothing to send so skip this operation */
	return;
    if (influx_port) {
	dprintf(dlevel, "ic_push() size=%ld\n", output_char);
	if (create_socket() == 1) {

	    sprintf(buffer, "POST /write?db=%s&u=%s&p=%s HTTP/1.1\r\nHost: %s:%ld\r\nContent-Length: %ld\r\n\r\n",
		    influx_database, influx_username, influx_password, influx_hostname, influx_port, output_char);
	    dprintf(dlevel, "buffer size=%ld\nbuffer=<%s>\n", strlen(buffer), buffer);
	    if ((ret = write(sockfd, buffer, strlen(buffer))) != strlen(buffer)) {
			fprintf(stderr, "warning: \"write post to sockfd failed.\" errno=%d\n", errno);
	    }
	    total = output_char;
	    sent = 0;
 	    dprintf(dlevel, "output size=%d output=\n<%s>\n", total, output);
	    while (sent < total) {
		ret = write(sockfd, &output[sent], total - sent);
		dprintf(dlevel, "written=%d bytes sent=%d total=%d\n", ret, sent, total);
		if (ret < 0) {
		    fprintf(stderr, "warning: \"write body to sockfd failed.\" errno=%d\n", errno);
		    break;
		}
		sent = sent + ret;
	    }
	    for (i = 0; i < 1024; i++) /* empty the buffer */
		result[i] = 0;
	    if ((ret = read(sockfd, result, sizeof(result))) > 0) {
		result[ret] = 0;
		dprintf(dlevel, "received bytes=%d data=<%s>\n", ret, result);
		sscanf(result, "HTTP/1.1 %d", &code);
		for (i = 13; i < 1024; i++)
		    if (result[i] == '\r')
			result[i] = 0;
		    dprintf(dlevel, "http-code=%d text=%s [204=Success]\n", code, &result[13]);
		if (code != 204)
		    dprintf(dlevel, "code %d -->%s<--\n", code, result);
	    }
	    close(sockfd);
	    sockfd = 0;
	    dprintf(dlevel, "ic_push complete\n");
	} else {
	    dprintf(dlevel, "socket create failed\n");
	}
    } else error("influx port is not set, bailing out");

    output[0] = 0;
    output_char = 0;
}
#endif

void influx_add_props(config_t *cp, influx_config_t *gconf, char *name, influx_config_t *conf) {
	config_property_t influx_private_props[] = {
		{ "influx_endpoint", DATA_TYPE_STRING, conf->endpoint, sizeof(conf->endpoint)-1, "https://localhost:8086", 0,
                        0, 0, 0, 1, (char *[]){ "InfluxDB endpoint" }, 0, 1, 0 },
		{ "influx_database", DATA_TYPE_STRING, conf->database, sizeof(conf->database)-1, "", 0,
                        0, 0, 0, 1, (char *[]){ "InfluxDB database" }, 0, 1, 0 },
		{ "influx_username", DATA_TYPE_STRING, conf->username, sizeof(conf->username)-1, "", 0,
                        0, 0, 0, 1, (char *[]){ "InfluxDB username" }, 0, 1, 0 },
		{ "influx_password", DATA_TYPE_STRING, conf->password, sizeof(conf->password)-1, "", 0,
                        0, 0, 0, 1, (char *[]){ "InfluxDB password" }, 0, 1, 0 },
		{ 0 }
	};

	config_property_t influx_global_props[] = {
		{ "endpoint", DATA_TYPE_STRING, conf->endpoint, sizeof(conf->endpoint)-1, "https://localhost:8086", 0,
                        0, 0, 0, 1, (char *[]){ "InfluxDB endpoint" }, 0, 1, 0 },
		{ "database", DATA_TYPE_STRING, conf->database, sizeof(conf->database)-1, "", 0,
                        0, 0, 0, 1, (char *[]){ "InfluxDB database" }, 0, 1, 0 },
		{ "username", DATA_TYPE_STRING, conf->username, sizeof(conf->username)-1, "", 0,
                        0, 0, 0, 1, (char *[]){ "InfluxDB username" }, 0, 1, 0 },
		{ "password", DATA_TYPE_STRING, conf->password, sizeof(conf->password)-1, "", 0,
                        0, 0, 0, 1, (char *[]){ "InfluxDB password" }, 0, 1, 0 },
		{ 0 }
	};

	dprintf(dlevel,"name: %s\n", name);
	config_add_props(cp, name, influx_private_props, 0);
	config_add_props(cp, "influx", influx_global_props, 0);
}

/********************************************************************************/
/*
*** SERIES
*/

#ifdef JS
#include "jsengine.h"
#include "jsdate.h"

JSObject *js_values_new(JSContext *cx, JSObject *obj, influx_series_t *sp) {
	JSObject *values,*vo,*dateobj;
	jsval ie,ke;
	int i,j,time_col,doconv;
	influx_value_t *vp;
	JSString *str;
	jsdouble d;
	char *p;

	/* values is just a multimensional array */
	values = JS_NewArrayObject(cx, sp->value_count, NULL);
	if (!values) return 0;

	/* Get the time column */
	time_col = -1;
	for(i=0; i < sp->column_count; i++) {
		if (strcmp(sp->columns[i],"time") == 0) {
			time_col = i;
			break;
		}
	}
	for(i=0; i < sp->value_count; i++) {
		vo = JS_NewArrayObject(cx, sp->column_count, NULL);
		if (!vo) break;
		for(j=0; j < sp->column_count; j++) {
			vp = &sp->values[i][j];
			doconv = 1;
			/* Automatically convert the time field into a date object */
			if (j == time_col && sp->convdt) {
				p = vp->data;
				str = JS_NewString(cx, p, strlen(p));
				if (str && date_parseString(str,&d)) {
					dateobj = js_NewDateObjectMsec(cx,d);
					if (dateobj) {
						ke = OBJECT_TO_JSVAL(dateobj);
						doconv = 0;
					}
				}
			}
			if (doconv) ke = type_to_jsval(cx,vp->type,vp->data,vp->len);
			JS_SetElement(cx, vo, j, &ke);
		}
		ie = OBJECT_TO_JSVAL(vo);
		JS_SetElement(cx, values, i, &ie);
	}
	dprintf(dlevel,"returning: %p\n", values);
	return values;
}

enum SERIES_PROPERTY_ID {
	SERIES_PROPERTY_ID_NAME,
	SERIES_PROPERTY_ID_COLUMNS,
	SERIES_PROPERTY_ID_VALUES,
};

static JSBool series_getprop(JSContext *cx, JSObject *obj, jsval id, jsval *rval) {
	influx_series_t *sp;
	int prop_id;

	sp = JS_GetPrivate(cx, obj);
	dprintf(dlevel,"sp: %p\n", sp);
	if (!sp) {
		JS_ReportError(cx, "private is null!");
		return JS_FALSE;
	}

	dprintf(dlevel,"is_int: %d\n", JSVAL_IS_INT(id));
	if (JSVAL_IS_INT(id)) {
		prop_id = JSVAL_TO_INT(id);
		dprintf(dlevel,"prop_id: %d\n", prop_id);
		switch(prop_id) {
		case SERIES_PROPERTY_ID_NAME:
			if (!sp->name_val) sp->name_val = type_to_jsval(cx,DATA_TYPE_STRING,sp->name,strlen(sp->name));
			*rval = sp->name_val;
			break;
		case SERIES_PROPERTY_ID_COLUMNS:
			if (!sp->columns_val) sp->columns_val = type_to_jsval(cx,DATA_TYPE_STRING_ARRAY,sp->columns,sp->column_count);
			*rval = sp->columns_val;
			break;
		case SERIES_PROPERTY_ID_VALUES:
			if (!sp->values_val) sp->values_val = OBJECT_TO_JSVAL(js_values_new(cx,obj,sp));
			*rval = sp->values_val;
			break;
		}
	}
	return JS_TRUE;
}

static JSClass series_class = {
	"influx_series",	/* Name */
	JSCLASS_HAS_PRIVATE,	/* Flags */
	JS_PropertyStub,	/* addProperty */
	JS_PropertyStub,	/* delProperty */
	series_getprop,		/* getProperty */
	JS_PropertyStub,	/* setProperty */
	JS_EnumerateStub,	/* enumerate */
	JS_ResolveStub,		/* resolve */
	JS_ConvertStub,		/* convert */
	JS_FinalizeStub,	/* finalize */
	JSCLASS_NO_OPTIONAL_MEMBERS
};

JSObject *js_series_new(JSContext *cx, JSObject *parent, influx_series_t *sp) {
	JSPropertySpec series_props[] = { 
		{ "name",		SERIES_PROPERTY_ID_NAME,	JSPROP_ENUMERATE | JSPROP_READONLY },
		{ "columns",		SERIES_PROPERTY_ID_COLUMNS,	JSPROP_ENUMERATE | JSPROP_READONLY },
		{ "values",		SERIES_PROPERTY_ID_VALUES,	JSPROP_ENUMERATE | JSPROP_READONLY },
		{0}
	};
	JSObject *obj;

	dprintf(dlevel,"creating %s class...\n",series_class.name);
	obj = JS_InitClass(cx, parent, 0, &series_class, 0, 0, series_props, 0, 0, 0);
	if (!obj) {
		JS_ReportError(cx,"unable to initialize influx class");
		return 0;
	}
	JS_SetPrivate(cx,obj,sp);
	dprintf(dlevel,"done!\n");
	return obj;
}

/********************************************************************************/
/*
*** RESULTS
*/
JSObject *js_results_new(JSContext *cx, JSObject *obj, influx_session_t *s) {
	influx_series_t *sp;
	JSObject *results;
	jsval element;
	int count,i;

	/* Results is just an array of series */
	count = list_count(s->results);
	dprintf(dlevel,"count: %d\n", count);
	results = JS_NewArrayObject(cx, count, NULL);
	if (!s->results) return results;

	i = 0;
	list_reset(s->results);
	while((sp = list_get_next(s->results)) != 0) {
		element = OBJECT_TO_JSVAL(js_series_new(cx,obj,sp));
		JS_SetElement(cx, results, i++, &element);
	}
	dprintf(dlevel,"returning: %p\n", results);
	return results;
}

/********************************************************************************/
/*
*** INFLUX
*/

enum INFLUX_PROPERTY_ID {
	INFLUX_PROPERTY_ID_URL,
	INFLUX_PROPERTY_ID_DATABASE,
	INFLUX_PROPERTY_ID_USERNAME,
	INFLUX_PROPERTY_ID_PASSWORD,
	INFLUX_PROPERTY_ID_VERBOSE,
	INFLUX_PROPERTY_ID_RESULTS,
	INFLUX_PROPERTY_ID_ERRMSG,
	INFLUX_PROPERTY_ID_CONVDT,
};

static JSBool influx_getprop(JSContext *cx, JSObject *obj, jsval id, jsval *rval) {
	influx_session_t *s;
	int prop_id;

	s = JS_GetPrivate(cx, obj);
	dprintf(dlevel,"s: %p\n", s);
	if (!s) {
		JS_ReportError(cx, "private is null!");
		return JS_FALSE;
	}

	dprintf(dlevel,"is_int: %d\n", JSVAL_IS_INT(id));
	if (JSVAL_IS_INT(id)) {
		prop_id = JSVAL_TO_INT(id);
		dprintf(dlevel,"prop_id: %d\n", prop_id);
		switch(prop_id) {
		case INFLUX_PROPERTY_ID_URL:
			*rval = type_to_jsval(cx,DATA_TYPE_STRING,s->config.endpoint,strlen(s->config.endpoint));
			break;
		case INFLUX_PROPERTY_ID_DATABASE:
			*rval = type_to_jsval(cx,DATA_TYPE_STRING,s->config.database,strlen(s->config.database));
			break;
		case INFLUX_PROPERTY_ID_USERNAME:
			*rval = type_to_jsval(cx,DATA_TYPE_STRING,s->config.username,strlen(s->config.username));
			break;
		case INFLUX_PROPERTY_ID_PASSWORD:
			*rval = type_to_jsval(cx,DATA_TYPE_STRING,s->config.password,strlen(s->config.password));
			break;
		case INFLUX_PROPERTY_ID_VERBOSE:
			dprintf(dlevel,"verbose: %d\n", s->verbose);
			*rval = type_to_jsval(cx,DATA_TYPE_BOOL,&s->verbose,0);
			break;
		case INFLUX_PROPERTY_ID_RESULTS:
			dprintf(dlevel,"results_val: %d\n", s->results_val);
			if (!s->results_val) s->results_val = OBJECT_TO_JSVAL(js_results_new(cx,obj,s));
			*rval = s->results_val;
			break;
		case INFLUX_PROPERTY_ID_ERRMSG:
			*rval = type_to_jsval(cx,DATA_TYPE_STRING,s->errmsg,strlen(s->errmsg));
			break;
		case INFLUX_PROPERTY_ID_CONVDT:
			*rval = type_to_jsval(cx,DATA_TYPE_BOOL,&s->convdt,0);
			break;
		default:
			break;
		}
	}
	return JS_TRUE;
}

static JSBool influx_setprop(JSContext *cx, JSObject *obj, jsval id, jsval *vp) {
	influx_session_t *s;
	int prop_id;

	s = JS_GetPrivate(cx, obj);
	dprintf(dlevel,"s: %p\n", s);
	if (!s) {
		JS_ReportError(cx, "private is null!");
		return JS_FALSE;
	}

	if (JSVAL_IS_INT(id)) {
		prop_id = JSVAL_TO_INT(id);
		dprintf(dlevel,"influx_setprop: prop_id: %d", prop_id);
		switch(prop_id) {
		case INFLUX_PROPERTY_ID_URL:
			jsval_to_type(DATA_TYPE_STRING,s->config.endpoint,sizeof(s->config.endpoint),cx,*vp);
			break;
		case INFLUX_PROPERTY_ID_DATABASE:
			jsval_to_type(DATA_TYPE_STRING,s->config.database,sizeof(s->config.database),cx,*vp);
			break;
		case INFLUX_PROPERTY_ID_USERNAME:
			jsval_to_type(DATA_TYPE_STRING,s->config.username,sizeof(s->config.username),cx,*vp);
			break;
		case INFLUX_PROPERTY_ID_PASSWORD:
			jsval_to_type(DATA_TYPE_STRING,s->config.password,sizeof(s->config.password),cx,*vp);
			break;
		case INFLUX_PROPERTY_ID_VERBOSE:
			jsval_to_type(DATA_TYPE_BOOL,&s->verbose,0,cx,*vp);
			break;
		case INFLUX_PROPERTY_ID_CONVDT:
			jsval_to_type(DATA_TYPE_BOOL,&s->convdt,0,cx,*vp);
			break;
		default:
			break;
		}
	}
	return JS_TRUE;
}

static JSClass influx_class = {
	"Influx",		/* Name */
	JSCLASS_HAS_PRIVATE,	/* Flags */
	JS_PropertyStub,	/* addProperty */
	JS_PropertyStub,	/* delProperty */
	influx_getprop,		/* getProperty */
	influx_setprop,		/* setProperty */
	JS_EnumerateStub,	/* enumerate */
	JS_ResolveStub,		/* resolve */
	JS_ConvertStub,		/* convert */
	JS_FinalizeStub,	/* finalize */
	JSCLASS_NO_OPTIONAL_MEMBERS
};

static char *js_string(JSContext *cx, jsval val) {
	JSString *str;

	str = JS_ValueToString(cx, val);
	if (!str) return JS_FALSE;
	return JS_EncodeString(cx, str);
}

static JSBool js_influx_query(JSContext *cx, uintN argc, jsval *vp) {
	JSObject *obj;
	influx_session_t *s;
	jsval *argv = vp + 2;
	char *type,*query;

	obj = JS_THIS_OBJECT(cx, vp);
	if (!obj) return JS_FALSE;
	s = JS_GetPrivate(cx, obj);
	dprintf(dlevel,"s: %p\n", s);

	if (argc != 1) goto js_influx_query_usage;

	/* If the 2nd arg is a string, treat it as JSON */
	type = jstypestr(cx,argv[0]);
	dprintf(dlevel,"arg type: %s\n", type);
	if (strcmp(type,"string") != 0) goto js_influx_query_usage;

	query = JS_GetStringBytes(JSVAL_TO_STRING(argv[0]));
	dprintf(dlevel,"query: %s\n", query);
	if (influx_query(s,query)) {
		*vp = JSVAL_VOID;
		return JS_TRUE;
	}
	s->results_val = OBJECT_TO_JSVAL(js_results_new(cx, obj, s));
	dprintf(dlevel,"results_val: %s\n", jstypestr(cx,s->results_val));
	*vp = s->results_val;
	return JS_TRUE;

js_influx_query_usage:
	JS_ReportError(cx,"influx.query requires 1 argument (query:string)");
	return JS_FALSE;
}

static JSBool js_influx_write(JSContext *cx, uintN argc, jsval *vp) {
	JSObject *obj,*rec;
	JSIdArray *ida;
	jsval *ids;
	char *name;
	influx_session_t *s;
	jsval *argv = vp + 2;
	int i;
	char *type,*key,*value;
	jsval val;
	JSClass *classp;

	obj = JS_THIS_OBJECT(cx, vp);
	if (!obj) return JS_FALSE;
	s = JS_GetPrivate(cx, obj);
	dprintf(dlevel,"s: %p\n", s);

	if (argc != 2) goto js_influx_write_usage;

	/* Get the measurement name */
	if (strcmp(jstypestr(cx,argv[0]),"string") != 0) goto js_influx_write_usage;
	name = JS_GetStringBytes(JSVAL_TO_STRING(argv[0]));
	dprintf(dlevel,"name: %s\n", name);

	/* If the 2nd arg is a string, treat it as JSON */
	type = jstypestr(cx,argv[1]);
	dprintf(dlevel,"arg 2 type: %s\n", type);
	if (strcmp(type,"string") == 0) {
		value = JS_GetStringBytes(JSVAL_TO_STRING(argv[1]));
		dprintf(dlevel,"value: %s\n", value);

	/* Otherwise it's an object - gets the keys and values */
	} else if (strcmp(type,"object") == 0) {
		rec = JSVAL_TO_OBJECT(argv[1]);
		classp = JS_GET_CLASS(cx, rec);
		dprintf(dlevel,"class: %s\n", classp->name);
		if (strcmp(classp->name,"Object") != 0) goto js_influx_write_usage;
		ida = JS_Enumerate(cx, rec);
		dprintf(dlevel,"ida: %p, count: %d\n", ida, ida->length);
		ids = &ida->vector[0];
		for(i=0; i< ida->length; i++) {
	//		dprintf(dlevel+1,"ids[%d]: %s\n", i, jstypestr(cx,ids[i]));
			key = js_string(cx,ids[i]);
			if (!key) continue;
	//		dprintf(dlevel+1,"key: %s\n", key);
			if (!JS_GetProperty(cx,rec,key,&val)) {
				JS_free(cx,key);
				continue;
			}
	//		dprintf(dlevel+1,"val: %s\n",jstypestr(cx,val));
			value = js_string(cx, val);
			if (!value) {
				JS_free(cx,key);
				continue;
			}
			dprintf(dlevel,"key: %s, value: %s\n", key, value);

			JS_free(cx,key);
			JS_free(cx,value);
		}
	} else {
		goto js_influx_write_usage;
	}
    	return JS_TRUE;
js_influx_write_usage:
	JS_ReportError(cx,"influx.write requires 2 arguments (measurement:string, rec:object OR rec:string)");
	return JS_FALSE;
}

JSObject *jsinflux_new(JSContext *cx, JSObject *parent, influx_session_t *i) {
	JSPropertySpec influx_props[] = { 
		{ "endpoint",		INFLUX_PROPERTY_ID_URL,		JSPROP_ENUMERATE },
		{ "database",		INFLUX_PROPERTY_ID_DATABASE,	JSPROP_ENUMERATE },
		{ "username",		INFLUX_PROPERTY_ID_USERNAME,	JSPROP_ENUMERATE },
		{ "password",		INFLUX_PROPERTY_ID_PASSWORD,	JSPROP_ENUMERATE },
		{ "verbose",		INFLUX_PROPERTY_ID_VERBOSE,	JSPROP_ENUMERATE },
		{ "convert_time",	INFLUX_PROPERTY_ID_CONVDT,	JSPROP_ENUMERATE },
		{ "results",		INFLUX_PROPERTY_ID_RESULTS,	JSPROP_ENUMERATE | JSPROP_READONLY },
		{ "errmsg",		INFLUX_PROPERTY_ID_ERRMSG,	JSPROP_ENUMERATE | JSPROP_READONLY },
		{0}
	};
	JSFunctionSpec influx_funcs[] = {
		JS_FN("query",js_influx_query,1,1,0),
		JS_FN("write",js_influx_write,2,2,0),
		{ 0 }
	};
	JSObject *obj;

	obj = JS_InitClass(cx, parent, 0, &influx_class, 0, 0, influx_props, influx_funcs, 0, 0);
	if (!obj) {
		JS_ReportError(cx,"unable to initialize influx class");
		return 0;
	}
	JS_SetPrivate(cx,obj,i);
	return obj;
}
#endif

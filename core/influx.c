
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
#define DEBUG DEBUG_INFLUX
#endif
#include "debug.h"

#include "common.h"
#include "influx.h"
#include <curl/curl.h>
#ifdef JS
#include "jsnum.h" /* for JSDOUBLE_IS_NaN */
#endif

#define SESSION_ID_SIZE 128

struct influx_session {
	char endpoint[INFLUX_ENDPOINT_SIZE];
	char database[INFLUX_DATABASE_SIZE];
	char username[INFLUX_USERNAME_SIZE];
	char password[INFLUX_PASSWORD_SIZE];
	bool connected;
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
#ifdef JS
	jsval results_val;
#endif
	char *login_fields;
	char *read_fields;
	char *config_fields;
	bool verbose;
	bool convdt;
};
typedef struct influx_session influx_session_t;

int influx_parse_config(influx_session_t *s, char *influx_info) {
	dprintf(dlevel,"info: %s\n", influx_info);
	strncpy(s->endpoint,strele(0,",",influx_info),sizeof(s->endpoint)-1);
	strncpy(s->database,strele(1,",",influx_info),sizeof(s->database)-1);
	strncpy(s->username,strele(2,",",influx_info),sizeof(s->username)-1);
	strncpy(s->password,strele(3,",",influx_info),sizeof(s->password)-1);
	dprintf(dlevel,"endpoint: %s, database: %s, user: %s, pass: %s\n", s->endpoint, s->database, s->username, s->password);
	return 0;
}

#if 0
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
#endif

static size_t getdata(void *ptr, size_t size, size_t nmemb, void *ctx) {
	influx_session_t *s = ctx;
	int bytes,newidx;

	if (s->verbose) log_info("data: %s\n",(char *)ptr);

	dprintf(dlevel,"bufsize: %d\n", s->bufsize);

	bytes = size*nmemb;
	dprintf(dlevel,"bytes: %d, bufidx: %d, bufsize: %d\n", bytes, s->bufidx, s->bufsize);
	newidx = s->bufidx + bytes;
	dprintf(dlevel,"newidx: %d\n", newidx);
	if (newidx > s->bufsize) {
		char *newbuf;
		int newsize;

		newsize = newidx + 1;
		newbuf = realloc(s->buffer,newsize);
		dprintf(dlevel,"newbuf: %p\n", newbuf);
		if (!newbuf) {
			log_syserror("influx_getdata: realloc(buffer,%d)",newsize);
			return 0;
		}
		s->buffer = newbuf;
		s->bufsize = newidx;
	}
	strcpy(s->buffer + s->bufidx,ptr);
	s->bufidx = newidx;

	return bytes;
}

influx_session_t *influx_new(void) {
	influx_session_t *s;

	s = calloc(sizeof(*s),1);
	if (!s) {
		log_syserror("influx_new: calloc");
		return 0;
	}

	s->curl = curl_easy_init();
	if (!s->curl) {
		log_syserror("influx_new: curl_init");
		free(s);
		return 0;
	}

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

	if (strlen(s->username)) {
		curl_easy_setopt(s->curl, CURLOPT_HTTPAUTH, CURLAUTH_BASIC);
		curl_easy_setopt(s->curl, CURLOPT_USERPWD, s->username);
		curl_easy_setopt(s->curl, CURLOPT_HTTPAUTH, CURLAUTH_BASIC);
		curl_easy_setopt(s->curl, CURLOPT_USERPWD, s->password);
	}

	return s;
}

list influx_get_results(influx_session_t *s) {
	return s->results;
}

char *influx_mkurl(influx_session_t *s, char *action, char *query) {
	char *url,*eq,*p;
	int size,db;

	if (action) {
		db = 1;
		if (strcmp(action,"health")==0) db = 0;
	} else {
		db = 0;
	}

	/* default endpoint */
	if (!strlen(s->endpoint)) strcpy(s->endpoint,"http://localhost:8086");

	size = 0;
	size += strlen(s->endpoint);
	if (action) {
		size += strlen(action) + 1; /* + "/" */
		size += strlen(s->database) + 4;	/* + "?db=" */
	}
	eq = (query ? curl_easy_escape(s->curl, query, strlen(query)) : 0);
	if (eq) size += strlen(eq) + 3; /* + "&q=" */
	dprintf(dlevel,"size: %d\n", size);
	url = malloc(size+1);
	dprintf(dlevel,"url: %p\n", url);
	if (!url) {
		log_syserror("influx_mkurl: malloc(%d)",size);
		return 0;
	}
	p = url;
	p += sprintf(p,"%s",s->endpoint);
	if (action) {
		p += sprintf(p,"/%s",action);
		if (db) p += sprintf(p,"?db=%s",s->database);
	}
	if (eq) {
		p += sprintf(p,"&q=%s",eq);
		curl_free(eq);
	}
	dprintf(dlevel,"url: %s\n", url);
	return url;
}

void influx_destroy_results(influx_session_t *s) {
	influx_series_t *sp;
	influx_value_t *vp;
	int i,j,count;

	if (!s->results) return;

	count = list_count(s->results);
	dprintf(dlevel,"count: %d\n",count);
	if (count) {
	list_reset(s->results);
	while((sp = list_get_next(s->results)) != 0) {
		dprintf(dlevel,"sp->columns: %p\n", sp->columns);
		if (sp->columns) {
			dprintf(dlevel,"column_count: %d\n", sp->column_count);
			for(i=0; i < sp->column_count; i++) {
				dprintf(dlevel,"columns[%d]: %p\n", i, sp->columns[i]);
				if (!sp->columns[i]) break;
				free(sp->columns[i]);
			}
			free(sp->columns);
		}
		dprintf(dlevel,"sp->values: %p\n", sp->values);
		if (sp->values) {
			dprintf(dlevel,"value_count: %d\n", sp->value_count);
			for(i=0; i < sp->value_count; i++) {
				dprintf(dlevel,"values[%d]: %p\n", i, sp->values[i]);
				if (!sp->values[i]) break;
				for(j=0; j < sp->column_count; j++) {
					vp = &sp->values[i][j];
					if (vp->data) free(vp->data);
				}
				free(sp->values[i]);
			}
			free(sp->values);
		}
	}
	}
	list_destroy(s->results);
	s->results = 0;
#ifdef JS
	s->results_val = 0;
#endif
	dprintf(dlevel,"done!\n");
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
			log_syserror("influx_getval: malloc(%d)",sizeof(double));
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
			log_syserror("influx_getval: malloc(%d)\n", vp->len+1);
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
#ifdef JS
		newseries.convdt = s->convdt;
#endif
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
		newseries.values = calloc(newseries.value_count * sizeof(influx_value_t *),1);
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
				newseries.values[j] = calloc(aa->count * sizeof(influx_value_t),1);
				if (!newseries.values[j]) {
					sprintf(s->errmsg,"malloc values");
					return 1;
				}
				for(k=0; k < aa->count; k++) getval(s,aa->items[k],&newseries.values[j][k]);
			}
		}
		list_add(s->results,&newseries,sizeof(newseries));
	}
	return 0;
}

static int influx_request(influx_session_t *s, char *url, int post, char *data) {
	struct curl_slist *hs = curl_slist_append(0, "Content-Type: application/x-www-form-urlencoded");
	CURLcode res;
	int rc,err;
	char *msg;

	dprintf(dlevel,"url: %s, post: %d, data: %s\n", url, post, data);

	influx_destroy_results(s);
#if 0
	dprintf(dlevel,"data: %p\n", s->data);
	if (s->data) {
		json_destroy_value(s->data);
		s->data = 0;
	}
#endif
	s->results = list_create();
	s->bufidx = 0;

	/* Make the request */
//	s->verbose = 1;
	curl_easy_setopt(s->curl, CURLOPT_VERBOSE, s->verbose);
	curl_easy_setopt(s->curl, CURLOPT_URL, url);
	curl_easy_setopt(s->curl, CURLOPT_HTTPHEADER, hs);
	curl_easy_setopt(s->curl, CURLOPT_POST, post);
	if (post && data) curl_easy_setopt(s->curl, CURLOPT_POSTFIELDS, data);
	dprintf(dlevel,"calling perform...\n");
	res = curl_easy_perform(s->curl);
	dprintf(dlevel,"res: %d\n", res);
	if (res != CURLE_OK) {
		sprintf(s->errmsg,"influx_request failed: %s", curl_easy_strerror(res));
		dprintf(dlevel,"%s\n",s->errmsg);
		return 1;
	}

	/* Get the response code */
	dprintf(dlevel,"getting rc...\n");
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
	if (!s->data) return err;

	/* If it's results, process it */
	dprintf(dlevel,"type: %s\n", json_typestr(json_value_get_type(s->data)));
	if (json_value_get_type(s->data) == JSON_TYPE_OBJECT) {
		json_object_t *o;

		o = json_value_get_object(s->data);
		if (o->count >= 1 && strcmp(o->names[0],"results") == 0 && json_value_get_type(o->values[0]) == JSON_TYPE_ARRAY) {
			int i,j,type;
			json_object_t *oo;
			json_array_t *a = json_value_get_array(o->values[0]);

			dprintf(dlevel,"count: %d\n", a->count);
			for(i=0; i < a->count; i++) {
				type = json_value_get_type(a->items[i]);
				dprintf(dlevel,"type[%d]: %s\n", i, json_typestr(type));
				if (json_value_get_type(a->items[i]) != JSON_TYPE_OBJECT) continue;
				oo = json_value_get_object(a->items[i]);
				dprintf(dlevel,"count: %d\n", oo->count);
				for(j=0; j < oo->count; j++) {
					dprintf(dlevel,"oo->names[%d]: %s\n", j, oo->names[j]);
					if (strcmp(oo->names[j],"series") == 0 && json_value_get_type(oo->values[j]) == JSON_TYPE_ARRAY) {
						if (getseries(s, json_value_get_array(oo->values[j]))) {
							influx_destroy_results(s);
							break;
						}
					}
				}
			}
		}
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
	char *url;
	long rc;
	char *p;

	url = influx_mkurl(s,"health",0);
	dprintf(dlevel,"url: %p\n", url);
	if (!url) return 1;
	if (influx_request(s,url,0,0)) {
		free(url);
		return 1;
	}
	curl_easy_getinfo(s->curl, CURLINFO_RESPONSE_CODE, &rc);
	dprintf(dlevel,"rc: %d\n", rc);
	free(url);
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
	char *url;
	long rc;

	url = influx_mkurl(s,"ping",0);
	dprintf(dlevel,"url: %p\n", url);
	if (!url) return 1;
//	curl_easy_setopt(s->curl, CURLOPT_HEADERFUNCTION, getheader);
//	curl_easy_setopt(s->curl, CURLOPT_HEADERDATA, s);
	if (influx_request(s,url,0,0)) {
		free(url);
		return 1;
	}
//	curl_easy_setopt(s->curl, CURLOPT_HEADERFUNCTION, 0);
//	curl_easy_setopt(s->curl, CURLOPT_HEADERDATA, 0);
	curl_easy_getinfo(s->curl, CURLINFO_RESPONSE_CODE, &rc);
	dprintf(dlevel,"rc: %d\n", rc);
	free(url);
	return (rc == 200 || rc == 204 ? 0 : 1);
}

int influx_query(influx_session_t *s, char *query) {
	char *postcmds[] = { "ALTER", "CREATE", "DELETE", "DROP", "GRANT", "KILL", "REVOKE", "SELECT INTO", 0 };
	char *url,*lcq,**p;
	int r,post;

	lcq = stredit(query,"COMPRESS,UPPERCASE");
	dprintf(dlevel,"lcq: %s\n", lcq);
	post = 0;
	for(p = postcmds; *p; p++) {
		dprintf(dlevel,"p: %s\n", *p);
		if (strstr(lcq,*p)) {
			post = 1;
			break;
		}
	}
	dprintf(dlevel,"post: %d\n", post);

	url = influx_mkurl(s,"query",query);
	dprintf(dlevel,"url: %p\n", url);
	if (!url) return 1;
	dprintf(dlevel,"calling request..\n");
	r = influx_request(s,url,post,0);
	dprintf(dlevel,"r: %d\n", r);
	if (!r && 0) display_results(s);
	dprintf(dlevel,"freeing url...\n");
	free(url);
	return r;
}

int influx_connect(influx_session_t *s) {
	int r;

	r = 1;
	if (!strlen(s->endpoint)) goto influx_connect_done;
	if (!strlen(s->database)) goto influx_connect_done;
	r = influx_query(s,"show measurements");
influx_connect_done:
	dprintf(dlevel,"r: %d\n", r);
	s->connected = (r == 0);
	influx_destroy_results(s);
	return r;
}

int influx_connected(influx_session_t *s) {
	return s->connected;
}

int influx_write(influx_session_t *s, char *mm, char *string) {
	char *url;
	int r;

	dprintf(dlevel,"mm: %s, string: %s\n", mm, string);
#if 0
consistency=[any,one,quorum,all]	Optional, available with InfluxEnterprise clusters only.	Sets the write consistency for the point. InfluxDB assumes that the write consistency is one if you do not specify consistency. See the InfluxEnterprise documentation for detailed descriptions of each consistency option.
db=<database>	Required	Sets the target database for the write.
p=<password>	Optional if you haven.t enabled authentication. Required if you.ve enabled authentication.*	Sets the password for authentication if you.ve enabled authentication. Use with the query string parameter u.
precision=[ns,u,ms,s,m,h]	Optional	Sets the precision for the supplied Unix time values. InfluxDB assumes that timestamps are in nanoseconds if you do not specify precision.**
rp=<retention_policy_name>	Optional	Sets the target retention policy for the write. InfluxDB writes to the DEFAULT retention policy if you do not specify a retention policy.
u=<username>	Optional if you haven.t enabled authentication. Required if you.ve enabled authentication.*	Sets the username for authentication if you.ve enabled authentication. The user must have write access to the database. Use with the query string parameter p.
#endif
	url = influx_mkurl(s,"write",0);
	dprintf(dlevel,"url: %p\n", url);
	if (!url) return 1;
	dprintf(dlevel,"calling request..\n");
	r = influx_request(s,url,1,string);
	dprintf(dlevel,"r: %d\n", r);
	if (!r && 0) display_results(s);
	dprintf(dlevel,"freeing url...\n");
	free(url);
	return r;
}

int influx_write_props(influx_session_t *s, char *name, config_property_t *props) {
	register config_property_t *pp;
	char value[2048], *string;
	int strsize,stridx,newidx,len,count;

	dprintf(dlevel,"props: %p\n", props);
	if (!props) return 1;

	strsize = 1024;
	string = malloc(strsize);
	stridx = sprintf(string,"%s ",name);
	count = 0;
	for(pp = props; pp->name; pp++) {
		dprintf(dlevel,"name: %s, type: %x, dest: %p, len: %d\n", pp->name, pp->type, pp->dest, pp->len);
		len = conv_type(DATA_TYPE_STRING,value,sizeof(value)-1,pp->type,pp->dest,pp->len);
		dprintf(dlevel,"len: %d, stridx: %d, strsize: %d\n", len, stridx, strsize);
		/* name=value, if string add 2 for quotes */
		newidx = stridx + strlen(pp->name) + 1 + len;
		if (pp->type == DATA_TYPE_STRING) newidx += 2;
		/* comma */
		if (count) newidx++;
		dprintf(dlevel,"newidx: %d\n", newidx);
		if (newidx > strsize) {
			char *newstr;
			int newsize;

			newsize = newidx + 1;
			newstr = realloc(string,newsize);
			dprintf(dlevel,"newstr: %p\n", newstr);
			if (!newstr) {
				log_syserror("influx_getdata: realloc(string,%d)",newsize);
				return 0;
			}
			string = newstr;
			strsize = newidx;
		}
		if (pp->type == DATA_TYPE_STRING)
			sprintf(string + stridx, "%s%s=\"%s\"\n", (count ? "," : ""), pp->name, value);
		else
			sprintf(string + stridx, "%s%s=%s\n", (count ? "," : ""), pp->name, value);
		stridx = newidx;
		count++;
	}
	dprintf(dlevel,"string: %s\n", string);
	len = influx_write(s,name,string);
	free(string);
	dprintf(dlevel,"returning: %d\n", len);
	return len;
}

int influx_write_json(influx_session_t *s, char *name, json_value_t *rv) {
	json_object_t *o;
	char value[2048], *string;
	int strsize,stridx,newidx,len,count;
	int i,type;

	if (!rv) return 1;

	if (json_value_get_type(rv) != JSON_TYPE_OBJECT) {
		sprintf(s->errmsg,"influx_write_json: invalid root value type");
		return 1;
	}
	strsize = 1024;
	string = malloc(strsize);
	stridx = sprintf(string,"%s ",name);
	count = 0;
	o = json_value_get_object(rv);
	for(i=0; i < o->count; i++) {
		type = json_value_get_type(o->values[i]);
		dprintf(dlevel,"name: %s, type: %x\n", o->names[i], json_typestr(type));
		len = json_to_type(DATA_TYPE_STRING,value,sizeof(value)-1,o->values[i]);
		dprintf(dlevel,"len: %d, stridx: %d, strsize: %d\n", len, stridx, strsize);
		/* name=value, if string add 2 for quotes */
		newidx = stridx + strlen(o->names[i]) + 1 + len;
		if (type == JSON_TYPE_STRING) newidx += 2;
		/* comma */
		if (count) newidx++;
		dprintf(dlevel,"newidx: %d\n", newidx);
		if (newidx > strsize) {
			char *newstr;
			int newsize;

			newsize = newidx + 1;
			newstr = realloc(string,newsize);
			dprintf(dlevel,"newstr: %p\n", newstr);
			if (!newstr) {
				log_syserror("influx_getdata: realloc(string,%d)",newsize);
				return 0;
			}
			string = newstr;
			strsize = newidx;
		}
		if (type == JSON_TYPE_STRING)
			sprintf(string + stridx, "%s%s=\"%s\"\n", (count ? "," : ""), o->names[i], value);
		else
			sprintf(string + stridx, "%s%s=%s\n", (count ? "," : ""), o->names[i], value);
		stridx = newidx;
		count++;
	}
	dprintf(dlevel,"string: %s\n", string);
	len = influx_write(s,name,string);
	free(string);
	dprintf(dlevel,"returning: %d\n", len);
	return len;
}

void influx_add_props(influx_session_t *s, config_t *cp, char *name) {
	config_property_t influx_private_props[] = {
		{ "influx_endpoint", DATA_TYPE_STRING, s->endpoint, sizeof(s->endpoint)-1, "http://localhost:8086", 0,
                        0, 0, 0, 1, (char *[]){ "InfluxDB endpoint" }, 0, 1, 0 },
		{ "influx_database", DATA_TYPE_STRING, s->database, sizeof(s->database)-1, "", 0,
                        0, 0, 0, 1, (char *[]){ "InfluxDB database" }, 0, 1, 0 },
		{ "influx_username", DATA_TYPE_STRING, s->username, sizeof(s->username)-1, "", 0,
                        0, 0, 0, 1, (char *[]){ "InfluxDB username" }, 0, 1, 0 },
		{ "influx_password", DATA_TYPE_STRING, s->password, sizeof(s->password)-1, "", 0,
                        0, 0, 0, 1, (char *[]){ "InfluxDB password" }, 0, 1, 0 },
		{ 0 }
	};

#if 0
	config_property_t influx_global_props[] = {
		{ "endpoint", DATA_TYPE_STRING, s->endpoint, sizeof(s->endpoint)-1, "http://localhost:8086", 0,
                        0, 0, 0, 1, (char *[]){ "InfluxDB endpoint" }, 0, 1, 0 },
		{ "database", DATA_TYPE_STRING, s->database, sizeof(s->database)-1, "", 0,
                        0, 0, 0, 1, (char *[]){ "InfluxDB database" }, 0, 1, 0 },
		{ "username", DATA_TYPE_STRING, s->username, sizeof(s->username)-1, "", 0,
                        0, 0, 0, 1, (char *[]){ "InfluxDB username" }, 0, 1, 0 },
		{ "password", DATA_TYPE_STRING, s->password, sizeof(s->password)-1, "", 0,
                        0, 0, 0, 1, (char *[]){ "InfluxDB password" }, 0, 1, 0 },
		{ 0 }
	};
#endif

	dprintf(dlevel,"name: %s\n", name);
	config_add_props(cp, name, influx_private_props, 0);
//	config_add_props(cp, "influx", influx_global_props, 0);
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
	dprintf(dlevel,"values: %p\n", values);
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
		dprintf(dlevel,"vo: %p\n", vo);
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
	INFLUX_PROPERTY_ID_CONNECTED,
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
			*rval = type_to_jsval(cx,DATA_TYPE_STRING,s->endpoint,strlen(s->endpoint));
			break;
		case INFLUX_PROPERTY_ID_DATABASE:
			*rval = type_to_jsval(cx,DATA_TYPE_STRING,s->database,strlen(s->database));
			break;
		case INFLUX_PROPERTY_ID_USERNAME:
			*rval = type_to_jsval(cx,DATA_TYPE_STRING,s->username,strlen(s->username));
			break;
		case INFLUX_PROPERTY_ID_PASSWORD:
			*rval = type_to_jsval(cx,DATA_TYPE_STRING,s->password,strlen(s->password));
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
		case INFLUX_PROPERTY_ID_CONNECTED:
			*rval = type_to_jsval(cx,DATA_TYPE_BOOL,&s->connected,0);
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
			jsval_to_type(DATA_TYPE_STRING,s->endpoint,sizeof(s->endpoint),cx,*vp);
			break;
		case INFLUX_PROPERTY_ID_DATABASE:
			jsval_to_type(DATA_TYPE_STRING,s->database,sizeof(s->database),cx,*vp);
			break;
		case INFLUX_PROPERTY_ID_USERNAME:
			jsval_to_type(DATA_TYPE_STRING,s->username,sizeof(s->username),cx,*vp);
			break;
		case INFLUX_PROPERTY_ID_PASSWORD:
			jsval_to_type(DATA_TYPE_STRING,s->password,sizeof(s->password),cx,*vp);
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
	"INFLUX",		/* Name */
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
	char *type,*key;
	jsval val;

	obj = JS_THIS_OBJECT(cx, vp);
	if (!obj) return JS_FALSE;
	s = JS_GetPrivate(cx, obj);
	dprintf(dlevel,"s: %p\n", s);

	if (argc != 2) goto js_influx_write_usage;

	/* Get the measurement name */
	if (strcmp(jstypestr(cx,argv[0]),"string") != 0) goto js_influx_write_usage;
	name = JS_GetStringBytes(JSVAL_TO_STRING(argv[0]));
	dprintf(dlevel,"name: %s\n", name);

	/* If the 2nd arg is a string, send it direct */
	type = jstypestr(cx,argv[1]);
	dprintf(dlevel,"arg 2 type: %s\n", type);
	if (strcmp(type,"string") == 0) {
		char *value = JS_GetStringBytes(JSVAL_TO_STRING(argv[1]));
		dprintf(dlevel,"value: %s\n", value);
		*vp = INT_TO_JSVAL(influx_write(s,name,value));
		JS_free(cx,value);

	/* Otherwise it's an object - gets the keys and values */
	} else if (strcmp(type,"object") == 0) {
		char *string,temp[2048],value[1536];
		int index,size,newidx,len;

		rec = JSVAL_TO_OBJECT(argv[1]);
		ida = JS_Enumerate(cx, rec);
		dprintf(dlevel,"ida: %p, count: %d\n", ida, ida->length);
		ids = &ida->vector[0];
		size = 1024;
		string = malloc(size);
		index = sprintf(string,"%s",name);
		for(i=0; i< ida->length; i++) {
	//		dprintf(dlevel+1,"ids[%d]: %s\n", i, jstypestr(cx,ids[i]));
			key = js_string(cx,ids[i]);
			if (!key) continue;
	//		dprintf(dlevel+1,"key: %s\n", key);
			if (!JS_GetProperty(cx,rec,key,&val)) {
				JS_free(cx,key);
				continue;
			}
			jsval_to_type(DATA_TYPE_STRING,&value,sizeof(value)-1,cx,val);
			dprintf(dlevel,"key: %s, type: %s, value: %s\n", key, jstypestr(cx,val), value);
			if (JSVAL_IS_NUMBER(val)) {
				jsdouble d;
				if (JSVAL_IS_INT(val)) {
					d = (jsdouble)JSVAL_TO_INT(val);
					if (JSDOUBLE_IS_NaN(d)) strcpy(value,"0");
				} else {
					d = *JSVAL_TO_DOUBLE(val);
					if (JSDOUBLE_IS_NaN(d)) strcpy(value,"0.0");
				}
				len = snprintf(temp,sizeof(temp)-1,"%s=%s",key,value);
			} else if (JSVAL_IS_STRING(val)) {
				len = snprintf(temp,sizeof(temp)-1,"%s=\"%s\"",key,value);
			} else {
				dprintf(dlevel,"ignoring field %s: unhandled type\n", key);
				continue;
			}
			dprintf(dlevel,"temp: %s\n", temp);
			/* XXX include comma */
			newidx = index + len + 1;
			dprintf(dlevel,"newidx: %d, size: %d\n", newidx, size);
			if (newidx > size) {
				char *newstr;

				dprintf(dlevel,"string: %p\n", string);
				newstr = realloc(string,newidx);
				dprintf(dlevel,"newstr: %p\n", newstr);
				if (!newstr) {
					JS_ReportError(cx,"Influx.write: memory allocation error");
					JS_free(cx,key);
					JS_free(cx,value);
					free(string);
					return JS_FALSE;
				}
				string = newstr;
				size = newidx;
			}
			sprintf(string + index,"%s%s",(i ? "," : " "),temp);
			index = newidx;
			JS_free(cx,key);
//			JS_free(cx,value);
		}
		string[index] = 0;
		dprintf(dlevel,"string: %s\n", string);
		*vp = INT_TO_JSVAL(influx_write(s,name,string));
		free(string);
	} else {
		goto js_influx_write_usage;
	}
    	return JS_TRUE;
js_influx_write_usage:
	JS_ReportError(cx,"Influx.write requires 2 arguments (measurement:string, rec:object OR rec:string)");
	return JS_FALSE;
}

static JSBool js_influx_ctor(JSContext *cx, JSObject *obj, uintN argc, jsval *argv, jsval *rval) {
	influx_session_t *s;
	JSObject *newobj;
	char *url, *db, *user, *pass;

	printf("argc: %d\n", argc);
	if (argc < 2) {
		JS_ReportError(cx, "INFLUX requires 2 arguments (endpoint:string, database:string)");
		return JS_FALSE;
	}
	url = db = user = pass = 0;
	if (!JS_ConvertArguments(cx, argc, argv, "s s / s s", &url, &db, &user, &pass))
		return JS_FALSE;
	dprintf(dlevel,"url: %s, db: %s, user: %s, pass: %s\n", url, db, user, pass);

	s = influx_new();
	if (!s) {
		JS_ReportError(cx, "influx_ctor: unable to create new session");
		return JS_FALSE;
	}
	strncpy(s->endpoint,url,sizeof(s->endpoint)-1);
	strncpy(s->database,db,sizeof(s->database)-1);
	if (user) strncpy(s->username,user,sizeof(s->username)-1);
	if (pass) strncpy(s->password,pass,sizeof(s->password)-1);
	dprintf(dlevel,"endpoint: %s, database: %s, user: %s, pass: %s\n", s->endpoint, s->database, s->username, s->password);

	newobj = js_influx_new(cx,JS_GetGlobalObject(cx),s);
	JS_SetPrivate(cx,newobj,s);
	*rval = OBJECT_TO_JSVAL(newobj);

	return JS_TRUE;
}

JSObject *js_influx_new(JSContext *cx, JSObject *parent, void *priv) {
	JSPropertySpec influx_props[] = { 
		{ "endpoint",		INFLUX_PROPERTY_ID_URL,		JSPROP_ENUMERATE },
		{ "database",		INFLUX_PROPERTY_ID_DATABASE,	JSPROP_ENUMERATE },
		{ "username",		INFLUX_PROPERTY_ID_USERNAME,	JSPROP_ENUMERATE },
		{ "password",		INFLUX_PROPERTY_ID_PASSWORD,	JSPROP_ENUMERATE },
		{ "connected",		INFLUX_PROPERTY_ID_CONNECTED,	JSPROP_ENUMERATE | JSPROP_READONLY },
		{ "verbose",		INFLUX_PROPERTY_ID_VERBOSE,	JSPROP_ENUMERATE },
		{ "convert_time",	INFLUX_PROPERTY_ID_CONVDT,	JSPROP_ENUMERATE },
		{ "results",		INFLUX_PROPERTY_ID_RESULTS,	JSPROP_ENUMERATE | JSPROP_READONLY },
		{ "errmsg",		INFLUX_PROPERTY_ID_ERRMSG,	JSPROP_ENUMERATE | JSPROP_READONLY },
		{0}
	};
	JSFunctionSpec influx_funcs[] = {
		JS_FN("query",js_influx_query,1,1,0),
		JS_FN("write",js_influx_write,2,2,0),
	};
	JSObject *obj;

	dprintf(dlevel,"creating influx class...\n");
	obj = JS_InitClass(cx, parent, 0, &influx_class, js_influx_ctor, 2, influx_props, influx_funcs, 0, 0);
	if (!obj) {
		JS_ReportError(cx,"unable to initialize influx class");
		return 0;
	}
	if (priv) JS_SetPrivate(cx,obj,priv);
	dprintf(dlevel,"done!\n");
	return obj;
}
#endif


/*
Copyright (c) 2021, Stephen P. Shoecraft
All rights reserved.

This source code is licensed under the BSD-style license found in the
LICENSE file in the root directory of this source tree.
*/

#include "common.h"
#include "influx.h"

#define DEBUG_INFLUX 0
#define dlevel 5

#ifdef DEBUG
#undef DEBUG
#endif
#define DEBUG DEBUG_INFLUX
#include "debug.h"

#include "socket.h"

struct influx_session {
	influx_config_t config;
	char errmsg[128];
};
typedef struct influx_session influx_session_t;

influx_session_t *influx_new(void) {
	influx_session_t *i;

	i = calloc(sizeof(*i),1);
	if (!i) {
		log_syserror("influx_new: calloc");
		return 0;
	}
	return i;
}

#if 0
/* ic_tags() argument is the measurement tags for influddb */
/* example: "host=vm1234"   note:the comma & hostname of the virtual machine sending the data */
/* complex: "host=lpar42,serialnum=987654,arch=power9" note:the comma separated list */
void ic_tags(char *t)	
{
    dprintf(1,"t: %s\n", t);
    if( influx_tags == (char *) 0) {
        if( (influx_tags = (char *)malloc(MEGABYTE)) == (char *)-1)
           error("failed to malloc() tags buffer");
    }

    strncpy(influx_tags,t,256);
}

/* note: converts influxdb hostname to ip address */
void ic_influx_database(char *host, long port, char *db) {
	struct hostent *he;
	char errorbuf[1024 +1 ];

	influx_port = port;
	strncpy(influx_database,db,256);

	if(host[0] <= '0' && host[0] <='9') { 
		dprintf(1,"ic_influx(ipaddr=%s,port=%ld,database=%s))\n",host,port,db);
		strncpy(influx_ip,host,16);
	} else {
		dprintf(1,"ic_influx_by_hostname(host=%s,port=%ld,database=%s))\n",host,port,db);
		strncpy(influx_hostname,host,1024);
		if (isalpha(host[0])) {

		    he = gethostbyname(host);
		    if (he == NULL) {
			sprintf(errorbuf, "influx host=%s to ip address convertion failed gethostbyname(), bailing out\n", host);
			error(errorbuf);
		    }
		    /* this could return multiple ip addresses but we assume its the first one */
		    if (he->h_addr_list[0] != NULL) {
			strcpy(influx_ip, inet_ntoa(*(struct in_addr *) (he->h_addr_list[0])));
			dprintf(1,"ic_influx_by_hostname hostname=%s converted to ip address %s))\n",host,influx_ip);
		    } else {
			sprintf(errorbuf, "influx host=%s to ip address convertion failed (empty list), bailing out\n", host);
			error(errorbuf);
		    }
		} else {
		    strcpy( influx_ip, host); /* perhaps the hostname is actually an ip address */
		}
        }
}

void ic_influx_userpw(char *user, char *pw)
{
	dprintf(1,"ic_influx_userpw(username=%s,pssword=%s))\n",user,pw);
	strncpy(influx_username,user,64);
	strncpy(influx_password,pw,64);
}

int create_socket() 		/* returns 1 for error and 0 for ok */
{
    int i;
//    static char buffer[4096];
//    static struct sockaddr_in serv_addr;

    dprintf(1, "socket: trying to connect to \"%s\":%ld\n", influx_ip, influx_port);
    if ((sockfd = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
	error("socket() call failed");
	return 0;
    }

    serv_addr.sin_family = AF_INET;
    serv_addr.sin_addr.s_addr = inet_addr(influx_ip);
    serv_addr.sin_port = htons(influx_port);

    /* connect tot he socket offered by the web server */
    if (connect(sockfd, (struct sockaddr *) &serv_addr, sizeof(serv_addr)) < 0) {
	dprintf(1, " connect() call failed errno=%d", errno);
	return 0;
    }
    return 1;
}

void ic_check(long adding) /* Check the buffer space */
{
	/* First time create the buffer */
    if(output == (char *)0) {
	if( (output = (char *)malloc(MEGABYTE)) == (char *)-1)
	    error("failed to malloc() output buffer");
    }
	/* When near the end of the output buffer, extend it*/
    if(output_char + (2*adding) > output_size) {
	if( (output = (char *)realloc(output, output_size + MEGABYTE)) == (char *)-1)
	    error("failed to realloc() output buffer");
    }
}

void remove_ending_comma_if_any()
{
    if (output[output_char - 1] == ',') {
	output[output_char - 1] = 0;	/* remove the char */
	output_char--;
    }
}

void ic_measure(char *section)
{
    ic_check( strlen(section) + strlen(influx_tags) + 3);

    output_char += sprintf(&output[output_char], "%s,%s ", section, influx_tags);
    strcpy(saved_section, section);
    first_sub = 1;
    subended = 0;
    dprintf(1, "ic_measure(\"%s\") count=%ld\n", section, output_char);
}

void ic_measureend()
{
    ic_check( 4 );
    remove_ending_comma_if_any();
    if (!subended) {
         output_char += sprintf(&output[output_char], "   \n");
    }
    subended = 0;
    dprintf(1, "ic_measureend()\n");
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
    dprintf(1, "ic_sub(\"%s\") count=%ld\n", resource, output_char);
}

void ic_subend()
{
    ic_check( 4 );
    remove_ending_comma_if_any();
    output_char += sprintf(&output[output_char], "   \n");
    subended = 1;
    dprintf(1, "ic_subend()\n");
}

void ic_long(char *name, long long value)
{
    ic_check( strlen(name) + 16 + 4 );
    output_char += sprintf(&output[output_char], "%s=%lldi,", name, value);
    dprintf(1, "ic_long(\"%s\",%lld) count=%ld\n", name, value, output_char);
}

void ic_double(char *name, double value)
{
    ic_check( strlen(name) + 16 + 4 );
    if (isnan(value) || isinf(value)) { /* not-a-number or infinity */
	dprintf(1, "ic_double(%s,%.1f) - nan error\n", name, value);
    } else {
	output_char += sprintf(&output[output_char], "%s=%.3f,", name, value);
	dprintf(1, "ic_double(\"%s\",%.1f) count=%ld\n", name, value, output_char);
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
    dprintf(1, "ic_string(\"%s\",\"%s\") count=%ld\n", name, value, output_char);
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
	dprintf(1, "ic_push() size=%ld\n", output_char);
	if (create_socket() == 1) {

	    sprintf(buffer, "POST /write?db=%s&u=%s&p=%s HTTP/1.1\r\nHost: %s:%ld\r\nContent-Length: %ld\r\n\r\n",
		    influx_database, influx_username, influx_password, influx_hostname, influx_port, output_char);
	    dprintf(1, "buffer size=%ld\nbuffer=<%s>\n", strlen(buffer), buffer);
	    if ((ret = write(sockfd, buffer, strlen(buffer))) != strlen(buffer)) {
			fprintf(stderr, "warning: \"write post to sockfd failed.\" errno=%d\n", errno);
	    }
	    total = output_char;
	    sent = 0;
 	    dprintf(2, "output size=%d output=\n<%s>\n", total, output);
	    while (sent < total) {
		ret = write(sockfd, &output[sent], total - sent);
		dprintf(1, "written=%d bytes sent=%d total=%d\n", ret, sent, total);
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
		dprintf(1, "received bytes=%d data=<%s>\n", ret, result);
		sscanf(result, "HTTP/1.1 %d", &code);
		for (i = 13; i < 1024; i++)
		    if (result[i] == '\r')
			result[i] = 0;
		    dprintf(2, "http-code=%d text=%s [204=Success]\n", code, &result[13]);
		if (code != 204)
		    dprintf(2, "code %d -->%s<--\n", code, result);
	    }
	    close(sockfd);
	    sockfd = 0;
	    dprintf(1, "ic_push complete\n");
	} else {
	    dprintf(1, "socket create failed\n");
	}
    } else error("influx port is not set, bailing out");

    output[0] = 0;
    output_char = 0;
}
#endif

/********************************************************************************/

#ifdef JS
#include "jsengine.h"

enum INFLUX_PROPERTY_ID {
	INFLUX_PROPERTY_ID_HOST,
	INFLUX_PROPERTY_ID_PORT,
	INFLUX_PROPERTY_ID_DATABASE,
	INFLUX_PROPERTY_ID_USERNAME,
	INFLUX_PROPERTY_ID_PASSWORD
};

static JSBool influx_getprop(JSContext *cx, JSObject *obj, jsval id, jsval *rval) {
	influx_session_t *m;
	int prop_id;

	m = JS_GetPrivate(cx, obj);
	dprintf(dlevel,"m: %p\n", m);
	if (!m) {
		JS_ReportError(cx, "private is null!");
		return JS_FALSE;
	}

	dprintf(dlevel,"is_int: %d\n", JSVAL_IS_INT(id));
	if (JSVAL_IS_INT(id)) {
		prop_id = JSVAL_TO_INT(id);
		dprintf(dlevel,"prop_id: %d", prop_id);
		switch(prop_id) {
		case INFLUX_PROPERTY_ID_HOST:
			*rval = STRING_TO_JSVAL(JS_NewStringCopyZ(cx,m->config.host));
			break;
		case INFLUX_PROPERTY_ID_PORT:
			*rval = INT_TO_JSVAL(m->config.port);
			break;
		case INFLUX_PROPERTY_ID_DATABASE:
			*rval = STRING_TO_JSVAL(JS_NewStringCopyZ(cx,m->config.database));
			break;
		case INFLUX_PROPERTY_ID_USERNAME:
			*rval = STRING_TO_JSVAL(JS_NewStringCopyZ(cx,m->config.username));
			break;
		case INFLUX_PROPERTY_ID_PASSWORD:
			*rval = STRING_TO_JSVAL(JS_NewStringCopyZ(cx,m->config.password));
			break;
		default:
			break;
		}
	}
	return JS_TRUE;
}

static JSBool influx_setprop(JSContext *cx, JSObject *obj, jsval id, jsval *rval) {
	int prop_id;

	if (JSVAL_IS_INT(id)) {
		prop_id = JSVAL_TO_INT(id);
		dprintf(dlevel,"influx_setprop: prop_id: %d", prop_id);
		switch(prop_id) {
		default:
			break;
		}
	}
	return JS_TRUE;
}

static JSClass influx_class = {
	"influx",		/* Name */
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

static JSBool influx_write(JSContext *cx, JSObject *obj, uintN argc, jsval *argv, jsval *rval) {
//	dprintf(0,"writing...\n");
    	return JS_TRUE;
}

JSObject *JSInflux(JSContext *cx, void *priv) {
	JSPropertySpec influx_props[] = { 
		{ "host",		INFLUX_PROPERTY_ID_HOST,	JSPROP_ENUMERATE | JSPROP_READONLY },
		{ "port",		INFLUX_PROPERTY_ID_PORT,	JSPROP_ENUMERATE | JSPROP_READONLY },
		{ "database",		INFLUX_PROPERTY_ID_DATABASE,	JSPROP_ENUMERATE | JSPROP_READONLY },
		{ "username",		INFLUX_PROPERTY_ID_USERNAME,	JSPROP_ENUMERATE | JSPROP_READONLY },
		{ "password",		INFLUX_PROPERTY_ID_PASSWORD,	JSPROP_ENUMERATE | JSPROP_READONLY },
		{0}
	};
	JSFunctionSpec influx_funcs[] = {
		{ "write",influx_write,2 },
		{ 0 }
	};
	JSObject *obj;

	obj = JS_InitClass(cx, JS_GetGlobalObject(cx), 0, &influx_class, 0, 0, influx_props, influx_funcs, 0, 0);
	if (!obj) {
		JS_ReportError(cx,"unable to initialize influx class");
		return 0;
	}
//	JS_AliasProperty(cx, JS_GetGlobalObject(cx), "Influx", "influx");
	JS_SetPrivate(cx,obj,priv);
	return obj;
}

void influx_add_props(config_t *cp, influx_config_t *gconf, char *name, influx_config_t *conf) {
	config_property_t influx_props[] = {
		{ "host", DATA_TYPE_STRING, conf->host, sizeof(conf->host)-1, "localhost", 0,
                        0, 0, 0, 1, (char *[]){ "InfluxDB host" }, 0, 1, 0 },
		{ "port", DATA_TYPE_INT, &conf->port, 0, "8086", 0,
			0, 0, 0, 1, (char *[]){ "InfluxDB port" }, 0, 1, 0 },
		{ "database", DATA_TYPE_STRING, conf->database, sizeof(conf->database)-1, "", 0,
                        0, 0, 0, 1, (char *[]){ "InfluxDB database" }, 0, 1, 0 },
		{ "username", DATA_TYPE_STRING, conf->username, sizeof(conf->username)-1, "", 0,
                        0, 0, 0, 1, (char *[]){ "InfluxDB username" }, 0, 1, 0 },
		{ "password", DATA_TYPE_STRING, conf->password, sizeof(conf->password)-1, "", 0,
                        0, 0, 0, 1, (char *[]){ "InfluxDB password" }, 0, 1, 0 },
		{ 0 }
	};

	config_add_props(cp, "influx", influx_props, 0);
}

int influx_jsinit(JSEngine *js, influx_session_t *i) {
	return JS_EngineAddInitFunc(js, "influx", JSInflux, i);
}
#endif

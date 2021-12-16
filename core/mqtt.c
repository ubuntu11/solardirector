
/*
Copyright (c) 2021, Stephen P. Shoecraft
All rights reserved.

This source code is licensed under the BSD-style license found in the
LICENSE file in the root directory of this source tree.
*/

#include <string.h>
#include <MQTTClient.h>
#include <MQTTAsync.h>
#include <stdlib.h>
#include "mqtt.h"
#include "utils.h"
#include "debug.h"
#include "config.h"

#define TIMEOUT 10000L

#define dlevel 2

struct mqtt_session {
	mqtt_config_t config;
	MQTTClient c;
	int interval;
	mqtt_callback_t *cb;
	void *ctx;
	MQTTClient_SSLOptions *ssl_opts;
	list subs;
};
typedef struct mqtt_session mqtt_session_t;

#if 0
int mqtt_get_config(void *cfg, mqtt_config_t *conf) {
	struct cfg_proctab tab[] = {
		{ "mqtt", "broker", "Broker URL", DATA_TYPE_STRING,&conf->host,sizeof(conf->host), 0 },
		{ "mqtt", "clientid", "Client ID", DATA_TYPE_STRING,&conf->clientid,sizeof(conf->clientid), 0 },
		{ "mqtt", "username", "Broker username", DATA_TYPE_STRING,&conf->user,sizeof(conf->user), 0 },
		{ "mqtt", "password", "Broker password", DATA_TYPE_STRING,&conf->pass,sizeof(conf->pass), 0 },
		CFG_PROCTAB_END
	};

	cfg_get_tab(cfg,tab);
#ifdef DEBUG
	if (debug) cfg_disp_tab(tab,"MQTT",0);
#endif
	return 0;
}
#endif

#if 0
int mqtt_add_config(mqtt_session_t *m, void *cp) {
	config_property_t mqtt_props[] = {
		{ "broker", DATA_TYPE_STRING, m->config.host, sizeof(m->config.host)-1, "", 0, 0, 0, 0, 0, 0, 0, 1, 0 },
		{ "clientid", DATA_TYPE_STRING, m->config.clientid, sizeof(m->config.clientid)-1, "", 0, 0, 0, 0, 0, 0, 0, 1, 0 },
//		{ "username", DATA_TYPE_STRING, m->config.username, sizeof(m->config.username)-1, 0, "", 0, 0, 0, 0, 0, 0, 1, 0 },
//		{ "password", DATA_TYPE_STRING, m->config.password, sizeof(m->config.password)-1, 0, "", 0, 0, 0, 0, 0, 0, 1, 0 },
		{ 0 }
	};
	config_propdir_t mqtt_propdir[] = {
		{ "mqtt", mqtt_props, 0 },
		{ 0 }
	};

	dprintf(1,"m: %p\n", m);
	if (!m) return 1;

	config_add_props((config_t *)cp, mqtt_propdir);
	return 0;
}
#endif

void logProperties(MQTTProperties *props) {
	int i = 0;

	dprintf(1,"props->count: %d\n", props->count);
	for (i = 0; i < props->count; ++i) {
		int id = props->array[i].identifier;
		const char* name = MQTTPropertyName(id);
//		char* intformat = "Property name %s value %d";

		dprintf(1,"prop: id: %d, name: %s\n", id, name);
		switch (MQTTProperty_getType(id)) {
		case MQTTPROPERTY_TYPE_BYTE:
			dprintf(1, "value %d", props->array[i].value.byte);
			break;
		case MQTTPROPERTY_TYPE_TWO_BYTE_INTEGER:
			dprintf(1, "value %d", props->array[i].value.integer2);
			break;
		case MQTTPROPERTY_TYPE_FOUR_BYTE_INTEGER:
			dprintf(1, "value %d", props->array[i].value.integer4);
			break;
		case MQTTPROPERTY_TYPE_VARIABLE_BYTE_INTEGER:
			dprintf(1, "value %d", props->array[i].value.integer4);
			break;
		case MQTTPROPERTY_TYPE_BINARY_DATA:
		case MQTTPROPERTY_TYPE_UTF_8_ENCODED_STRING:
			dprintf(1, "value %s %.*s", name, props->array[i].value.data.len, props->array[i].value.data.data);
			break;
		case MQTTPROPERTY_CODE_USER_PROPERTY:
		case MQTTPROPERTY_TYPE_UTF_8_STRING_PAIR:
			dprintf(1, "key %.*s value %.*s", props->array[i].value.data.len, props->array[i].value.data.data, props->array[i].value.value.len, props->array[i].value.value.data);
			break;
		}
	}
}

static int mqtt_getmsg(void *ctx, char *topicName, int topicLen, MQTTClient_message *message) {
	char topic[256],*replyto;
	mqtt_session_t *s = ctx;
	int len;

	/* Ignore zero length messages */
	if (!message->payloadlen) goto mqtt_getmsg_skip;

	dprintf(4,"topicLen: %d\n", topicLen);
	if (topicLen) {
		len = topicLen > sizeof(topic)-1 ? sizeof(topic)-1 : topicLen;
		dprintf(1,"len: %d\n", len);
		memcpy(topic,topicName,len);
		topic[len] = 0;
	} else {
		topic[0] = 0;
		strncat(topic,topicName,sizeof(topic)-1);
	}
	dprintf(4,"topic: %s\n",topic);

//	logProperties(&message->properties);

	/* Do we have a replyto user property? */
	replyto = 0;
	dprintf(4,"hasProperty UP: %d\n", MQTTProperties_hasProperty(&message->properties, MQTTPROPERTY_CODE_USER_PROPERTY));
	if (MQTTProperties_hasProperty(&message->properties, MQTTPROPERTY_CODE_USER_PROPERTY)) {
		MQTTProperty *prop;

		prop = MQTTProperties_getProperty(&message->properties, MQTTPROPERTY_CODE_USER_PROPERTY);
		if (strncmp(prop->value.data.data,"replyto",prop->value.data.len) == 0) {
			replyto = prop->value.value.data;
		}
	}

	dprintf(4,"cb: %p\n", s->cb);
	if (s->cb) s->cb(s->ctx, topic, message->payload, message->payloadlen, replyto);

mqtt_getmsg_skip:
	MQTTClient_freeMessage(&message);
	MQTTClient_free(topicName);
	dprintf(4,"returning...\n");
	return 1;
}

int mqtt_newclient(struct mqtt_session *s, mqtt_config_t *config) {
	MQTTClient_createOptions opts = MQTTClient_createOptions_initializer;
	int rc;

	/* Set the config */
	s->config = *config;
	dprintf(1,"mqtt_config: host: %s, clientid: %s, username: %s, password: %s, lwt: %s\n",
		s->config.host, s->config.clientid, s->config.username, s->config.password, s->config.lwt_topic);

	opts.MQTTVersion = MQTTVERSION_5;
	rc = MQTTClient_createWithOptions(&s->c, s->config.host, s->config.clientid, MQTTCLIENT_PERSISTENCE_NONE, NULL, &opts);
	dprintf(2,"create rc: %d\n", rc);
	if (rc != MQTTCLIENT_SUCCESS) {
		log_error("MQTTClient_create failed");
		return 1;
	}

	/* Set callback BEFORE connect */
	rc = MQTTClient_setCallbacks(s->c, s, 0, mqtt_getmsg, 0);
	dprintf(2,"setcb rc: %d\n", rc);
	if (rc != MQTTCLIENT_SUCCESS) {
		log_error("MQTTClient_setCallbacks failed");
		return 1;
	}
#if 0
	if (strlen(s->config.ssl_key)) {
		s->ssl_opts = malloc(sizeof(*s->ssl_opts));
		if (s->ssl_opts) {
			s->ssl_opts = MQTTClient_SSLOptions_initializer;
			ssl_opts.keyStore = s->client_pem;
			ssl_opts.trustStore = s->ca_crt;
		}
	}
#endif
	return 0;
}

/* Create a new session */
struct mqtt_session *mqtt_new(mqtt_callback_t *cb, void *ctx) {
	mqtt_session_t *s;

	s = calloc(1,sizeof(*s));
	if (!s) {
		log_write(LOG_SYSERR,"mqtt_new: calloc");
		return 0;
	}
	s->cb = cb;
	s->ctx = ctx;
	s->subs = list_create();
	if (!s->subs) {
		free(s);
		return 0;
	}

	return s;
}

int mqtt_connect(mqtt_session_t *s, int interval) {
//	MQTTProperties props = MQTTProperties_initializer;
	MQTTClient_willOptions will_opts = MQTTClient_willOptions_initializer;
//	MQTTProperties willProps = MQTTProperties_initializer;
	MQTTClient_connectOptions conn_opts = MQTTClient_connectOptions_initializer5;
	MQTTResponse response = MQTTResponse_initializer;
	int rc;

	if (!s) return 1;

	dprintf(2,"interval: %d\n", interval);

	conn_opts.MQTTVersion = MQTTVERSION_5;
	conn_opts.keepAliveInterval = interval;
	conn_opts.connectTimeout = 3;
	conn_opts.cleanstart = 1;
	conn_opts.ssl = s->ssl_opts;
	dprintf(2,"lwt: %s\n", s->config.lwt_topic);
	if (strlen(s->config.lwt_topic)) {
		will_opts.topicName = s->config.lwt_topic;
		will_opts.message = "Offline";
		will_opts.retained = 1;
		will_opts.qos = 1;
		conn_opts.will = &will_opts;
	}

	s->interval = interval;
	if (strlen(s->config.username)) {
#if 0
	        conn_opts.username = opts.username;
		conn_opts.password = opts.password;
#endif
		conn_opts.username = s->config.username;
		if (strlen(s->config.password))
			conn_opts.password = s->config.password;
	}

	dprintf(2,"connecting...\n");
	response = MQTTClient_connect5(s->c, &conn_opts, 0, 0);
	rc = response.reasonCode;
	dprintf(2,"rc: %d\n", rc);
	if (rc != MQTTCLIENT_SUCCESS) {
		if (rc == 5) {
			log_error("MQTT: bad username or password\n");
			return 1;
		} else {
			char *p = (char *)MQTTReasonCode_toString(rc);
			log_error("MQTTClient_connect: %s\n",p ? p : "cant connect");
		}
		return 1;
	} else if (strlen(s->config.lwt_topic)) {
		mqtt_pub(s,s->config.lwt_topic,"Online",1,1);
	}
	return 0;
}

int mqtt_disconnect(mqtt_session_t *s, int timeout) {
	int rc;

	dprintf(2,"timeout: %d\n", timeout);

	if (!s) return 1;
	rc = MQTTClient_disconnect5(s->c, timeout, MQTTREASONCODE_SUCCESS, 0);
	dprintf(2,"rc: %d\n", rc);
	return rc;
}

int mqtt_reconnect(mqtt_session_t *s) {
	char *topic;

	dprintf(2,"reconnecting...\n");
	if (mqtt_disconnect(s,5)) return 1;
	if (mqtt_connect(s,20)) return 1;
	list_reset(s->subs);
	while((topic = list_get_next(s->subs)) != 0) mqtt_sub(s,topic);
	dprintf(2,"reconnect done!\n");
	return 0;
}

int mqtt_destroy(mqtt_session_t *s) {
	if (!s) return 1;
	MQTTClient_destroy(&s->c);
	free(s);
	return 0;
}

int mqtt_pub(mqtt_session_t *s, char *topic, char *message, int wait, int retain) {
	MQTTClient_message pubmsg = MQTTClient_message_initializer;
	MQTTClient_deliveryToken token;
	MQTTProperty property;
	MQTTResponse response = MQTTResponse_initializer;
//	options.WriteTimeout
	int rc,rt,retry;

	dprintf(2,"topic: %s, message: %s\n", topic, message);

	if (message) {
		pubmsg.payload = message;
		pubmsg.payloadlen = strlen(message);
	}

	/* Add a replyto user property */
	rt = 1;
	if (rt) {
		property.identifier = MQTTPROPERTY_CODE_USER_PROPERTY;
		property.value.data.data = "replyto";
		property.value.data.len = strlen(property.value.data.data);
		property.value.value.data = s->config.clientid;
		property.value.value.len = strlen(property.value.value.data);
		MQTTProperties_add(&pubmsg.properties, &property);
//		logProperties(&pubmsg.properties);
	}

	pubmsg.qos = 2;
	pubmsg.retained = retain;
	token = 0;
	retry = 0;
again:
	dprintf(2,"publishing...\n");
	response = MQTTClient_publishMessage5(s->c, topic, &pubmsg, &token);
	rc = response.reasonCode;
	dprintf(2,"rc: %d\n", rc);
	if (rc != MQTTCLIENT_SUCCESS) {
		dprintf(2,"publish error\n");
		if (!retry) {
			mqtt_reconnect(s);
			retry = 1;
			goto again;
		} else {
			return 1;
		}
	}
	MQTTResponse_free(response);
	if (wait) {
		dprintf(2,"waiting...\n");
		rc = MQTTClient_waitForCompletion(s->c, token, TIMEOUT);
		dprintf(2,"rc: %d\n", rc);
		if (rc != MQTTCLIENT_SUCCESS) return 1;
	}
	dprintf(2,"delivered message... token: %d\n",token);
	if (rt) MQTTProperties_free(&pubmsg.properties);
	return 0;
}

int mqtt_setcb(mqtt_session_t *s, void *ctx, MQTTClient_connectionLost *cl, MQTTClient_messageArrived *ma, MQTTClient_deliveryComplete *dc) {
	int rc;

	dprintf(2,"s: %p, ctx: %p, ma: %p\n", s, ctx, ma);
	rc = MQTTClient_setCallbacks(s->c, ctx, cl, ma, dc);
	dprintf(2,"rc: %d\n", rc);
	if (rc) log_write(LOG_ERROR,"MQTTClient_setCallbacks: rc: %d\n", rc);
	return rc;
}

static void mqtt_addsub(mqtt_session_t *s, char *topic) {
	char *p;

	list_reset(s->subs);
	while((p = list_get_next(s->subs)) != 0) {
		if (strcmp(p,topic) == 0) {
			return;
		}
	}
	list_add(s->subs,topic,strlen(topic)+1);
}

int mqtt_sub(mqtt_session_t *s, char *topic) {
	MQTTSubscribe_options opts = MQTTSubscribe_options_initializer;
	MQTTResponse response = MQTTResponse_initializer;
	int rc;

	opts.noLocal = 1;
	dprintf(2,"topic: %s\n", topic);
	response = MQTTClient_subscribe5(s->c, topic, 1, &opts, 0);
	rc = response.reasonCode;
	if (rc == MQTTREASONCODE_GRANTED_QOS_1) rc = 0;
	MQTTResponse_free(response);
	dprintf(2,"rc: %d\n", rc);
	if (rc == MQTTCLIENT_SUCCESS) mqtt_addsub(s,topic);
	return rc;
}

int mqtt_submany(mqtt_session_t *s, int count, char **topic) {
	int i,rc,*qos;

	dprintf(2,"s: %p, count: %d\n", s, count);
	qos = calloc(1,sizeof(int)*count);
	if (!qos) {
		log_write(LOG_SYSERR,"mqtt_submany: calloc");
		return 1;
	}
	for(i=0; i < count; i++) qos[i] = 1;
	rc = MQTTClient_subscribeMany(s->c, count, topic, qos);
	free(qos);
	dprintf(2,"rc: %d\n", rc);
	if (rc == MQTTCLIENT_SUCCESS) {
		for(i=0; i < count; i++)
			mqtt_addsub(s,topic[i]);
	}
	return rc;
}

int mqtt_unsub(mqtt_session_t *s, char *topic) {
	MQTTResponse response = MQTTResponse_initializer;
	int rc;

	dprintf(2,"s: %p, topic: %s\n", s, topic);
	response = MQTTClient_unsubscribe5(s->c, topic, 0);
	rc = response.reasonCode;
	dprintf(2,"rc: %d\n", rc);
	return rc;
}

int mqtt_unsubmany(mqtt_session_t *s, int count, char **topic) {
	int rc;

	dprintf(2,"s: %p, topic: %p\n", s, topic);
	rc = MQTTClient_unsubscribeMany(s->c, count, topic);
	dprintf(2,"rc: %d\n", rc);
	return rc;
}


#include "jsapi.h"

enum MQTT_PROPERTY_ID {
	MQTT_PROPERTY_ID_NONE,
	MQTT_PROPERTY_ID_HOST,
	MQTT_PROPERTY_ID_PORT,
	MQTT_PROPERTY_ID_CLIENTID,
	MQTT_PROPERTY_ID_USERNAME,
	MQTT_PROPERTY_ID_PASSWORD
};

#if 0
struct mqtt_session *mqtt_new(mqtt_config_t *conf, mqtt_callback_t *cb, void *ctx) {
	mqtt_session_t *s;
	s = calloc(1,sizeof(*s));
	if (!s) {
		log_write(LOG_SYSERR,"mqtt_new: calloc");
		return 0;
	}
	memcpy(&s->config,conf,sizeof(*conf));
	s->cb = cb;
	s->ctx = ctx;
	return 0;
}
int mqtt_connect(mqtt_session_t *s, int interval) { return 0; }
int mqtt_disconnect(mqtt_session_t *s, int timeout) { return 0; }
int mqtt_destroy(mqtt_session_t *s) { return 0; }
int mqtt_pub(mqtt_session_t *s, char *topic, char *message, int retain) { return 0; }
int mqtt_sub(mqtt_session_t *s, char *topic) { return 0; }
#endif

#ifdef JS
#include "jsengine.h"

static JSBool mqtt_getprop(JSContext *cx, JSObject *obj, jsval id, jsval *rval) {
	mqtt_session_t *m;
	int prop_id;

	m = JS_GetPrivate(cx, obj);
	dprintf(1,"m: %p\n", m);
	if (!m) {
		JS_ReportError(cx, "private is null!");
		return JS_FALSE;
	}

	dprintf(1,"is_int: %d\n", JSVAL_IS_INT(id));
	if (JSVAL_IS_INT(id)) {
		prop_id = JSVAL_TO_INT(id);
		dprintf(1,"prop_id: %d", prop_id);
		switch(prop_id) {
		case MQTT_PROPERTY_ID_HOST:
			*rval = STRING_TO_JSVAL(JS_NewStringCopyZ(cx,m->config.host));
			break;
		case MQTT_PROPERTY_ID_PORT:
			*rval = INT_TO_JSVAL(m->config.port);
			break;
		case MQTT_PROPERTY_ID_CLIENTID:
			*rval = STRING_TO_JSVAL(JS_NewStringCopyZ(cx,m->config.clientid));
			break;
		case MQTT_PROPERTY_ID_USERNAME:
			*rval = STRING_TO_JSVAL(JS_NewStringCopyZ(cx,m->config.username));
			break;
		case MQTT_PROPERTY_ID_PASSWORD:
			*rval = STRING_TO_JSVAL(JS_NewStringCopyZ(cx,m->config.password));
			break;
		default:
			break;
		}
	}
	return JS_TRUE;
}

static JSBool mqtt_setprop(JSContext *cx, JSObject *obj, jsval id, jsval *rval) {
	int prop_id;

	if (JSVAL_IS_INT(id)) {
		prop_id = JSVAL_TO_INT(id);
		dprintf(1,"mqtt_setprop: prop_id: %d", prop_id);
		switch(prop_id) {
		default:
			break;
		}
	}
	return JS_TRUE;
}

static JSClass mqtt_class = {
	"MQTT",			/* Name */
	JSCLASS_HAS_PRIVATE,	/* Flags */
	JS_PropertyStub,	/* addProperty */
	JS_PropertyStub,	/* delProperty */
	mqtt_getprop,		/* getProperty */
	mqtt_setprop,		/* setProperty */
	JS_EnumerateStub,	/* enumerate */
	JS_ResolveStub,		/* resolve */
	JS_ConvertStub,		/* convert */
	JS_FinalizeStub,	/* finalize */
	JSCLASS_NO_OPTIONAL_MEMBERS
};


static JSBool mqtt_jspub(JSContext *cx, JSObject *obj, uintN argc, jsval *argv, jsval *rval) {
	char *topic,*message;
	int retain;
	mqtt_session_t *m;

	m = JS_GetPrivate(cx, obj);
	dprintf(1,"m: %p\n", m);
	if (!m) {
		JS_ReportError(cx, "private is null!");
		return JS_FALSE;
	}

	dprintf(1,"argc: %d\n", argc);
	if (argc < 2) {
		JS_ReportError(cx, "not enough parms");
		return JS_FALSE;
	}

	topic = JS_GetStringBytes(JS_ValueToString(cx, argv[0]));
	message = JS_GetStringBytes(JS_ValueToString(cx, argv[1]));
	if (argc > 2)
		retain = JSVAL_TO_INT(argv[2]);
	else
		retain = 0;
	dprintf(1,"topic: %s, message: %s, retain: %d\n", topic, message, retain);
        mqtt_pub(m,topic,message,1,retain);
    	return JS_TRUE;
}

JSObject *JSMQTT(JSContext *cx, void *priv) {
	JSPropertySpec mqtt_props[] = { 
		{ "host",		MQTT_PROPERTY_ID_HOST,	JSPROP_ENUMERATE | JSPROP_READONLY },
		{ "port",		MQTT_PROPERTY_ID_PORT,	JSPROP_ENUMERATE | JSPROP_READONLY },
		{ "clientid",		MQTT_PROPERTY_ID_CLIENTID,	JSPROP_ENUMERATE | JSPROP_READONLY },
		{ "username",		MQTT_PROPERTY_ID_USERNAME,	JSPROP_ENUMERATE | JSPROP_READONLY },
		{0}
	};
	JSFunctionSpec mqtt_funcs[] = {
		{ "pub",mqtt_jspub,2 },
		{ 0 }
	};
	JSObject *obj;

	obj = JS_InitClass(cx, JS_GetGlobalObject(cx), 0, &mqtt_class, 0, 0, mqtt_props, mqtt_funcs, 0, 0);
	if (!obj) {
		JS_ReportError(cx,"unable to initialize mqtt class");
		return 0;
	}
	JS_AliasProperty(cx, JS_GetGlobalObject(cx), "MQTT", "mqtt");
	JS_SetPrivate(cx,obj,priv);
	return obj;
}

int mqtt_jsinit(JSEngine *js, mqtt_session_t *m) {
	return JS_EngineAddInitFunc(js, "mqtt", JSMQTT, m);
}
#endif

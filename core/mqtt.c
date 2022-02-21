
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
#include "config.h"
#include "uuid.h"

#define DEBUG_MQTT 1
#define dlevel 5

#ifdef DEBUG
#undef DEBUG
#endif
#define DEBUG DEBUG_MQTT
#include "debug.h"

#define TIMEOUT 10000L

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

int mqtt_parse_config(mqtt_config_t *conf, char *mqtt_info) {
	strncpy(conf->host,strele(0,",",mqtt_info),sizeof(conf->host)-1);
	strncpy(conf->clientid,strele(1,",",mqtt_info),sizeof(conf->clientid)-1);
	strncpy(conf->username,strele(2,",",mqtt_info),sizeof(conf->username)-1);
	strncpy(conf->password,strele(3,",",mqtt_info),sizeof(conf->password)-1);
	dprintf(1,"host: %s, clientid: %s, user: %s, pass: %s\n", conf->host, conf->clientid, conf->username, conf->password);
	return 0;
}

int mqtt_init(mqtt_session_t *m, mqtt_config_t *conf) {

	dprintf(1,"host: %s, clientid: %s, user: %s, pass: %s\n",
		conf->host, conf->clientid, conf->username, conf->password);
	if (!strlen(conf->host)) {
		log_write(LOG_WARNING,"mqtt host not specified, using localhost\n");
		strcpy(conf->host,"localhost");
	}

	/* Create a unique ID for MQTT */
	if (!strlen(conf->clientid)) {
		uint8_t uuid[16];

		dprintf(1,"gen'ing MQTT ClientID...\n");
		uuid_generate_random(uuid);
		my_uuid_unparse(uuid, conf->clientid);
		dprintf(4,"clientid: %s\n",conf->clientid);
	}

	/* Create a new client */
	if (mqtt_newclient(m, conf)) return 1;

	/* Connect to the server */
	if (mqtt_connect(m,20)) return 1;

	return 0;
}

void logProperties(MQTTProperties *props) {
	int i = 0;

	dprintf(dlevel,"props->count: %d\n", props->count);
	for (i = 0; i < props->count; ++i) {
		int id = props->array[i].identifier;
#if DEBUG
		const char* name = MQTTPropertyName(id);
#endif
//		char* intformat = "Property name %s value %d";

		dprintf(dlevel,"prop: id: %d, name: %s\n", id, name);
		switch (MQTTProperty_getType(id)) {
		case MQTTPROPERTY_TYPE_BYTE:
			dprintf(dlevel, "value %d", props->array[i].value.byte);
			break;
		case MQTTPROPERTY_TYPE_TWO_BYTE_INTEGER:
			dprintf(dlevel, "value %d", props->array[i].value.integer2);
			break;
		case MQTTPROPERTY_TYPE_FOUR_BYTE_INTEGER:
			dprintf(dlevel, "value %d", props->array[i].value.integer4);
			break;
		case MQTTPROPERTY_TYPE_VARIABLE_BYTE_INTEGER:
			dprintf(dlevel, "value %d", props->array[i].value.integer4);
			break;
		case MQTTPROPERTY_TYPE_BINARY_DATA:
		case MQTTPROPERTY_TYPE_UTF_8_ENCODED_STRING:
			dprintf(dlevel, "value %s %.*s", name, props->array[i].value.data.len, props->array[i].value.data.data);
			break;
		case MQTTPROPERTY_CODE_USER_PROPERTY:
		case MQTTPROPERTY_TYPE_UTF_8_STRING_PAIR:
			dprintf(dlevel, "key %.*s value %.*s", props->array[i].value.data.len, props->array[i].value.data.data, props->array[i].value.value.len, props->array[i].value.value.data);
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
		dprintf(dlevel,"len: %d\n", len);
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
	dprintf(dlevel,"mqtt_config: host: %s, clientid: %s, username: %s, password: %s, lwt: %s\n",
		s->config.host, s->config.clientid, s->config.username, s->config.password, s->config.lwt_topic);

	opts.MQTTVersion = MQTTVERSION_5;
	rc = MQTTClient_createWithOptions(&s->c, s->config.host, s->config.clientid, MQTTCLIENT_PERSISTENCE_NONE, NULL, &opts);
	dprintf(dlevel,"create rc: %d\n", rc);
	if (rc != MQTTCLIENT_SUCCESS) {
		log_error("MQTTClient_create failed");
		return 1;
	}

	/* Set callback BEFORE connect */
	rc = MQTTClient_setCallbacks(s->c, s, 0, mqtt_getmsg, 0);
	dprintf(dlevel,"setcb rc: %d\n", rc);
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

	dprintf(dlevel,"interval: %d\n", interval);

	conn_opts.MQTTVersion = MQTTVERSION_5;
	conn_opts.keepAliveInterval = interval;
	conn_opts.connectTimeout = 3;
	conn_opts.cleanstart = 1;
	conn_opts.ssl = s->ssl_opts;
	dprintf(dlevel,"lwt: %s\n", s->config.lwt_topic);
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

	dprintf(dlevel,"connecting...\n");
	response = MQTTClient_connect5(s->c, &conn_opts, 0, 0);
	rc = response.reasonCode;
	dprintf(dlevel,"rc: %d\n", rc);
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

	dprintf(dlevel,"timeout: %d\n", timeout);

	if (!s) return 1;
	rc = MQTTClient_disconnect5(s->c, timeout, MQTTREASONCODE_SUCCESS, 0);
	dprintf(dlevel,"rc: %d\n", rc);
	return rc;
}

int mqtt_reconnect(mqtt_session_t *s) {
	char *topic;

	dprintf(dlevel,"reconnecting...\n");
	if (mqtt_disconnect(s,5)) return 1;
	if (mqtt_connect(s,20)) return 1;
	list_reset(s->subs);
	while((topic = list_get_next(s->subs)) != 0) mqtt_sub(s,topic);
	dprintf(dlevel,"reconnect done!\n");
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

	dprintf(dlevel,"topic: %s, msglen: %d, retain: %d\n", topic, message ? strlen(message) : 0, retain);

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
	dprintf(dlevel,"publishing...\n");
	response = MQTTClient_publishMessage5(s->c, topic, &pubmsg, &token);
	rc = response.reasonCode;
	dprintf(dlevel,"rc: %d\n", rc);
	if (rc != MQTTCLIENT_SUCCESS) {
		dprintf(dlevel,"publish error\n");
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
		dprintf(dlevel,"waiting...\n");
		rc = MQTTClient_waitForCompletion(s->c, token, TIMEOUT);
		dprintf(dlevel,"rc: %d\n", rc);
		if (rc != MQTTCLIENT_SUCCESS) return 1;
	}
	dprintf(dlevel,"delivered message... token: %d\n",token);
	if (rt) MQTTProperties_free(&pubmsg.properties);
	return 0;
}

int mqtt_setcb(mqtt_session_t *s, void *ctx, MQTTClient_connectionLost *cl, MQTTClient_messageArrived *ma, MQTTClient_deliveryComplete *dc) {
	int rc;

	dprintf(dlevel,"s: %p, ctx: %p, ma: %p\n", s, ctx, ma);
	rc = MQTTClient_setCallbacks(s->c, ctx, cl, ma, dc);
	dprintf(dlevel,"rc: %d\n", rc);
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
	dprintf(dlevel,"topic: %s\n", topic);
	response = MQTTClient_subscribe5(s->c, topic, 1, &opts, 0);
	rc = response.reasonCode;
	if (rc == MQTTREASONCODE_GRANTED_QOS_1) rc = 0;
	MQTTResponse_free(response);
	dprintf(dlevel,"rc: %d\n", rc);
	if (rc == MQTTCLIENT_SUCCESS) mqtt_addsub(s,topic);
	return rc;
}

int mqtt_submany(mqtt_session_t *s, int count, char **topic) {
	int i,rc,*qos;

	dprintf(dlevel,"s: %p, count: %d\n", s, count);
	qos = calloc(1,sizeof(int)*count);
	if (!qos) {
		log_write(LOG_SYSERR,"mqtt_submany: calloc");
		return 1;
	}
	for(i=0; i < count; i++) qos[i] = 1;
	rc = MQTTClient_subscribeMany(s->c, count, topic, qos);
	free(qos);
	dprintf(dlevel,"rc: %d\n", rc);
	if (rc == MQTTCLIENT_SUCCESS) {
		for(i=0; i < count; i++)
			mqtt_addsub(s,topic[i]);
	}
	return rc;
}

int mqtt_unsub(mqtt_session_t *s, char *topic) {
	MQTTResponse response = MQTTResponse_initializer;
	int rc;

	dprintf(dlevel,"s: %p, topic: %s\n", s, topic);
	response = MQTTClient_unsubscribe5(s->c, topic, 0);
	rc = response.reasonCode;
	dprintf(dlevel,"rc: %d\n", rc);
	return rc;
}

int mqtt_unsubmany(mqtt_session_t *s, int count, char **topic) {
	int rc;

	dprintf(dlevel,"s: %p, topic: %p\n", s, topic);
	rc = MQTTClient_unsubscribeMany(s->c, count, topic);
	dprintf(dlevel,"rc: %d\n", rc);
	return rc;
}

#ifdef JS
#include "jsengine.h"

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

static JSBool mqtt_getprop(JSContext *cx, JSObject *obj, jsval id, jsval *rval) {
	mqtt_session_t *m;
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
		dprintf(dlevel,"mqtt_setprop: prop_id: %d", prop_id);
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

	dprintf(5,"argc: %d\n", argc);
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
	dprintf(5,"topic: %s, message: %s, retain: %d\n", topic, message, retain);
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

void mqtt_add_props(config_t *cp, mqtt_config_t *gconf, char *name, mqtt_config_t *conf) {
	config_property_t mqtt_props[] = {
		{ "host", DATA_TYPE_STRING, conf->host, sizeof(conf->host)-1, "localhost", 0,
                        0, 0, 0, 1, (char *[]){ "MQTT host" }, 0, 1, 0 },
		{ "port", DATA_TYPE_INT, &conf->port, 0, "1883", 0,
			0, 0, 0, 1, (char *[]){ "MQTT port" }, 0, 1, 0 },
		{ "clientid", DATA_TYPE_STRING, conf->clientid, sizeof(conf->clientid)-1, "", 0,
                        0, 0, 0, 1, (char *[]){ "MQTT clientid" }, 0, 1, 0 },
		{ "username", DATA_TYPE_STRING, conf->username, sizeof(conf->username)-1, "", 0,
                        0, 0, 0, 1, (char *[]){ "MQTT username" }, 0, 1, 0 },
		{ "password", DATA_TYPE_STRING, conf->password, sizeof(conf->password)-1, "", 0,
                        0, 0, 0, 1, (char *[]){ "MQTT password" }, 0, 1, 0 },
		{ 0 }
	};
	dprintf(1,"adding mqtt...\n");

	config_add_props(cp, "mqtt", mqtt_props, CONFIG_FLAG_NOID);

	/* Add the mqtt props to the instance config */
	{
		char *names[] = { "mqtt_host", "mqtt_port", "mqtt_clientid", "mqtt_username", "mqtt_password", 0 };
		config_section_t *s;
		config_property_t *p;
		int i;

		s = config_get_section(cp, name);
		if (!s) {
			log_error("%s section does not exist?!?\n", name);
			return;
		}
		for(i=0; names[i]; i++) {
			p = config_section_dup_property(s, &mqtt_props[i], names[i]);
			if (!p) continue;
			dprintf(1,"p->name: %s\n",p->name);
			config_section_add_property(cp, s, p, CONFIG_FLAG_NOID);
		}
	}
}

int mqtt_jsinit(JSEngine *js, mqtt_session_t *m) {
	return JS_EngineAddInitFunc(js, "mqtt", JSMQTT, m);
}
#endif

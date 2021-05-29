
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

#define TIMEOUT 10000L

#define MQTT 1

struct mqtt_session {
	mqtt_config_t config;
	MQTTClient c;
	int interval;
	mqtt_callback_t *cb;
	void *ctx;
	MQTTClient_SSLOptions *ssl_opts;
};
typedef struct mqtt_session mqtt_session_t;

int mqtt_parse_config(mqtt_config_t *conf, char *str) {
	strncat(conf->host,strele(0,",",str),sizeof(conf->host)-1);
	strncat(conf->clientid,strele(1,",",str),sizeof(conf->clientid)-1);
	strncat(conf->user,strele(2,",",str),sizeof(conf->user)-1);
	strncat(conf->pass,strele(3,",",str),sizeof(conf->pass)-1);
	return 0;
}

int mqtt_get_config(void *cfg, mqtt_config_t *conf) {
	struct cfg_proctab tab[] = {
		{ "mqtt", "broker", "Broker URL", DATA_TYPE_STRING,&conf->host,sizeof(conf->host), 0 },
		{ "mqtt", "clientid", "Client ID", DATA_TYPE_STRING,&conf->clientid,sizeof(conf->clientid), 0 },
		{ "mqtt", "username", "Broker username", DATA_TYPE_STRING,&conf->user,sizeof(conf->user), 0 },
		{ "mqtt", "password", "Broker password", DATA_TYPE_STRING,&conf->pass,sizeof(conf->pass), 0 },
		CFG_PROCTAB_END
	};

	cfg_get_tab(cfg,tab);
	if (debug) cfg_disp_tab(tab,"MQTT",0);
	return 0;
}

#if MQTT
static void mqtt_dc(void *ctx, char *cause) {
	dprintf(1,"MQTT disconnected! cause: %s\n", cause);
}

static int mqtt_getmsg(void *ctx, char *topicName, int topicLen, MQTTClient_message *message) {
	char topic[256];
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

	if (s->cb) s->cb(s->ctx, topic, message->payload, message->payloadlen);

mqtt_getmsg_skip:
	MQTTClient_freeMessage(&message);
	MQTTClient_free(topicName);
	dprintf(4,"returning...\n");
	return 1;
}

int mqtt_newclient(struct mqtt_session *s) {
	int rc;

	rc = MQTTClient_create(&s->c, s->config.host, s->config.clientid, MQTTCLIENT_PERSISTENCE_NONE, NULL);
	if (rc != MQTTCLIENT_SUCCESS) {
		lprintf(LOG_SYSERR,"MQTTClient_create");
		return 1;
	}
	return 0;
}

struct mqtt_session *mqtt_new(mqtt_config_t *conf, mqtt_callback_t *cb, void *ctx) {
	mqtt_session_t *s;
	int rc;

	dprintf(1,"mqtt_config: host: %s, clientid: %s, user: %s, pass: %s\n",
		conf->host, conf->clientid, conf->user, conf->pass);
	s = calloc(1,sizeof(*s));
	if (!s) {
		log_write(LOG_SYSERR,"mqtt_new: calloc");
		return 0;
	}
	memcpy(&s->config,conf,sizeof(*conf));
	if (mqtt_newclient(s)) {
		free(s);
		return 0;
	}
	s->cb = cb;
	s->ctx = ctx;
#if 0
	if (strlen(conf->ssl_key)) {
		s->ssl_opts = malloc(sizeof(*s->ssl_opts));
		if (s->ssl_opts) {
			s->ssl_opts = MQTTClient_SSLOptions_initializer;
			ssl_opts.keyStore = s->client_pem;
			ssl_opts.trustStore = s->ca_crt;
		}
	}
#endif

	rc = MQTTClient_setCallbacks(s->c, s, mqtt_dc, mqtt_getmsg, 0);
	dprintf(2,"rc: %d\n", rc);
	if (rc) {
		free(s);
		return 0;
	}
	return s;
}

int mqtt_connect(mqtt_session_t *s, int interval) {
	MQTTClient_connectOptions conn_opts = MQTTClient_connectOptions_initializer;
	MQTTClient_willOptions will_opts = MQTTClient_willOptions_initializer;
	int rc;

	if (!s) return 1;

	dprintf(2,"interval: %d\n", interval);

	if (!s) return 1;
	conn_opts.keepAliveInterval = interval;
	conn_opts.cleansession = 1;
	conn_opts.ssl = s->ssl_opts;
	if (strlen(s->config.lwt_topic)) {
		will_opts.topicName = s->config.lwt_topic;
		will_opts.message = "Offline";
		will_opts.retained = 1;
		will_opts.qos = 1;
		conn_opts.will = &will_opts;
	}

	s->interval = interval;
	if (strlen(s->config.user)) {
		conn_opts.username = s->config.user;
		if (strlen(s->config.pass))
			conn_opts.password = s->config.pass;
	}
	rc = MQTTClient_connect(s->c, &conn_opts);
	dprintf(2,"rc: %d\n", rc);
	if (rc != MQTTCLIENT_SUCCESS) {
		if (rc == 5) {
			printf("error: bad username or password\n");
			return 1;
		} else {
			char *p = (char *)MQTTReasonCode_toString(rc);
			lprintf(LOG_ERROR,"MQTTClient_connect: %s\n",p ? p : "cant connect");
		}
		return 1;
	} else if (strlen(s->config.lwt_topic)) {
		mqtt_pub(s,s->config.lwt_topic,"Online",1);
	}
	return 0;
}

int mqtt_disconnect(mqtt_session_t *s, int timeout) {
	int rc;

	dprintf(2,"timeout: %d\n", timeout);

	if (!s) return 1;
	rc = MQTTClient_disconnect(s->c, timeout);
	dprintf(2,"rc: %d\n", rc);
	return rc;
}

int mqtt_destroy(mqtt_session_t *s) {
	if (!s) return 1;
	MQTTClient_destroy(&s->c);
	free(s);
	return 0;
}


int mqtt_pub(mqtt_session_t *s, char *topic, char *message, int retain) {
	MQTTClient_message pubmsg = MQTTClient_message_initializer;
	MQTTClient_deliveryToken token;
	int rc;

	dprintf(2,"topic: %s, message: %s\n", topic, message);

	if (message) {
		pubmsg.payload = message;
		pubmsg.payloadlen = strlen(message);
	}

	pubmsg.qos = 2;
	pubmsg.retained = retain;
	token = 0;
	rc = MQTTClient_publishMessage(s->c, topic, &pubmsg, &token);
	dprintf(2,"rc: %d\n", rc);
	if (rc != MQTTCLIENT_SUCCESS) return 1;
	rc = MQTTClient_waitForCompletion(s->c, token, TIMEOUT);
	dprintf(2,"rc: %d\n", rc);
	if (rc != MQTTCLIENT_SUCCESS) return 1;
	dprintf(2,"delivered message... token: %d\n",token);
	return 0;
}

#if 0
int mqtt_send(mqtt_session_t *s, char *topic, char *message, int timeout) {
	MQTTClient_message pubmsg = MQTTClient_message_initializer;
	MQTTClient_deliveryToken token;
	int rc;

	dprintf(5,"message: %s, timeout: %d\n", message, timeout);

	if (message) {
		pubmsg.payload = message;
		pubmsg.payloadlen = strlen(message);
	}
	pubmsg.qos = 2;
	pubmsg.retained = 0;
	rc = MQTTClient_publishMessage(s->c, topic, &pubmsg, &token);
	dprintf(3,"rc: %d\n", rc);
	if (rc != MQTTCLIENT_SUCCESS) {
		lprintf(LOG_ERROR,"MQTTClient_publishMessage: %s\n",MQTTReasonCode_toString(rc));
		return 1;
	}
	rc = MQTTClient_waitForCompletion(s->c, token, timeout * 1000);
	dprintf(3,"rc: %d\n", rc);
	if (rc != MQTTCLIENT_SUCCESS) {
		lprintf(LOG_SYSERR,"MQTTClient_waitForCompletion");
		return 1;
	}
	dprintf(3,"delivered message.\n");
	return 0;
}
#endif

int mqtt_setcb(mqtt_session_t *s, void *ctx, MQTTClient_connectionLost *cl, MQTTClient_messageArrived *ma, MQTTClient_deliveryComplete *dc) {
	int rc;

	dprintf(2,"s: %p, ctx: %p, ma: %p\n", s, ctx, ma);
	rc = MQTTClient_setCallbacks(s->c, ctx, cl, ma, dc);
	dprintf(2,"rc: %d\n", rc);
	if (rc) log_write(LOG_ERROR,"MQTTClient_setCallbacks: rc: %d\n", rc);
	return rc;
}

int mqtt_sub(mqtt_session_t *s, char *topic) {
	int rc;

	dprintf(2,"s: %p, topic: %s\n", s, topic);
	rc = MQTTClient_subscribe(s->c, topic, 1);
	dprintf(2,"rc: %d\n", rc);
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
	return rc;
}

int mqtt_unsub(mqtt_session_t *s, char *topic) {
	int rc;

	dprintf(2,"s: %p, topic: %s\n", s, topic);
	rc = MQTTClient_unsubscribe(s->c, topic);
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

#if 0
int mqtt_fullsend(char *address, char *clientid, char *message, char *topic, char *user, char *pass) {
	mqtt_config_t config;
	mqtt_session_t *s;
	int rc;

	memset(&config,0,sizeof(config));
	strncat(config.host,address,sizeof(config.host)-1);
	strncat(config.clientid,clientid,sizeof(config.clientid)-1);
	strncat(config.user,user,sizeof(config.user)-1);
	strncat(config.pass,pass,sizeof(config.pass)-1);
	s = mqtt_new(&config,0,0);
	if (!s) return 1;

	rc = 1;
	if (mqtt_connect(s,20)) goto mqtt_send_error;
	if (mqtt_send(s,topic,message,10)) goto mqtt_send_error;
	rc = 0;
mqtt_send_error:
	mqtt_disconnect(s,10);
	mqtt_destroy(s);
	return rc;
}
#endif
#else
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

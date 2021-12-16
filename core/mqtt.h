
/*
Copyright (c) 2021, Stephen P. Shoecraft
All rights reserved.

This source code is licensed under the BSD-style license found in the
LICENSE file in the root directory of this source tree.
*/

#ifndef __SOLARD_MQTT_H
#define __SOLARD_MQTT_H

/* We use the paho mqtt.c library */
#include <MQTTClient.h>

struct mqtt_session;
typedef struct mqtt_session mqtt_session_t;

#define MQTT_HOST_LEN 64
#define MQTT_USER_LEN 32
#define MQTT_PASS_LEN 32
#define MQTT_CLIENTID_LEN 64
#define MQTT_TOPIC_LEN 128
#define MQTT_MAX_MESSAGE_SIZE 262144

struct mqtt_config {
	char host[MQTT_HOST_LEN];
	int port;
	char clientid[MQTT_CLIENTID_LEN];
	char username[MQTT_USER_LEN];
	char password[MQTT_PASS_LEN];
	char lwt_topic[MQTT_TOPIC_LEN];
};
typedef struct mqtt_config mqtt_config_t;

typedef void (mqtt_callback_t)(void *, char *, char *, int, char *);

int mqtt_add_config(mqtt_session_t *, void *);
int mqtt_parse_config(mqtt_config_t *conf, char *str);
int mqtt_get_config(void *,mqtt_config_t *);
struct mqtt_session *mqtt_new(mqtt_callback_t *, void *);
int mqtt_newclient(mqtt_session_t *, mqtt_config_t *);
int mqtt_connect(mqtt_session_t *s, int interval);
int mqtt_disconnect(mqtt_session_t *s, int timeout);
int mqtt_destroy(mqtt_session_t *s);
int mqtt_send(mqtt_session_t *s, char *topic, char *message, int timeout);
int mqtt_sub(mqtt_session_t *s, char *topic);
int mqtt_unsub(mqtt_session_t *s, char *topic);
int mqtt_setcb(mqtt_session_t *s, void *ctx, MQTTClient_connectionLost *cl, MQTTClient_messageArrived *ma, MQTTClient_deliveryComplete *dc);
int mqtt_pub(mqtt_session_t *s, char *topic, char *message, int wait, int retain);

int mqtt_dosend(mqtt_session_t *m, char *topic, char *message);
int mqtt_fullsend(char *address, char *clientid, char *message, char *topic, char *user, char *pass);

#ifdef JS
#include "jsapi.h"
#include "jsengine.h"
int mqtt_jsinit(JSEngine *, mqtt_session_t *);
#endif

#endif


#ifndef __SOLARD_MQTT_H
#define __SOLARD_MQTT_H

#include <MQTTClient.h>

#include "solard.h"

struct mqtt_session;
typedef struct mqtt_session mqtt_session_t;

struct mqtt_info {
	char host[64];
	char user[32];
	char pass[32];
	char clientid[64];
	char topic[128];
};
typedef struct mqtt_info mqtt_info_t;

struct mqtt_session *mqtt_new(char *address, char *clientid, char *topic);
int mqtt_connect(mqtt_session_t *s, int interval, char *, char *);
int mqtt_disconnect(mqtt_session_t *s, int timeout);
int mqtt_destroy(mqtt_session_t *s);
int mqtt_send(mqtt_session_t *s, char *message, int timeout);
int mqtt_sub(mqtt_session_t *s, char *topic);
int mqtt_setcb(mqtt_session_t *s, void *ctx, MQTTClient_connectionLost *cl, MQTTClient_messageArrived *ma, MQTTClient_deliveryComplete *dc);
int mqtt_pub(mqtt_session_t *s, char *topic, char *message, int retain);

int mqtt_fullsend(char *address, char *clientid, char *message, char *topic, char *user, char *pass);

#endif


#include <string.h>
#include <MQTTClient.h>
#include <MQTTAsync.h>
#include <stdlib.h>
#include "solard.h"
#include "mqtt.h"
#include "debug.h"

#define TIMEOUT 1000L

struct mqtt_session {
	char address[64];
	char clientid[32];
	char topic[128];
	MQTTClient c;
	MQTTClient_deliveryToken token;
	char user[32];
	char pass[32];
	int interval;
};
typedef struct mqtt_session mqtt_session_t;

struct mqtt_session *mqtt_new(char *address, char *clientid, char *topic) {
	struct mqtt_session *s;
	MQTTClient client;
	int rc;

	dprintf(2,"address: %s, clientid: %s, topic: %s\n", address, clientid, topic);
	rc = MQTTClient_create(&client, address, clientid, MQTTCLIENT_PERSISTENCE_NONE, NULL);
	if (rc != MQTTCLIENT_SUCCESS) {
		lprintf(LOG_SYSERR,"MQTTClient_create");
		return 0;
	}
	s = calloc(1,sizeof(*s));
	if (!s) {
		lprintf(LOG_SYSERR,"calloc");
		MQTTClient_destroy(&client);
		return 0;
	}
	strncat(s->address,address,sizeof(s->address)-1);
	strncat(s->clientid,clientid,sizeof(s->clientid)-1);
	if (topic) strncat(s->topic,topic,sizeof(s->topic)-1);
	s->c = client;

	return s;
}

int mqtt_connect(mqtt_session_t *s, int interval, char *user, char *pass) {
	MQTTClient_connectOptions conn_opts = MQTTClient_connectOptions_initializer;
	int rc;

	dprintf(2,"interval: %d, user: %s, pass: %s\n", interval, user, pass);

	if (!s) return 1;
	conn_opts.keepAliveInterval = interval;
	conn_opts.cleansession = 1;
	s->interval = interval;
	if (user && strlen(user)) {
		s->user[0] = 0;
		strncat(s->user,user,sizeof(user)-1);
		conn_opts.username = user;
		if (pass && strlen(pass)) {
			conn_opts.password = pass;
			s->pass[0] = 0;
			strncat(s->pass,pass,sizeof(pass)-1);
		}
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
	pubmsg.qos = 1;
	pubmsg.retained = retain;
	rc = MQTTClient_publishMessage(s->c, topic, &pubmsg, &token);
	dprintf(2,"rc: %d\n", rc);
	if (rc != MQTTCLIENT_SUCCESS) {
		lprintf(LOG_ERROR,"MQTTClient_publishMessage: %s\n",MQTTReasonCode_toString(rc));
		return 1;
	}
	rc = MQTTClient_waitForCompletion(s->c, token, 5000);
	dprintf(2,"rc: %d\n", rc);
	if (rc != MQTTCLIENT_SUCCESS) {
		lprintf(LOG_SYSERR,"MQTTClient_waitForCompletion");
		return 1;
	}
	dprintf(2,"delivered message.\n");
	return 0;
}

int mqtt_send(mqtt_session_t *s, char *message, int timeout) {
	MQTTClient_message pubmsg = MQTTClient_message_initializer;
	MQTTClient_deliveryToken token;
	int rc;

	dprintf(2,"message: %s, timeout: %d\n", message, timeout);

	if (message) {
		pubmsg.payload = message;
		pubmsg.payloadlen = strlen(message);
	}
	pubmsg.qos = 1;
	pubmsg.retained = 0;
	rc = MQTTClient_publishMessage(s->c, s->topic, &pubmsg, &token);
	dprintf(2,"rc: %d\n", rc);
	if (rc != MQTTCLIENT_SUCCESS) {
		lprintf(LOG_ERROR,"MQTTClient_publishMessage: %s\n",MQTTReasonCode_toString(rc));
		return 1;
	}
	rc = MQTTClient_waitForCompletion(s->c, token, timeout * 1000);
	dprintf(2,"rc: %d\n", rc);
	if (rc != MQTTCLIENT_SUCCESS) {
		lprintf(LOG_SYSERR,"MQTTClient_waitForCompletion");
		return 1;
	}
	dprintf(2,"delivered message.\n");
	return 0;
}

int mqtt_setcb(mqtt_session_t *s, void *ctx, MQTTClient_connectionLost *cl, MQTTClient_messageArrived *ma, MQTTClient_deliveryComplete *dc) {
	int rc;

	dprintf(2,"s: %p, ctx: %p, ma: %p\n", s, ctx, ma);
	rc = MQTTClient_setCallbacks(s->c, ctx, cl, ma, dc);
	dprintf(2,"rc: %d\n", rc);
	return rc;
}

int mqtt_sub(mqtt_session_t *s, char *topic) {
	int rc;

	dprintf(2,"s: %p, topic: %s\n", s, topic);
	rc = MQTTClient_subscribe(s->c, topic, 1);
	dprintf(2,"rc: %d\n", rc);
	return rc;
}

int mqtt_fullsend(char *address, char *clientid, char *message, char *topic, char *user, char *pass) {
	int rc = 1;
	mqtt_session_t *s = mqtt_new(address, clientid, topic);
	if (!s) return 1;
	if (mqtt_connect(s,20,user,pass)) goto mqtt_send_error;
	if (mqtt_send(s,message,10)) goto mqtt_send_error;
	rc = 0;
mqtt_send_error:
	mqtt_disconnect(s,10);
	mqtt_destroy(s);
	return rc;
}

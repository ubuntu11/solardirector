
/*
Copyright (c) 2021, Stephen P. Shoecraft
All rights reserved.

This source code is licensed under the BSD-style license found in the
LICENSE file in the root directory of this source tree.
*/

#include "common.h"

void solard_message_dump(solard_message_t *msg, int dlevel) {
	if (!msg) return;
	dprintf(dlevel,"msg: id: %s, name: %s, func: %s, replyto: %s, data(%d): %s\n",
		msg->id,msg->name,msg->func,msg->replyto,strlen(msg->data),msg->data);
}

/* Topic format:  SolarD/<id|name>/func (status,info,data,etc) */
solard_message_t *solard_getmsg(char *topic, char *message, int msglen, char *replyto) {
	solard_message_t newmsg,*msg;
	char *root;

	dprintf(4,"topic: %s\n", topic);

	/* All messages must start with SOLARD_TOPIC_ROOT */
	root = strele(0,"/",topic);
	dprintf(4,"root: %s\n", root);
	if (strcmp(root,SOLARD_TOPIC_ROOT) != 0) return 0;

	msg = &newmsg;
	memset(&newmsg,0,sizeof(newmsg));
	strncat(msg->id,strele(1,"/",topic),sizeof(msg->id)-1);
	strncat(msg->func,strele(2,"/",topic),sizeof(msg->func)-1);
	if (message && msglen) {
		memcpy(msg->data,message,msglen);
		msg->data[msglen] = 0;
	}
	if (replyto) strncat(msg->replyto,replyto,sizeof(msg->replyto)-1);
	if (debug >= 5) solard_message_dump(msg,5);
	return msg;
}

int solard_message_reply(mqtt_session_t *m, solard_message_t *msg, int status, char *message) {
	char *str;
	char topic[SOLARD_TOPIC_LEN];
	json_value_t *o;
	int r;

	/* If no replyto, dont bother */
	solard_message_dump(msg,1);
	if (!*msg->replyto) return 0;

	o = json_create_object();
	json_add_number(o,"status",status);
	json_add_string(o,"message",message);
	str = json_dumps(o,0);
	json_destroy(o);

	/* Replyto is expected to be the UUID of the sender */
	sprintf(topic,"%s/%s",SOLARD_TOPIC_ROOT,msg->replyto);
	dprintf(1,"topic: %s\n", topic);
	r = mqtt_pub(m,topic,str,1,0);
	free(str);
	return r;
}

void solard_ingest(list lp, int timeout) {
	time_t cur,upd;

	while(1) {
		time(&cur);
		upd = list_updated(lp);
		dprintf(4,"cur: %ld, upd: %ld, diff: %ld\n", cur, upd, cur-upd);
		if ((cur - upd) >= timeout) break;
		sleep(1);
	}
}

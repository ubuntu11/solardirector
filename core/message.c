
/*
Copyright (c) 2021, Stephen P. Shoecraft
All rights reserved.

This source code is licensed under the BSD-style license found in the
LICENSE file in the root directory of this source tree.
*/

#include "common.h"

void solard_message_dump(solard_message_t *msg, int dlevel) {
	if (!msg) return;
#if 0
	dprintf(dlevel,"msg: role: %s, name: %s, func: %s, action: %s, replyto: %s, data(%d): %s\n",
		msg->role,msg->name,msg->func,msg->action,msg->replyto,msg->data_len,msg->data);
#endif
	dprintf(dlevel,"msg: role: %s, name: %s, func: %s, replyto: %s, data(%d): %s\n",
		msg->role,msg->name,msg->func,msg->replyto,msg->data_len,msg->data);
}

/* SolarD/Role/Name/Func/Action/ .. ID? */

solard_message_t *solard_message_alloc(char *data, int data_len) {
	solard_message_t *msg;
	int size;

	size = sizeof(struct solard_message) + data_len + 1;
	dprintf(1,"data_len: %d, size: %d\n", data_len, size);
	msg = calloc(size,1);
	if (!msg) {
		log_write(LOG_SYSERR,"solard_message_alloc: calloc");
		return 0;
	}
	msg->size = size;
	msg->data = &msg->_d;
	if (data && data_len) {
		memcpy(msg->data,data,data_len);
		msg->data_len = data_len;
		msg->data[msg->data_len] = 0;
	}
	dprintf(1,"data(%d): %s\n", msg->data_len, msg->data);
	return msg;
}

void solard_message_free(solard_message_t *msg) {
	free(msg);
}

solard_message_t *solard_getmsg(char *topic, char *message, int msglen, char *replyto) {
	solard_message_t *msg;
	char *root;

	dprintf(4,"topic: %s\n", topic);

	/* All messages must start with SOLARD_TOPIC_ROOT */
	root = strele(0,"/",topic);
	dprintf(4,"root: %s\n", root);
	if (strcmp(root,SOLARD_TOPIC_ROOT) != 0) return 0;

	msg = solard_message_alloc(message,msglen);
	strncat(msg->role,strele(1,"/",topic),sizeof(msg->role)-1);
	strncat(msg->name,strele(2,"/",topic),sizeof(msg->name)-1);
	strncat(msg->func,strele(3,"/",topic),sizeof(msg->func)-1);
	if (replyto) strncat(msg->replyto,replyto,sizeof(msg->replyto)-1);
	solard_message_dump(msg,1);
//	dprintf(1,"data(%d): %s\n", msg->data_len, msg->data);
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

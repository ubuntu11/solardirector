
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

	printf("topic: %s, msglen: %d, replyto: %s\n", topic, msglen, replyto);

	/* All messages must start with SOLARD_TOPIC_ROOT */
	root = strele(0,"/",topic);
	dprintf(4,"root: %s\n", root);
	if (strcmp(root,SOLARD_TOPIC_ROOT) != 0) return 0;

	msg = &newmsg;
	memset(&newmsg,0,sizeof(newmsg));
	strncat(msg->id,strele(1,"/",topic),sizeof(msg->id)-1);
	strncat(msg->func,strele(2,"/",topic),sizeof(msg->func)-1);
	if (message && msglen) {
#if 0
		msg->data = malloc(msglen+1);
		if(!msg->data) {
			log_syserror("solard_getmsg: malloc(%d)\n", msglen+1);
			return 0;
		}
		strcpy(msg->data,message);
#else
		if (msglen >= SOLARD_MAX_PAYLOAD_SIZE) {
			log_error("+++++ msglen(%d) > SOLARD_MAX_PAYLOAD_SIZE(%d)\n",msglen, SOLARD_MAX_PAYLOAD_SIZE);
			*msg->data = 0;
		} else {
			memcpy(msg->data,message,msglen);
			msg->data[msglen] = 0;
		}
#endif
	}
	if (replyto) strncat(msg->replyto,replyto,sizeof(msg->replyto)-1);
	if (debug >= 5) solard_message_dump(msg,5);
	return msg;
}

int solard_message_reply(mqtt_session_t *m, solard_message_t *msg, int status, char *message) {
	char *str;
	char topic[SOLARD_TOPIC_LEN];
	json_object_t *o;
	int r;

	/* If no replyto, dont bother */
	solard_message_dump(msg,1);
	if (!*msg->replyto) return 0;

	o = json_create_object();
	json_object_set_number(o,"status",status);
	json_object_set_string(o,"message",message);
	str = json_dumps(json_object_get_value(o),0);
	json_destroy_object(o);

	/* Replyto is expected to be the UUID of the sender */
	sprintf(topic,"%s/%s",SOLARD_TOPIC_ROOT,msg->replyto);
	dprintf(1,"topic: %s\n", topic);
	r = mqtt_pub(m,topic,str,1,0);
	free(str);
	return r;
}

int solard_message_wait(list lp, int timeout) {
	time_t cur,upd;
	int count;

	while(1) {
		count = list_count(lp);
		if (count) break;
		if (timeout >= 0) {
			time(&cur);
			upd = list_updated(lp);
			dprintf(4,"cur: %ld, upd: %ld, diff: %ld\n", cur, upd, cur-upd);
			if ((cur - upd) >= timeout) break;
		}
		sleep(1);
	}
	return count;
}

solard_message_t *solard_message_wait_id(list lp, char *id, int timeout) {
	solard_message_t *msg;
	time_t cur,upd;

	dprintf(1,"==> want id: %s\n", id);

	while(1) {
		list_reset(lp);
		while((msg = list_get_next(lp)) != 0) {
			dprintf(1,"msg->id: %s\n", msg->id);
			if (strcmp(msg->replyto,id) == 0) {
				dprintf(1,"found\n");
				return msg;
			}
		}
		if (timeout >= 0) {
			time(&cur);
			upd = list_updated(lp);
			dprintf(4,"cur: %ld, upd: %ld, diff: %ld\n", cur, upd, cur-upd);
			if ((cur - upd) >= timeout) break;
		}
		sleep(1);
	}
	dprintf(1,"NOT found\n");
	return 0;
}

solard_message_t *solard_message_wait_target(list lp, char *target, int timeout) {
	solard_message_t *msg;
	time_t cur,upd;

	while(1) {
		list_reset(lp);
		while((msg = list_get_next(lp)) != 0) {
			if (strcmp(msg->name, target) == 0)
				return msg;
		}
		if (timeout >= 0) {
			time(&cur);
			upd = list_updated(lp);
			dprintf(4,"cur: %ld, upd: %ld, diff: %ld\n", cur, upd, cur-upd);
			if ((cur - upd) >= timeout) break;
		}
		sleep(1);
	}
	return 0;
}


/*
Copyright (c) 2021, Stephen P. Shoecraft
All rights reserved.

This source code is licensed under the BSD-style license found in the
LICENSE file in the root directory of this source tree.
*/

#include "common.h"

void solard_message_dump(solard_message_t *msg, int dlevel) {
	char temp[32];

	if (!msg) return;
	snprintf(temp,sizeof(temp)-1,"%s",msg->data);

	switch(msg->type) {
	case SOLARD_MESSAGE_TYPE_AGENT:
		dprintf(dlevel,"Agent msg: name: %s, func: %s, replyto: %s, data(%d): %s\n",
			msg->name,msg->func,msg->replyto,msg->size,temp);
		break;
	case SOLARD_MESSAGE_TYPE_CLIENT:
		dprintf(dlevel,"Client msg: id: %s, replyto: %s, data(%d): %s\n",
			msg->id,msg->replyto,msg->size,temp);
		break;
	default:
		dprintf(dlevel,"UNKNOWN: topic: %s, id/name: %s, func: %s, replyto: %s, data(%d): %s\n",
			msg->id,msg->name,msg->func,msg->replyto,msg->size,msg->data);
		break;
	}
}


/* Topic format:  SolarD/<id|name>/func (status,info,data,etc) */
int solard_getmsg(solard_message_t *msg, char *topic, char *message, int msglen, char *replyto) {
	char *root,*p;

	dprintf(4,"topic: %s, msglen: %d, replyto: %s\n", topic, msglen, replyto);

	/* Clear out dest before we start */
	memset(msg,0,sizeof(*msg));

	/* All messages must start with SOLARD_TOPIC_ROOT */
	root = strele(0,"/",topic);
	dprintf(4,"root: %s\n", root);
	if (strcmp(root,SOLARD_TOPIC_ROOT) != 0) return 1;
	strncpy(msg->topic,topic,sizeof(msg->topic)-1);

	/* Next must be agents or clients */
	p = strele(1,"/",topic);
	dprintf(4,"type: %s\n", p);
	if (strcmp(p,SOLARD_TOPIC_AGENTS) == 0) {
		msg->type = SOLARD_MESSAGE_TYPE_AGENT;
		/* Next is agent name */
		strncpy(msg->name,strele(2,"/",topic),sizeof(msg->name)-1);
		/* Next is agent func */
		strncpy(msg->func,strele(3,"/",topic),sizeof(msg->func)-1);
	} else if (strcmp(p,SOLARD_TOPIC_CLIENTS) == 0) {
		msg->type = SOLARD_MESSAGE_TYPE_CLIENT;
		/* Next is client id */
		strncpy(msg->id,strele(2,"/",topic),sizeof(msg->id)-1);
	} else {
		return 1;
	}

#if 0
	slashes=0;
	for(p=topic; *p; p++) {
		if (*p == '/') slashes++;
	}
//	dprintf(4,"slashes: %d\n", slashes);
	i = 1;
	strncpy(msg->id,strele(i++,"/",topic),sizeof(msg->id)-1);
//	dprintf(4,"id: %s\n", msg->id);
	if (slashes > 2) {
		r = sizeof(msg->id) - strlen(msg->id);
		strcat(msg->id,"/");
		strncat(msg->id,strele(i++,"/",topic),r);
		dprintf(4,"NEW id: %s\n", msg->id);
	}
	strncpy(msg->func,strele(i++,"/",topic),sizeof(msg->func)-1);
//	dprintf(4,"func: %s\n", msg->func);
#endif
	if (message && msglen) {
		if (msglen >= SOLARD_MAX_PAYLOAD_SIZE) {
			log_warning("solard_getmsg: msglen(%d) > %d",msglen,SOLARD_MAX_PAYLOAD_SIZE);
			msg->size = SOLARD_MAX_PAYLOAD_SIZE-1;
		} else {
			msg->size = msglen;
		}
		memcpy(msg->data,message,msg->size);
		msg->data[msg->size] = 0;
	}
	if (replyto) strncpy(msg->replyto,replyto,sizeof(msg->replyto)-1);
//	dprintf(4,"replyto: %s\n", msg->replyto);
	solard_message_dump(msg,0);
	return 0;
}

int solard_message_reply(mqtt_session_t *m, solard_message_t *msg, int status, char *message) {
	char *str;
	char topic[SOLARD_TOPIC_LEN];
	json_object_t *o;
	int r;

	/* If no replyto, dont bother */
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

int solard_message_delete(list lp, solard_message_t *msg) {
	return list_delete(lp, msg);
}

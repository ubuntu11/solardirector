
/*
Copyright (c) 2021, Stephen P. Shoecraft
All rights reserved.

This source code is licensed under the BSD-style license found in the
LICENSE file in the root directory of this source tree.
*/

#include "common.h"

void solard_message_dump(solard_message_t *msg, int dlevel) {
#if 0
        enum SOLARD_MESSAGE_TYPE type;
        char role[32];
        char name[64];
        char func[32];
        union {
                char action[32];
                char param[32];
        };
        char id[64];
        union {
                char data[262144];
                int status;
                char message[256];
        };
        int data_len;
#endif
	if (!msg) return;
	dprintf(dlevel,"msg: role: %s, name: %s, func: %s, action: %s, id: %s\n",
		msg->role,msg->name,msg->func,msg->action,msg->id);
//	dprintf(dlevel,"msg: type: %d, role: %s, name: %s, func: %s, action: %s, id: %s, data_len: %d\n",
//		msg->type,msg->role,msg->name,msg->func,msg->action,msg->data_len);
//	dprintf(dlevel,"msg: data_len: %d\n", msg->data_len);
}

/* SolarD/Role/Name/Func/Action/ .. ID? */

void solard_getmsg(void *ctx, char *topic, char *message, int msg_len) {
	list lp = ctx;
	solard_message_t newmsg;
	char *root;
	int i;

	dprintf(4,"topic: %s, message(%d): %s\n", topic, msg_len, message);

	/* All messages must start with SOLARD_TOPIC_ROOT */
	root = strele(0,"/",topic);
	dprintf(4,"root: %s\n", root);
	if (strcmp(root,SOLARD_TOPIC_ROOT) != 0) return;

	memset(&newmsg,0,sizeof(newmsg));

	memcpy(newmsg.data,message,msg_len);
	newmsg.data[msg_len] = 0;
	newmsg.data_len = msg_len;
	dprintf(4,"data: %s\n", newmsg.data);

	newmsg.role[0] = 0;
	strncat(newmsg.role,strele(1,"/",topic),sizeof(newmsg.role)-1);

//	i = strcmp(newmsg.role,SOLARD_ROLE_CONTROLLER)==0 ? 1 : 2;
	i = 2;
	dprintf(4,"role: %s, i: %d\n", newmsg.role, i);

	newmsg.name[0] = 0;
	strncat(newmsg.name,strele(i++,"/",topic),sizeof(newmsg.name)-1);
	newmsg.func[0] = 0;
	strncat(newmsg.func,strele(i++,"/",topic),sizeof(newmsg.func)-1);
	newmsg.action[0] = 0;
	strncat(newmsg.action,strele(i++,"/",topic),sizeof(newmsg.action)-1);
	newmsg.id[0] = 0;
	strncat(newmsg.id,strele(i++,"/",topic),sizeof(newmsg.id)-1);

       	/* Is this a Status Reply? */
	if (strcmp(newmsg.id,"Status") == 0) {
		newmsg.type = SOLARD_MESSAGE_STATUS;
		newmsg.id[0] = 0;
		strncat(newmsg.id,strele(i++,"/",topic),sizeof(newmsg.id)-1);
		dprintf(4,"new id: %s\n", newmsg.id);
	}
	dprintf(4,"newmsg: name: %s, func: %s, action: %s, id: %s, data(%d): %s\n",
		newmsg.name,newmsg.func,newmsg.action,newmsg.id,newmsg.data_len,newmsg.data);
	list_add(lp,&newmsg,sizeof(newmsg));
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

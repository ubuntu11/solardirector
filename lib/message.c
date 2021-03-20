
#include "common.h"

void solard_getmsg(void *ctx, char *topic, char *message, int msg_len) {
	list lp = ctx;
	solard_message_t newmsg;

	dprintf(1,"topic: %s, message(%d): %s\n", topic, msg_len, message);

	memset(&newmsg,0,sizeof(newmsg));

	memcpy(newmsg.data,message,msg_len);
	newmsg.data[msg_len] = 0;
	newmsg.data_len = msg_len;
	dprintf(4,"data: %s\n", newmsg.data);

	newmsg.name[0] = 0;
	strncat(newmsg.name,strele(3,"/",topic),sizeof(newmsg.name)-1);
	newmsg.func[0] = 0;
	strncat(newmsg.func,strele(4,"/",topic),sizeof(newmsg.func)-1);
	newmsg.action[0] = 0;
	strncat(newmsg.action,strele(5,"/",topic),sizeof(newmsg.action)-1);
	newmsg.id[0] = 0;
	strncat(newmsg.id,strele(6,"/",topic),sizeof(newmsg.id)-1);

       	/* Is this a Status Reply? */
	if (strcmp(newmsg.id,"Status") == 0) {
		newmsg.type = SOLARD_MESSAGE_STATUS;
		newmsg.id[0] = 0;
		strncat(newmsg.id,strele(7,"/",topic),sizeof(newmsg.id)-1);
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
		dprintf(1,"cur: %ld, upd: %ld, diff: %ld\n", cur, upd, cur-upd);
		if ((cur - upd) >= timeout) break;
		sleep(1);
	}
}

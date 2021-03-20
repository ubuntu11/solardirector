
#include "solard.h"
#include <pthread.h>

struct proto_info {
	sdagent_config_t *conf;
	proto_get_t *get;
	proto_set_t *set;
};

struct msginfo {
	char topic[128];
	char payload[1024];
};

int proto_callback(void *ctx, char *topicName, int topicLen, MQTTClient_message *message) {
	list mq = (list) ctx;
	struct msginfo newm;
	int len;

	if (topicLen) {
		len = topicLen > sizeof(newm.topic)-1 ? sizeof(newm.topic)-1 : topicLen;
		memcpy(newm.topic,topicName,len);
		newm.topic[len] = 0;
	} else {
		newm.topic[0] = 0;
		strncat(newm.topic,topicName,sizeof(newm.topic)-1);
	}
	dprintf(1,"topic: %s\n",newm.topic);

	len = message->payloadlen > sizeof(newm.payload)-1 ? sizeof(newm.payload)-1 : message->payloadlen;
	memcpy(newm.payload,message->payload,len);
	newm.payload[len] = 0;
	dprintf(1,"payload: %s\n", newm.payload);

	list_add(mq,&newm,sizeof(newm));

	MQTTClient_freeMessage(&message);
	MQTTClient_free(topicName);
	dprintf(1,"returning...\n");
	return 1;
}

int send_status(struct myctx *cp, char *op, char *name, int status, char *message) {
	char temp[64];
	sprintf(temp,"/test/Status/%s-%s",op,name);
	mqtt_pub(cp->m,temp,message,0);
	return 0;
}

void proto_process(struct proto_config *pc) {
	int c,r;
	char action[8],name[32],*p;
	struct config_data *dp;

	list_reset(pc->mq);
	while((m = list_get_next(pc->mq)) != 0) {
		c = 0;
		for(p=m->topic; *p; p++) {
			if (*p == '/') c++;
		}
		dprintf(1,"c: %d\n", c);
		strcpy(action,strele(c-1,"/",m->topic));
		strcpy(name,strele(c,"/",m->topic));
		dprintf(1,"action: %s, name: %s\n", action, name);
		r = 1;
		if (strcmp(action,"Get") == 0) {
			get(pc->conf,m->topic,m->name);
			dp = find_entry(cp->p,name);
			if (dp) {
				send_dp(cp,action,name,dp);
				r = 0;
			}
		} else if (strcmp(action,"Set") == 0) {
			dp = find_entry(cp->p,name);
			if (dp) {
				strcpy(dp->value,m->payload);
				send_dp(cp,action,name,dp);
				r = 0;
			}
		}
		if (r) send_status(cp,action,name,1,"not found");
	}
}

int proto_init(sdagent_config_t *conf, proto_get_t *get, proto_set_t *set) {
	struct proto_config *pc;
	struct msginfo *m;

	pc = calloc(1,sizeof(*pc));
	pc->conf = conf;
	pc->get = get;
	pc->set = set;
	if (mqtt_setcb(proto_callback,&pc,0,mycb,0)) return 1;
	if (mqtt_connect(myctx.m,20,0,0)) return 1;
	if (mqtt_sub(myctx.m,"/test/Config/Get/+")) return 1;
	if (mqtt_sub(myctx.m,"/test/Config/Set/+")) return 1;
	while(1) {
		list_reset(myctx.l);
		while((m = list_get_next(cp->l)) != 0) {
			dprintf(1,"m->topic: %s\n", m->topic);
			process(cp,m);
			list_delete(cp->l,m);
		}
		dprintf(1,"sleeping...\n");
		sleep(10);
	}
	return 0;
}

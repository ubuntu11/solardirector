
#include "agent.h"
#include <pthread.h>

struct msginfo {
	char topic[128];
	char payload[1024];
};

struct config_data {
        char name[32];
        char value[256];
};

struct myctx {
	mqtt_session_t *m;
	pthread_mutex_t lock;
	MQTTClient_deliveryToken token;
	list p;
	list l;
};

struct config_data *find_entry(list l, char *name) {
        struct config_data *dp;

	dprintf(1,"name: %s\n", name);
        list_reset(l);
        while((dp = list_get_next(l)) != 0) {
                dprintf(1,"dp: name %s\n, value: %s\n", dp->name, dp->value);
                if (strcmp(dp->name,name)==0) {
			dprintf(1,"found\n");
			return dp;
		}
        }
	dprintf(1,"NOT found\n");
        return 0;
}

int mycb(void *ctx, char *topicName, int topicLen, MQTTClient_message *message) {
	struct myctx *cp = ctx;
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

	list_add(cp->l,&newm,sizeof(newm));

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

int send_dp(struct myctx *cp, char *op, char *name, struct config_data *dp) {
	char temp[64];
	sprintf(temp,"/test/Config/%s",dp->name);
	mqtt_pub(cp->m,temp,dp->value,1);
	send_status(cp,op,name,0,"Success");
	return 0;
}

void process(struct myctx *cp, struct msginfo *m) {
	int c,r;
	char action[8],name[32],*p;
	struct config_data *dp;

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

void add_parm(struct myctx *cp, char *name, char *value) {
	struct config_data newcd;

	strcpy(newcd.name,name);
	strcpy(newcd.value,value);
	list_add(cp->p,&newcd,sizeof(newcd));
}

int main(void) {
	struct myctx myctx, *cp;
	struct msginfo *m;

	debug = 9;

	cp = &myctx;
	pthread_mutex_init(&myctx.lock, 0);
	myctx.l = list_create();
	myctx.p = list_create();

	add_parm(cp,"Voltage","54.0");
	add_parm(cp,"Current","10.0");
	add_parm(cp,"Load","1.0");

	myctx.m = mqtt_new("localhost","t1");
	if (mqtt_setcb(myctx.m,&myctx,0,mycb,0)) return 1;
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

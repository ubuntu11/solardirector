
/*
Copyright (c) 2021, Stephen P. Shoecraft
All rights reserved.

This source code is licensed under the BSD-style license found in the
LICENSE file in the root directory of this source tree.
*/

#include "cellmon.h"

solard_battery_t *find_pack_by_name(cellmon_config_t *conf, char *name) {
	solard_battery_t *pp;

	dprintf(1,"name: %s\n", name);
	list_reset(conf->packs);
	while((pp = list_get_next(conf->packs)) != 0) {
		if (strcmp(pp->name,name) == 0) {
			dprintf(1,"found!\n");
			return pp;
		}
	}
	dprintf(1,"NOT found!\n");
	return 0;
}

int sort_packs(void *i1, void *i2) {
	solard_battery_t *p1 = (solard_battery_t *)i1;
	solard_battery_t *p2 = (solard_battery_t *)i2;
	int val;

	dprintf(7,"p1: %s, p2: %s\n", p1->name, p2->name);
	val = strcmp(p1->name,p2->name);
	if (val < 0)
		val =  -1;
	else if (val > 0)
		val = 1;
	dprintf(7,"returning: %d\n", val);
	return val;

}

void getpack(cellmon_config_t *conf, char *name, char *data) {
	solard_battery_t bat,*pp = &bat;

//	dprintf(1,"getting pack: name: %s, data: %s\n", name, data);
	battery_from_json(&bat,data);
//	battery_dump(&bat,3);
	solard_set_state((&bat),BATTERY_STATE_UPDATED);
	time(&bat.last_update);
	if (!strlen(bat.name)) return;

	pp = find_pack_by_name(conf,bat.name);
	if (!pp) {
		dprintf(1,"adding pack...\n");
		list_add(conf->packs,&bat,sizeof(bat));
		dprintf(1,"sorting packs...\n");
		list_sort(conf->packs,sort_packs,0);
	} else {
		dprintf(1,"updating pack...\n");
		memcpy(pp,&bat,sizeof(bat));
	}
	return;
};


int main(int argc,char **argv) {
	cellmon_config_t *conf;
	client_agentinfo_t *ap;
	solard_message_t *msg;
	char configfile[256],topic[SOLARD_TOPIC_SIZE];
	char target[SOLARD_ROLE_LEN+SOLARD_NAME_LEN+2];
	char role[SOLARD_ROLE_LEN];
	char name[SOLARD_NAME_LEN];
	long start;
//	int web_flag;
	opt_proctab_t opts[] = {
		/* Spec, dest, type len, reqd, default val, have */
		{ "-t::|topic",&topic,DATA_TYPE_STRING,sizeof(topic)-1,0,"" },
//		{ "-w|web output",&web_flag,DATA_TYPE_BOOL,0,0,"false" },
		OPTS_END
	};
	register char *p;
#if TESTING
	char *args[] = { "t2", "-d", "2", "-c", "cellmon.conf" };
	#define nargs (sizeof(args)/sizeof(char *))
#endif

	find_config_file("cellmon.conf",configfile,sizeof(configfile)-1);
	dprintf(1,"configfile: %s\n", configfile);

	conf = calloc(1,sizeof(*conf));
	if (!conf) return 1;
	conf->c = client_init(argc,argv,opts,"cellmon",0,0);
	if (!conf->c) return 1;
	conf->packs = list_create();

	if (!strlen(topic)) {
		p = cfg_get_item(conf->c->cfg,"cellmon","topic");
		if (p) strncat(topic,p,sizeof(topic)-1);
		else strcpy(topic,"SolarD/Battery/+/Data");
	}

	if (mqtt_sub(conf->c->m,topic)) return 1;

	/* main loop */
	start = mem_used();
	while(1) {
		dprintf(1,"count: %d\n", list_count(conf->c->mq));
		list_reset(conf->c->mq);
		while((msg = list_get_next(conf->c->mq)) != 0) {
//			printf("msg: topic: %s, id: %s, len: %d, replyto: %s\n", msg->topic, msg->id, (int)strlen(msg->data), msg->replyto);
			*role = 0;
			p = strchr(msg->id,'/');
			if (p) {
				strncpy(target,msg->id,sizeof(target)-1);
				p = strchr(target,'/');
				if (p) {
					*p = 0;
					strncpy(role,target,sizeof(role)-1);
					strncpy(name,p+1,sizeof(name)-1);
				}
			}
			if (!*role) {
				strncpy(name,msg->id,sizeof(name)-1);
				ap = 0;
				if (strlen(msg->replyto)) ap = client_getagentbyid(conf->c,msg->replyto);
//				printf("ap: %p\n", ap);
				if (!ap) ap = client_getagentbyname(conf->c,msg->id);
//				printf("ap: %p\n", ap);
				if (!ap) continue;
				strncpy(role,ap->role,sizeof(role)-1);
			}
//			printf("name: %s, role: %s\n", name, role);
			if (strcmp(role,SOLARD_ROLE_BATTERY) != 0) continue;
//			printf("getting pack...\n");
			getpack(conf,msg->name,msg->data);
			list_delete(conf->c->mq,msg);
		}
		display(conf);
		dprintf(1,"used: %ld\n", mem_used() - start);
		sleep(1);
	}
	return 0;
}

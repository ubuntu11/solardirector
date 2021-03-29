
/*
Copyright (c) 2021, Stephen P. Shoecraft
All rights reserved.

This source code is licensed under the BSD-style license found in the
LICENSE file in the root directory of this source tree.
*/

#include "solard.h"
#include <sys/types.h>
#include <pwd.h>

char *find_config_file(char *name) {
	static char temp[1024];
	long uid;
	struct passwd *pw;

	if (access(name,R_OK)==0) return name;
	uid = getuid();
	pw = getpwuid(uid);
	if (pw) {
		sprintf(temp,"%s/etc/%s",pw->pw_dir,name);
		dprintf(1,"temp: %s\n", temp);
		if (access(temp,R_OK)==0) return temp;
	}

	sprintf(temp,"/etc/%s",name);
	if (access(temp,R_OK)==0) return temp;
	sprintf(temp,"/usr/local/etc/%s",name);
	if (access(temp,R_OK)==0) return temp;
	sprintf(temp,"/opt/mybmm/etc/%s",name);
	if (access(temp,R_OK)==0) return temp;
	return 0;
}

void getinv(solard_config_t *conf, char *name, char *data) {
	inverter_from_json(conf->inv,data);
}

solard_battery_t *find_pack_by_id(solard_config_t *conf, char *id) {
	solard_battery_t *pp;

	dprintf(1,"id: %s\n", id);
	list_reset(conf->packs);
	while((pp = list_get_next(conf->packs)) != 0) {
		if (strcmp(pp->id,id) == 0) {
			dprintf(1,"found!\n");
			return pp;
		}
	}
	dprintf(1,"NOT found!\n");
	return 0;
}

solard_battery_t *find_pack_by_name(solard_config_t *conf, char *name) {
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

void getpack(solard_config_t *conf, char *name, char *data) {
	solard_battery_t bat,*pp = &bat;

	battery_from_json(&bat,data);
	battery_dump(&bat,3);
	solard_set_state((&bat),BATTERY_STATE_UPDATED);
	time(&bat.last_update);

	pp = 0;
	if (strlen(bat.id)) pp = find_pack_by_id(conf,bat.id);
	if (!pp) pp = find_pack_by_name(conf,bat.name);
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
	char *args[] = { "t2", "-d", "2", "-c", "solard.conf" };
	#define nargs (sizeof(args)/sizeof(char *))
	solard_config_t *conf;
	solard_message_t *msg;
	char *configfile;
	long start;

	configfile = find_config_file("solard.conf");

	conf = calloc(1,sizeof(*conf));
	if (!conf) return 1;
	conf->c = client_init(nargs,args,0,"solard",configfile);
//	conf->c = client_init(argc,argv,0,"solard",configfile);
	if (!conf->c) return 1;
	conf->packs = list_create();

//	if (mqtt_sub(conf->c->m,"/SolarD/+/+/Info")) return 1;
	if (mqtt_sub(conf->c->m,"/SolarD/Battery/+/Data")) return 1;

	/* Publish our info */
	solard_pubinfo(conf);

	/* main loop */
	start = mem_used();
	while(1) {
//		dprintf(1,"count: %d\n", list_count(conf->c->messages));
		list_reset(conf->c->messages);
		while((msg = list_get_next(conf->c->messages)) != 0) {
			dprintf(1,"msg: role: %s, name: %s, func: %s, data_len: %d\n", msg->role, msg->name, msg->func, msg->data_len);
			if (strcmp(msg->role,AGENT_ROLE_INVERTER)==0)
				getinv(conf,msg->name,msg->data);
			else
			if (strcmp(msg->role,AGENT_ROLE_BATTERY)==0 && strcmp(msg->func,"Data")==0)
				getpack(conf,msg->name,msg->data);
			list_delete(conf->c->messages,msg);
		}
		dprintf(1,"used: %ld\n", mem_used() - start);
		sleep(1);
		/* Are we managing any processes? */
//		check_agents(conf);
	}
	return 0;
}

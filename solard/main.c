
/*
Copyright (c) 2021, Stephen P. Shoecraft
All rights reserved.

This source code is licensed under the BSD-style license found in the
LICENSE file in the root directory of this source tree.
*/

#include "solard.h"
#include "uuid.h"

int main(int argc,char **argv) {
	char *args[] = { "t2", "-d", "3", "-c", "solard.conf" };
	#define nargs (sizeof(args)/sizeof(char *))
	solard_config_t *conf;
	solard_message_t *msg;
	char configfile[256];
	long start;

	find_config_file("solard.conf",configfile,sizeof(configfile)-1);
	dprintf(1,"configfile: %s\n",configfile);

	conf = calloc(1,sizeof(*conf));
	if (!conf) return 1;
	conf->c = client_init(nargs,args,0,"solard",configfile);
//	conf->c = client_init(argc,argv,0,"solard",configfile);
	if (!conf->c) return 1;
	conf->packs = list_create();

//	if (mqtt_sub(conf->c->m,SOLARD_TOPIC_ROOT"/+/+/Info")) return 1;
	if (mqtt_sub(conf->c->m,SOLARD_TOPIC_ROOT"/"SOLARD_ROLE_INVERTER"/+/"SOLARD_FUNC_DATA)) return 1;
//	if (mqtt_sub(conf->c->m,SOLARD_TOPIC_ROOT"/"SOLARD_ROLE_BATTERY"/+/"SOLARD_FUNC_DATA)) return 1;
//	if (mqtt_sub(conf->c->m,SOLARD_TOPIC_ROOT"/"SOLARD_ROLE_CONSUMER"/+/"SOLARD_FUNC_DATA)) return 1;

	/* Publish our info */
	solard_pubinfo(conf);

	/* main loop */
	start = mem_used();
	while(1) {
//		dprintf(1,"count: %d\n", list_count(conf->c->messages));
		list_reset(conf->c->messages);
		while((msg = list_get_next(conf->c->messages)) != 0) {
			dprintf(1,"msg: role: %s, name: %s, func: %s, data_len: %d\n", msg->role, msg->name, msg->func, msg->data_len);
			if (strcmp(msg->role,SOLARD_ROLE_INVERTER)==0)
				getinv(conf,msg->name,msg->data);
			else
			if (strcmp(msg->role,SOLARD_ROLE_BATTERY)==0 && strcmp(msg->func,SOLARD_FUNC_DATA)==0)
				getpack(conf,msg->name,msg->data);
			list_delete(conf->c->messages,msg);
		}
		dprintf(8,"used: %ld\n", mem_used() - start);
		sleep(1);
		/* Are we managing any processes? */
//		check_agents(conf);
	}
	return 0;
}

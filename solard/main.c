
/*
Copyright (c) 2021, Stephen P. Shoecraft
All rights reserved.

This source code is licensed under the BSD-style license found in the
LICENSE file in the root directory of this source tree.
*/

#include "solard.h"
#include "uuid.h"

extern int logopts;

int main(int argc,char **argv) {
	solard_config_t *conf;
	solard_message_t *msg;
	char configfile[256];
	long start;
	time_t cur,last_check,diff;
#if 0
	char *args[] = { "t2", "-d", "0", "-c", "solard.conf" };
	#define nargs (sizeof(args)/sizeof(char *))
	argv = args;
	argc = nargs;
#endif

	find_config_file("solard.conf",configfile,sizeof(configfile)-1);
	dprintf(1,"configfile: %s\n",configfile);

	conf = calloc(1,sizeof(*conf));
	if (!conf) return 1;


	conf->c = client_init(argc,argv,0,"solard");
	if (!conf->c) return 1;
	conf->agents = list_create();
	conf->batteries = list_create();
	conf->inverters = list_create();
	conf->producers = list_create();
	conf->consumers = list_create();

	logopts |= LOG_TIME;

	solard_read_config(conf);
	if (conf->c->cfg && conf->c->cfg->dirty) solard_write_config(conf);

	if (mqtt_sub(conf->c->m,SOLARD_TOPIC_ROOT"/"SOLARD_ROLE_CONTROLLER"/"SOLARD_FUNC_CONFIG"/+/+")) return 1;
//	if (mqtt_sub(conf->c->m,SOLARD_TOPIC_ROOT"/+/+/Info")) return 1;
	if (mqtt_sub(conf->c->m,SOLARD_TOPIC_ROOT"/"SOLARD_ROLE_INVERTER"/+/"SOLARD_FUNC_DATA)) return 1;
	if (mqtt_sub(conf->c->m,SOLARD_TOPIC_ROOT"/"SOLARD_ROLE_BATTERY"/+/"SOLARD_FUNC_DATA)) return 1;
//	if (mqtt_sub(conf->c->m,SOLARD_TOPIC_ROOT"/"SOLARD_ROLE_PRODUCER"/+/"SOLARD_FUNC_DATA)) return 1;
//	if (mqtt_sub(conf->c->m,SOLARD_TOPIC_ROOT"/"SOLARD_ROLE_CONSUMER"/+/"SOLARD_FUNC_DATA)) return 1;

	/* Publish our info */
	solard_pubinfo(conf);

	/* main loop */
	start = mem_used();
	last_check = 0;
	while(1) {
//		dprintf(1,"count: %d\n", list_count(conf->c->messages));
		list_reset(conf->c->messages);
		while((msg = list_get_next(conf->c->messages)) != 0) {
//			dprintf(1,"msg: role: %s, name: %s, func: %s, data_len: %d\n", msg->role, msg->name, msg->func, msg->data_len);
			dprintf(1,"msg: type: %d, role: %s, name: %s, func: %s, action: %s\n",msg->type,msg->role,msg->name,msg->func,msg->action);
			if (strcmp(msg->role,SOLARD_ROLE_CONTROLLER)==0)
				solard_config(conf,msg);
			if (strcmp(msg->role,SOLARD_ROLE_INVERTER)==0)
				getinv(conf,msg->name,msg->data);
			else
			if (strcmp(msg->role,SOLARD_ROLE_BATTERY)==0 && strcmp(msg->func,SOLARD_FUNC_DATA)==0)
				getpack(conf,msg->name,msg->data);
			list_delete(conf->c->messages,msg);
		}
		dprintf(8,"used: %ld\n", mem_used() - start);
		sleep(1);

		/* Check agents */
		time(&cur);
		diff = cur - last_check;
		dprintf(3,"diff: %d, interval: %d\n", (int)diff, conf->interval);
		if (diff >= conf->interval) {
			check_agents(conf);
			time(&last_check);
		}

	}
	return 0;
}

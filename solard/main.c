
/*
Copyright (c) 2021, Stephen P. Shoecraft
All rights reserved.

This source code is licensed under the BSD-style license found in the
LICENSE file in the root directory of this source tree.
*/

#include "solard.h"
#include "uuid.h"

extern int logopts;

char *solard_version_string = "1.0";

void *solard_new(void *conf, void *driver, void *driver_handle) {
	solard_config_t *s;

	log_write(LOG_INFO,"SolarD version %s\n",solard_version_string);
	log_write(LOG_INFO,"Starting up...\n");

	s = calloc(1,sizeof(*s));
	if (!s) {
		perror("solard_new: calloc");
		return 0;
	}
	s->ap = conf;

	strcpy(s->name, s->ap->instance_name);

	dprintf(1,"returning: %p\n", s);
	return s;
}

int main(int argc,char **argv) {
	solard_module_t driver;
	solard_config_t *conf;
	solard_message_t *msg;
	solard_agent_t *ap;
	long start;
	time_t cur,last_check,diff;
#if 0
	char *args[] = { "t2", "-d", "0", "-c", "solard.conf" };
	#define nargs (sizeof(args)/sizeof(char *))
	argv = args;
	argc = nargs;
#endif

	/* Init agent */
	memset(&driver,0,sizeof(driver));
	driver.type = SOLARD_MODTYPE_CONTROLLER;
	driver.name = "solard";
	driver.new = solard_new;
	driver.info = solard_info;
	ap = agent_init(argc,argv,0,&driver);

        /* Get our session */
        conf = agent_get_driver_handle(ap);
	if (!conf) {
		log_error("unable to get driver handle");
		return 1;
	}

	conf->agents = list_create();
	conf->batteries = list_create();
	conf->inverters = list_create();
	conf->producers = list_create();
	conf->consumers = list_create();

	logopts |= LOG_TIME;

	solard_read_config(conf);
	if (conf->ap->cfg && cfg_is_dirty(conf->ap->cfg)) solard_write_config(conf);

//	if (mqtt_sub(ap->m,SOLARD_TOPIC_ROOT"/"SOLARD_ROLE_CONTROLLER"/"SOLARD_FUNC_CONFIG"/+/+")) return 1;
//	if (mqtt_sub(ap->m,SOLARD_TOPIC_ROOT"/+/+/Info")) return 1;
	if (mqtt_sub(ap->m,SOLARD_TOPIC_ROOT"/"SOLARD_ROLE_INVERTER"/+/"SOLARD_FUNC_DATA)) return 1;
	if (mqtt_sub(ap->m,SOLARD_TOPIC_ROOT"/"SOLARD_ROLE_BATTERY"/+/"SOLARD_FUNC_DATA)) return 1;
//	if (mqtt_sub(ap->m,SOLARD_TOPIC_ROOT"/"SOLARD_ROLE_PRODUCER"/+/"SOLARD_FUNC_DATA)) return 1;
//	if (mqtt_sub(ap->m,SOLARD_TOPIC_ROOT"/"SOLARD_ROLE_CONSUMER"/+/"SOLARD_FUNC_DATA)) return 1;

	/* main loop */
	start = mem_used();
	last_check = 0;
	while(1) {
		dprintf(1,"count: %d\n", list_count(conf->ap->mq));
		list_reset(conf->ap->mq);
		while((msg = list_get_next(conf->ap->mq)) != 0) {
//			dprintf(1,"msg: role: %s, name: %s, func: %s, data_len: %d\n", msg->role, msg->name, msg->func, msg->data_len);
			dprintf(1,"msg: type: %d, role: %s, name: %s, func: %s, action: %s\n",msg->type,msg->role,msg->name,msg->func,msg->action);
			if (strcmp(msg->role,SOLARD_ROLE_CONTROLLER)==0)
				solard_config(conf,msg);
			if (strcmp(msg->role,SOLARD_ROLE_INVERTER)==0)
				getinv(conf,msg->name,msg->data);
			else
			if (strcmp(msg->role,SOLARD_ROLE_BATTERY)==0 && strcmp(msg->func,SOLARD_FUNC_DATA)==0)
				getpack(conf,msg->name,msg->data);
			list_delete(conf->ap->mq,msg);
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

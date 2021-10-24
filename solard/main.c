
/*
Copyright (c) 2021, Stephen P. Shoecraft
All rights reserved.

This source code is licensed under the BSD-style license found in the
LICENSE file in the root directory of this source tree.
*/

#include "solard.h"
#include "transports.h"
#include "roles.h"

#define TESTING 0

extern int logopts;

char *solard_version_string = "1.0";

static void *solard_new(void *conf, void *driver, void *driver_handle) {
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

#if 0
struct controller_session {
	solard_agent_t *ap;
	solard_driver_t *driver;
	void *driver_handle;
};
typedef struct controller_session controller_session_t;

static void *controller_new(void *conf, void *driver, void *driver_handle) {
	controller_session_t *s;
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
#endif

static int _check(void *ctx) {
	solard_config_t *conf = ctx;
	time_t cur,diff;

	/* Check agents */
	time(&cur);
	diff = cur - conf->last_check;
	dprintf(3,"diff: %d, interval: %d\n", (int)diff, conf->interval);
	if (diff >= conf->interval) {
		check_agents(conf);
		time(&conf->last_check);
	}
	dprintf(1,"mem used: %ld\n", mem_used() - conf->start);
	return 0;
}

static int solard_mh(void *ctx, solard_message_t *msg) {
	solard_config_t *conf = ctx;

	dprintf(1,"called!\n");
	if (strcmp(msg->role,SOLARD_ROLE_CONTROLLER)==0) solard_config(conf,SOLARD_CONFIG_MESSAGE,msg);
	else if (strcmp(msg->func,SOLARD_FUNC_DATA)==0) {
		if (strcmp(msg->role,SOLARD_ROLE_INVERTER)==0) getinv(conf,msg->name,msg->data);
		else if (strcmp(msg->role,SOLARD_ROLE_BATTERY)==0) getpack(conf,msg->name,msg->data);
//		else if (strcmp(msg->role,SOLARD_ROLE_CONSUMER)==0) getcon(conf,msg->name,msg->data);
//		else if (strcmp(msg->role,SOLARD_ROLE_PRODUCER)==0) getpro(conf,msg->name,msg->data);
	}
	dprintf(1,"mem used: %ld\n", mem_used() - conf->start);
	return 0;
}

int main(int argc,char **argv) {
	solard_driver_t driver;
	solard_config_t *conf;
//	solard_message_t *msg;
	solard_agent_t *ap;
//	long start;
//	time_t cur,last_check,diff;
#if TESTING
	char *args[] = { "solard", "-d", "6", "-c", "sdtest.conf" };
	argc = (sizeof(args)/sizeof(char *));
	argv = args;
#endif

	/* Init agent driver */
	memset(&driver,0,sizeof(driver));
	driver.type = SOLARD_DRIVER_AGENT;
	driver.name = "solard";
	driver.new = solard_new;
	driver.config = solard_config;

#if 0
	/* Init role driver */
	memset(&role,0,sizeof(role));
	role.type = SOLARD_DRIVER_ROLE;
	role.name = "Controller";
	role.new = contoller_new;
	role.config = controller_config;
#endif

	ap = agent_init(argc,argv,0,&driver,0,&controller_driver);
	dprintf(1,"ap: %p\n", ap);
	if (!ap) return 1;

	dprintf(1,"ap->role: %p\n", ap->role);
	if (ap->role->config(ap->role_handle,SOLARD_CONFIG_GET_HANDLE,&conf)) {
		log_error("unable to get driver handle from role");
		return 1;
	}

	/* Init JS */
	conf->rt = JS_NewRuntime(8L * 1024L * 1024L);
	if (!conf->rt) {
		log_error("unable to initialize JS runtime\n");
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
	if (mqtt_sub(ap->m,SOLARD_TOPIC_ROOT"/"SOLARD_ROLE_PRODUCER"/+/"SOLARD_FUNC_DATA)) return 1;
	if (mqtt_sub(ap->m,SOLARD_TOPIC_ROOT"/"SOLARD_ROLE_CONSUMER"/+/"SOLARD_FUNC_DATA)) return 1;

	conf->start = mem_used();

	/* This is called once/second */
	agent_set_callback(ap,_check,conf);

	/* Add our message processor to the stack */
	agent_add_msghandler(ap,solard_mh,conf);

#if 0
	/* main loop */
	start = mem_used();
	last_check = 0;
	while(1) {
		dprintf(4,"count: %d\n", list_count(conf->ap->mq));
		list_reset(conf->ap->mq);
		while((msg = list_get_next(conf->ap->mq)) != 0) {
			dprintf(4,"msg: type: %d, role: %s, name: %s, func: %s, action: %s\n",msg->type,msg->role,msg->name,msg->func,msg->action);
			if (strcmp(msg->role,SOLARD_ROLE_CONTROLLER)==0) solard_config(conf,SOLARD_CONFIG_MESSAGE,msg);
			else if (strcmp(msg->func,SOLARD_FUNC_DATA)==0) {
				if (strcmp(msg->role,SOLARD_ROLE_INVERTER)==0) getinv(conf,msg->name,msg->data);
				else if (strcmp(msg->role,SOLARD_ROLE_BATTERY)==0) getpack(conf,msg->name,msg->data);
//				else if (strcmp(msg->role,SOLARD_ROLE_CONSUMER)==0) getcon(conf,msg->name,msg->data);
//				else if (strcmp(msg->role,SOLARD_ROLE_PRODUCER)==0) getpro(conf,msg->name,msg->data);
			}
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
#endif
	agent_run(ap);
	return 0;
}

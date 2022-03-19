
/*
Copyright (c) 2021, Stephen P. Shoecraft
All rights reserved.

This source code is licensed under the BSD-style license found in the
LICENSE file in the root directory of this source tree.
*/

#include "solard.h"
#include "transports.h"
#include "__sd_build.h"

#define TESTING 0

#if DEBUG_STARTUP
#define _ST_DEBUG LOG_DEBUG
#else
#define _ST_DEBUG 0
#endif

char *sd_version_string = "1.0-" STRINGIFY(__SD_BUILD);

solard_driver_t sd_driver;

int solard_read(void *handle, uint32_t *control, void *buf, int buflen) {
	solard_config_t *conf = handle;
	list agents = conf->c->agents;
	client_agentinfo_t *ap;
	char *p;

	/* Update the agents */
//	dprintf(0,"agent count: %d\n", list_count(agents));
	list_reset(agents);
	while((ap = list_get_next(agents)) != 0) {
		p = client_getagentrole(ap);
		if (!p) continue;
//		dprintf(0,"role: %s, name: %s, count: %d\n", p, ap->name, list_count(ap->mq));
		if (strcmp(p,SOLARD_ROLE_BATTERY) == 0) getpack(conf,ap);
		else if (strcmp(p,SOLARD_ROLE_BATTERY) == 0) getinv(conf,ap);
		else list_purge(ap->mq);
	}

	check_agents(conf);
	solard_monitor(conf);
	agent_start_script(conf->ap,"monitor.js");
	time(&conf->last_check);
	return 0;
}

int main(int argc,char **argv) {
	solard_config_t *conf;
	opt_proctab_t sd_opts[] = {
		OPTS_END
	};
#if TESTING
	char *args[] = { "solard", "-d", "0", "-e", "agent.script_dir=\".\"; agent.libdir=\"../../\";", "-c", "sdtest.conf" };
	argc = (sizeof(args)/sizeof(char *));
	argv = args;
#endif

//	log_open("solard",0,LOG_INFO|LOG_WARNING|LOG_ERROR|LOG_SYSERR|_ST_DEBUG);
//	log_open("solard",0,0xffffff);

	/* Init agent driver */
	memset(&sd_driver,0,sizeof(sd_driver));
	sd_driver.name = "solard";
	sd_driver.read = solard_read;
	sd_driver.config = solard_config;

	conf = calloc(1,sizeof(*conf));
	if (!conf) {
		log_syserror("calloc(%d)",sizeof(*conf));
		return 0;
	}
	dprintf(1,"conf: %p\n", conf);
	conf->agents = list_create();
	conf->batteries = list_create();
	conf->inverters = list_create();

	if (solard_agent_init(argc,argv,sd_opts,conf)) return 1;

//	conf->ap->read_interval = conf->ap->write_interval = -1;

	/* We're not only the president but a client too! */
	dprintf(1,"calling client_init...\n");
	conf->c = client_init(0,0,sd_version_string,0,sd_driver.name,0,0);
	dprintf(1,"c: %p\n", conf->c);
	if (!conf->c) return 1;

	agent_run(conf->ap);
	return 0;
}

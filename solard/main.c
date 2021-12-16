
/*
Copyright (c) 2021, Stephen P. Shoecraft
All rights reserved.

This source code is licensed under the BSD-style license found in the
LICENSE file in the root directory of this source tree.
*/

#include "solard.h"
#include "transports.h"

#define TESTING 1

#if DEBUG_STARTUP
#define _ST_DEBUG LOG_DEBUG
#else
#define _ST_DEBUG 0
#endif

char *solard_version_string = "1.0";
solard_driver_t sd_driver;

int main(int argc,char **argv) {
	solard_config_t *conf;
	opt_proctab_t sd_opts[] = {
		OPTS_END
	};
#if TESTING
	char *args[] = { "solard", "-d", "6", "-c", "sdtest.conf" };
	argc = (sizeof(args)/sizeof(char *));
	argv = args;
#endif

	debug = 4;

//	log_open("solard",0,LOG_INFO|LOG_WARNING|LOG_ERROR|LOG_SYSERR|_ST_DEBUG);
//	log_open("solard",0,0xffffff);

	/* Init agent driver */
	memset(&sd_driver,0,sizeof(sd_driver));
	sd_driver.type = SOLARD_DRIVER_AGENT;
	sd_driver.name = "solard";
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

	if (mqtt_sub(conf->ap->m,SOLARD_TOPIC_ROOT"/"SOLARD_ROLE_BATTERY"/+/"SOLARD_FUNC_DATA)) return 1;
	if (mqtt_sub(conf->ap->m,SOLARD_TOPIC_ROOT"/"SOLARD_ROLE_INVERTER"/+/"SOLARD_FUNC_DATA)) return 1;

//	if (conf->ap->cfg && cfg_is_dirty(conf->ap->cfg)) solard_write_config(conf);

	conf->start = mem_used();

	/* This is called once/second */
//	agent_set_callback(ap,solard_cb,conf);

//	config_set_filename(conf->ap->cp, "sdtest.json", 0);
//	config_write(conf->ap->cp);
//	return 0;

//	agent_run_script(conf->ap,"test.js");
//	return 0;
	agent_run(conf->ap);
	return 0;
}

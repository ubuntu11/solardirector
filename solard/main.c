
/*
Copyright (c) 2021, Stephen P. Shoecraft
All rights reserved.

This source code is licensed under the BSD-style license found in the
LICENSE file in the root directory of this source tree.
*/

#include "solard.h"
#include "transports.h"
#include "__sd_build.h"

#define TESTING 1
#define TESTLVL 5

#if DEBUG_STARTUP
#define _ST_DEBUG LOG_DEBUG
#else
#define _ST_DEBUG 0
#endif

char *sd_version_string = "1.0-" STRINGIFY(__SD_BUILD);

solard_driver_t sd_driver;

int solard_read(void *handle, uint32_t *control, void *buf, int buflen) {
	solard_instance_t *sd = handle;
	solard_agentinfo_t *info;
//	char *p;

        dprintf(1,"checking agents... count: %d\n", list_count(sd->agents));
        list_reset(sd->agents);
        while((info = list_get_next(sd->agents)) != 0) agent_check(sd,info);

//	agent_start_script(sd->ap,"monitor.js");
	time(&sd->last_check);
	return 0;
}

int main(int argc,char **argv) {
	solard_instance_t *sd;
	opt_proctab_t sd_opts[] = {
		OPTS_END
	};
#if TESTING
	char *args[] = { "solard", "-d", STRINGIFY(TESTLVL), "-c", "sdtest.conf" };
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

	sd = calloc(sizeof(*sd),1);
	if (!sd) {
		log_syserror("calloc(%d)",sizeof(*sd));
		return 0;
	}
	dprintf(1,"sd: %p\n", sd);
	sd->names = list_create();
	sd->agents = list_create();

	if (solard_agent_init(argc,argv,sd_opts,sd)) return 1;

	sd->ap->interval = 15;

	agent_run(sd->ap);
	return 0;
}

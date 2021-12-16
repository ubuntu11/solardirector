
/*
Copyright (c) 2021, Stephen P. Shoecraft
All rights reserved.

This source code is licensed under the BSD-style license found in the
LICENSE file in the root directory of this source tree.
*/

#include "si.h"
#include <pthread.h>
#include "transports.h"
#include "roles.h"

#define TESTING 0
#define CONFIG_FROM_MQTT 0

char *si_agent_version_string = "1.0";

int main(int argc, char **argv) {
	solard_agent_t *ap;
//	si_session_t *s;
#if TESTING
	char *args[] = { "si", "-d", "6", "-c", "sitest.conf" };
	argc = (sizeof(args)/sizeof(char *));
	argv = args;
#endif

        log_write(LOG_INFO,"SMA Sunny Island Agent version %s\n",si_agent_version_string);
        log_write(LOG_INFO,"Starting up...\n");

	ap = agent_init(argc,argv,0,&si_driver,si_transports,&inverter_driver);
	dprintf(1,"ap: %p\n",ap);
	if (!ap) return 1;

	/* Go */
	log_write(LOG_INFO,"Agent Running!\n");
	agent_run(ap);
	return 0;
}

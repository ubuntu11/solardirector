/*
Copyright (c) 2021, Stephen P. Shoecraft
All rights reserved.

This source code is licensed under the BSD-style license found in the
LICENSE file in the root directory of this source tree.
*/

#include "jk.h"
#include "transports.h"
#include <pthread.h>

#define TESTING 0

char *jk_version_string = "1.0";
extern solard_driver_t jk_driver;

int main(int argc, char **argv) {
	jk_session_t *s;
#if TESTING
	char *args[] = { "jk", "-d", "6", "-c", "jktest.conf" };
	argc = sizeof(args)/sizeof(char *);
	argv = args;
#endif

	/* Init the driver */
	s = jk_driver.new(0,0);
	if (!s) return 1;

	/* Agent init */
	if (jk_agent_init(s,argc,argv)) return 1;

	/* if using BT, open & close for read? */
	if (s->tp && strcmp(s->tp->name,"bt") == 0) s->ap->open_before_read = s->ap->close_after_read = 1;

	/* Main loop */
	log_write(LOG_INFO,"Running...\n");
	agent_run(s->ap);
	return 0;
}

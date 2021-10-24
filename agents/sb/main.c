/*
Copyright (c) 2021, Stephen P. Shoecraft
All rights reserved.

This source code is licensed under the BSD-style license found in the
LICENSE file in the root directory of this source tree.
*/

#include "sb.h"
#include "transports.h"
#include "roles.h"

#define TESTING 0

void *sb_new(void *conf, void *tp, void *tp_handle) {
	sb_session_t *s;

	s = calloc(1,sizeof(*s));
	if (!s) {
		perror("sb_new: calloc");
		return 0;
	}
	s->conf = conf;
	s->tp = tp;
	s->tp_handle = tp_handle;

//	strncat(s->name,s->conf->name,sizeof(s->name)-1);

	return s;
}

int sb_open(void *handle) {
	return 0;
}

int sb_close(void *handle) {
	return 0;
}

static solard_driver_t sb_driver = {
	SOLARD_DRIVER_AGENT,
	"sb",
	sb_new,				/* New */
	sb_open,			/* Open */
	sb_close,			/* Close */
	sb_read,			/* Read */
	0,
	sb_config			/* Config */
};

#if defined(__WIN32) || defined(__WIN64)
solard_driver_t *sb_transports[] = { 0 };
#else
solard_driver_t *sb_transports[] = { &http_driver, 0 };
#endif

int main(int argc, char **argv) {
	opt_proctab_t opts[] = {
		OPTS_END
	};
	solard_agent_t *ap;
#if TESTING
        char *args[] = { "sb", "-d", "6", "-c", "sbtest.conf" };
	argc = (sizeof(args)/sizeof(char *));
	argv = args;
#endif

	ap = agent_init(argc,argv,opts,&sb_driver,sb_transports,&inverter_driver);
	dprintf(1,"ap: %p\n",ap);
	if (!ap) return 1;
//	ap->interval = 5;
	agent_run(ap);
	return 0;
}

/*
Copyright (c) 2021, Stephen P. Shoecraft
All rights reserved.

This source code is licensed under the BSD-style license found in the
LICENSE file in the root directory of this source tree.
*/

#include "jbd.h"
#include "transports.h"
#include "roles.h"

#define TESTING 0

char *jbd_version_string = "1.0";

void *jbd_new(void *conf, void *transport, void *transport_handle) {
	jbd_session_t *s;

	log_write(LOG_INFO,"JBD Agent version %s\n",jbd_version_string);
	log_write(LOG_INFO,"Starting up...\n");

	s = calloc(1,sizeof(*s));
	if (!s) {
		perror("jbd_new: calloc");
		return 0;
	}
	s->ap = conf;
	s->tp = transport;
	s->tp_handle = transport_handle;

	/* Save a copy of the name */
	strcpy(s->name,((solard_agent_t *)conf)->instance_name);

	return s;
}

int jbd_open(void *handle) {
	jbd_session_t *s = handle;
	int r;

	dprintf(3,"s: %p\n", s);

	r = 0;
	if (!solard_check_state(s,BATTERY_STATE_OPEN)) {
		if (s->tp->open(s->tp_handle) == 0)
			solard_set_state(s,BATTERY_STATE_OPEN);
		else
			r = 1;
	}
	dprintf(3,"returning: %d\n", r);
	return r;
}

int jbd_close(void *handle) {
	jbd_session_t *s = handle;
	int r;

	dprintf(3,"s: %p\n", s);
	r = 0;
	if (solard_check_state(s,BATTERY_STATE_OPEN)) {
		if (s->tp->close(s->tp_handle) == 0)
			solard_clear_state(s,BATTERY_STATE_OPEN);
		else
			r = 1;
	}
	dprintf(3,"returning: %d\n", r);
	return r;
}

static solard_driver_t jbd_driver = {
	SOLARD_DRIVER_AGENT,
	"jbd",
	jbd_new,			/* New */
	jbd_open,			/* Open */
	jbd_close,			/* Close */
	jbd_read,			/* Read */
	0,				/* Write */
	jbd_config			/* Config */
};

#if defined(__WIN32) || defined(__WIN64)
static solard_driver_t *transports[] = { &ip_driver, &serial_driver, 0 };
#else
#ifdef BLUETOOTH
static solard_driver_t *transports[] = { &ip_driver, &bt_driver, &serial_driver, &can_driver, 0 };
#else
static solard_driver_t *transports[] = { &ip_driver, &serial_driver, &can_driver, 0 };
#endif
#endif

int main(int argc, char **argv) {
	solard_agent_t *ap;
	opt_proctab_t opts[] = {
		/* Spec, dest, type len, reqd, default val, have */
		OPTS_END
	};
#if TESTING
	char *args[] = { "jbd", "-d", "6", "-c", "jbdtest.conf" };
	argc = sizeof(args)/sizeof(char *);
	argv = args;
#endif

	/* Init the agent */
	ap = agent_init(argc,argv,opts,&jbd_driver,transports,&battery_driver);
	dprintf(1,"ap: %p\n",ap);
	if (!ap) return 1;

	/* Main loop */
	log_write(LOG_INFO,"Running...\n");
	agent_run(ap);
	return 0;
}

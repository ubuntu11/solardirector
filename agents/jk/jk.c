/*
Copyright (c) 2021, Stephen P. Shoecraft
All rights reserved.

This source code is licensed under the BSD-style license found in the
LICENSE file in the root directory of this source tree.
*/

#include "jk.h"
#include "transports.h"

#define TESTING 0

void *jk_new(void *conf, void *driver, void *driver_handle) {
	jk_session_t *s;

	s = calloc(1,sizeof(*s));
	if (!s) {
		perror("jk_new: calloc");
		return 0;
	}
	s->ap = conf;
	if (driver && driver_handle) {
		s->tp = driver;
		s->tp_handle = driver_handle;
	}

	/* Save a copy of our name */
	if (s->ap) strcpy(s->name,s->ap->instance_name);

	/* If it's bluetooth, reduce read interval to 15s */
	if (s->ap) if (strcmp(s->tp->name,"bt")==0) s->ap->read_interval = 15;

	return s;
}

int jk_open(void *handle) {
	jk_session_t *s = handle;
	int r;

	dprintf(3,"s: %p\n", s);
	if (!s) return 1;
	dprintf(3,"open: %d\n", solard_check_state(s,JK_STATE_OPEN));

	r = 0;
	if (!solard_check_state(s,JK_STATE_OPEN)) {
		dprintf(1,"tp: %p\n", s->tp);
		dprintf(1,"tp->open: %p\n", s->tp->open);
		if (s->tp->open(s->tp_handle) == 0)
			solard_set_state(s,JK_STATE_OPEN);
		else
			r = 1;
	}
	dprintf(3,"returning: %d\n", r);
	return r;
}

int jk_close(void *handle) {
	jk_session_t *s = handle;
	int r;

	dprintf(3,"s: %p\n", s);
	if (!s) return 1;
	dprintf(3,"open: %d\n", solard_check_state(s,JK_STATE_OPEN));


	r = 0;
	/* If it's bluetooth, dont bother closing - it's a waste of time and stability */
	if (solard_check_state(s,JK_STATE_OPEN) && strcmp(s->tp->name,"bt") != 0) {
		if (s->tp->close(s->tp_handle) == 0)
			solard_clear_state(s,JK_STATE_OPEN);
		else
			r = 1;
	}
	dprintf(3,"returning: %d\n", r);
	return r;
}

solard_driver_t jk_driver = {
	SOLARD_DRIVER_AGENT,
	"jk",
	jk_new,				/* New */
	0,				/* Destroy */
	jk_open,			/* Open */
	jk_close,			/* Close */
	jk_read,			/* Read */
	0,				/* Write */
	jk_config			/* Config */
};

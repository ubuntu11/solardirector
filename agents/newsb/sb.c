/*
Copyright (c) 2021, Stephen P. Shoecraft
All rights reserved.

This source code is licensed under the BSD-style license found in the
LICENSE file in the root directory of this source tree.
*/

#include "sb.h"
#include "transports.h"

#define TESTING 0

void *sb_new(void *conf, void *driver, void *driver_handle) {
	sb_session_t *s;

	s = calloc(1,sizeof(*s));
	if (!s) {
		perror("sb_new: calloc");
		return 0;
	}
	s->ap = conf;
	if (driver && driver_handle) {
		s->tp = driver;
		s->tp_handle = driver_handle;
	}

	/* Save a copy of our name */
//	if (s->ap) strcpy(s->name,s->ap->instance_name);

	/* If it's bluetooth, reduce read interval to 15s */
	if (s->ap) if (strcmp(s->tp->name,"bt")==0) s->ap->read_interval = 15;

	return s;
}

int sb_open(void *handle) {
	sb_session_t *s = handle;
	int r;

	dprintf(3,"s: %p\n", s);
	if (!s) return 1;
	dprintf(3,"open: %d\n", solard_check_state(s,SB_STATE_OPEN));

	r = 0;
	if (!solard_check_state(s,SB_STATE_OPEN)) {
		dprintf(1,"tp: %p\n", s->tp);
		dprintf(1,"tp->open: %p\n", s->tp->open);
		if (s->tp->open(s->tp_handle) == 0)
			solard_set_state(s,SB_STATE_OPEN);
		else
			r = 1;
	}
	dprintf(3,"returning: %d\n", r);
	return r;
}

int sb_close(void *handle) {
	sb_session_t *s = handle;
	int r;

	dprintf(3,"s: %p\n", s);
	if (!s) return 1;
	dprintf(3,"open: %d\n", solard_check_state(s,SB_STATE_OPEN));


	r = 0;
	/* If it's bluetooth, dont bother closing - it's a waste of time and stability */
	if (solard_check_state(s,SB_STATE_OPEN) && strcmp(s->tp->name,"bt") != 0) {
		if (s->tp->close(s->tp_handle) == 0)
			solard_clear_state(s,SB_STATE_OPEN);
		else
			r = 1;
	}
	dprintf(3,"returning: %d\n", r);
	return r;
}

solard_driver_t sb_driver = {
	SOLARD_DRIVER_AGENT,
	"sb",
	sb_new,				/* New */
	0,				/* Destroy */
	sb_open,			/* Open */
	sb_close,			/* Close */
	sb_read,			/* Read */
	0,				/* Write */
	sb_config			/* Config */
};

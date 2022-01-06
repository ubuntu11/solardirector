
/*
Copyright (c) 2021, Stephen P. Shoecraft
All rights reserved.

This source code is licensed under the BSD-style license found in the
LICENSE file in the root directory of this source tree.
*/

#include "si.h"
#include "transports.h"

static void *si_new(void *uu, void *driver, void *driver_handle) {
	si_session_t *s;

	s = calloc(1,sizeof(*s));
	if (!s) {
		perror("si_new: calloc");
		return 0;
	}
	s->desc = list_create();
	if (driver) {
		s->can = driver;
		s->can_handle = driver_handle;
		si_can_set_reader(s);
	}

	dprintf(1,"returning: %p\n", s);
	return s;
}

static int si_destroy(void *h) {
	si_session_t *s = h;
	if (!s) return 1;
	/* Close and destroy transport */
	dprintf(1,"s->can: %p, s->can_handle: %p\n", s->can, s->can_handle);
	if (s->can && s->can_handle) {
		if (solard_check_state(s,SI_STATE_OPEN)) s->can->close(s->can_handle);
		s->can->destroy(s->can_handle);
	}
	free(s);
	return 0;
}

static int si_open(void *h) {
	si_session_t *s = h;
	int r;

	dprintf(3,"s: %p\n", s);

	r = 0;
	if (!solard_check_state(s,SI_STATE_OPEN)) {
		if (s->can->open(s->can_handle) == 0)
			solard_set_state(s,SI_STATE_OPEN);
		else
			r = 1;
	}
	dprintf(3,"returning: %d\n", r);
	return r;
}

static int si_close(void *handle) {
	si_session_t *s = handle;
	int r;

	dprintf(3,"s: %p\n", s);
	r = 0;
	if (solard_check_state(s,SI_STATE_OPEN)) {
		if (s->can->close(s->can_handle) == 0)
			solard_clear_state(s,SI_STATE_OPEN);
		else
			r = 1;
	}
	dprintf(3,"returning: %d\n", r);
	return r;
}

static int si_read(void *handle, void *buf, int buflen) {
	si_session_t *s = handle;
	return si_read_data(s);
}

static int si_write(void *handle, void *buffer, int len) {
	si_session_t *s = handle;

	return(s->can_connected ? si_can_write_data(s) : 0);
}

solard_driver_t si_driver = {
	SOLARD_DRIVER_AGENT,
	"si",
	si_new,		/* New */
	si_destroy,	/* Free */
	si_open,	/* Open */
	si_close,	/* Close */
	si_read,	/* Read */
	si_write,	/* Write */
	si_config,	/* Config */
};


/*
Copyright (c) 2021, Stephen P. Shoecraft
All rights reserved.

This source code is licensed under the BSD-style license found in the
LICENSE file in the root directory of this source tree.
*/

#include "si.h"
#include "transports.h"

solard_driver_t *si_transports[] = { &can_driver, &serial_driver, &rdev_driver, 0 };

static void *si_new(void *conf, void *driver, void *driver_handle) {
	si_session_t *s;

	s = calloc(1,sizeof(*s));
	if (!s) {
		perror("si_new: calloc");
		return 0;
	}
	s->ap = conf;
	s->desc = list_create();
	if (driver) {
		s->can = driver;
		s->can_handle = driver_handle;
	} else {
		return s;
	}

	/* Start background recv thread */
	dprintf(1,"driver name: %s\n", s->can->name);
	if (strcmp(s->can->name,"can") == 0) {
		pthread_attr_t attr;
		
		/* Create a detached thread */
		if (pthread_attr_init(&attr)) {
			perror("si_new: pthread_attr_init");
			goto si_new_error;
		}
		if (pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_DETACHED)) {
			perror("si_new: pthread_attr_setdetachstate");
			goto si_new_error;
		}
		solard_set_state(s,SI_STATE_RUNNING);
		if (pthread_create(&s->th,&attr,&si_recv_thread,s)) {
			perror("si_new: pthread_create");
			goto si_new_error;
		}
		dprintf(1,"setting func to local data\n");
		s->can_read = si_get_local_can_data;
	} else {
		dprintf(1,"setting func to remote data\n");
		s->can_read = si_get_remote_can_data;
	}

	dprintf(1,"returning: %p\n", s);
	return s;
si_new_error:
	free(s);
	return 0;
}

static int si_open(si_session_t *s) {
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
	/* We only close when shutdown */
	return 0;
#if 0
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
#endif
}

solard_driver_t si_driver = {
	SOLARD_DRIVER_AGENT,
	"si",
	si_new,				/* New */
	(solard_driver_open_t)si_open,			/* Open */
	(solard_driver_close_t)si_close,			/* Close */
	(solard_driver_read_t)si_read,			/* Read */
	(solard_driver_write_t)si_write,			/* Write */
	si_config,			/* Config */
};

int si_startstop(si_session_t *s, int op) {
	int retries;
	uint8_t data[8],b;

	dprintf(1,"op: %d\n", op);

	b = (op ? 1 : 2);
	dprintf(1,"b: %d\n", b);
	retries=10;
	while(retries--) {
		dprintf(1,"writing...\n");
		if (si_can_write(s,0x35c,&b,1) < 0) return 1;
		dprintf(1,"reading...\n");
		if (s->can_read(s,0x307,data,8) == 0) {
			if (debug >= 3) bindump("data",data,8);
			dprintf(1,"*** data[3] & 2: %d\n", data[3] & 0x0002);
			if (op) {
				if (data[3] & 0x0002) return 0;
			} else {
				if ((data[3] & 0x0002) == 0) return 0;
			}
		}
		sleep(1);
	}
	if (retries < 0) printf("start failed.\n");
	return (retries < 0 ? 1 : 0);
}

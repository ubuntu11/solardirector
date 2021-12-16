
/*
Copyright (c) 2021, Stephen P. Shoecraft
All rights reserved.

This source code is licensed under the BSD-style license found in the
LICENSE file in the root directory of this source tree.
*/

#include "si.h"
#include "transports.h"

solard_driver_t *si_transports[] = { &can_driver, &serial_driver, &rdev_driver, 0 };

static int _can_init(si_session_t *s) {
	/* Start background recv thread */
	dprintf(1,"driver name: %s\n", s->can->name);
	if (strcmp(s->can->name,"can") == 0) {
		pthread_attr_t attr;
		
		/* Create a detached thread */
		if (pthread_attr_init(&attr)) {
			sprintf(s->errmsg,"pthread_attr_init: %s",strerror(errno));
			goto _can_init_error;
		}
		if (pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_DETACHED)) {
			sprintf(s->errmsg,"pthread_attr_setdetachstate: %s",strerror(errno));
			goto _can_init_error;
		}
		solard_set_state(s,SI_STATE_RUNNING);
		if (pthread_create(&s->th,&attr,&si_recv_thread,s)) {
			sprintf(s->errmsg,"pthread_create: %s",strerror(errno));
			goto _can_init_error;
		}
		pthread_attr_destroy(&attr);
		dprintf(1,"setting func to local data\n");
		s->can_read = si_get_local_can_data;
	} else {
		dprintf(1,"setting func to remote data\n");
		s->can_read = si_get_remote_can_data;
	}
	return 0;
_can_init_error:
	s->can_read = si_get_remote_can_data;
	return 1;
}

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
		if (_can_init(s)) goto si_new_error;
	}

	dprintf(1,"returning: %p\n", s);
	return s;
si_new_error:
	free(s);
	return 0;
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
	si_new,					/* New */
	(solard_driver_destroy_t)si_destroy,	/* Open */
	(solard_driver_open_t)si_open,		/* Open */
	(solard_driver_close_t)si_close,	/* Close */
	(solard_driver_read_t)si_read,		/* Read */
	(solard_driver_write_t)si_write,	/* Write */
	(solard_driver_config_t)si_config,	/* Config */
};

/* Function cannot return error */
int si_can_init(si_session_t *s) {
	solard_driver_t *old_driver;
	void *old_handle;

	/* Stop the read thread and wait for it to exit */
	dprintf(1,"SI_STATE_RUNNING: %d\n", solard_check_state(s,SI_STATE_RUNNING))
	if (solard_check_state(s,SI_STATE_RUNNING)) {
		solard_clear_state(s,SI_STATE_RUNNING);
		sleep(1);
	}

	/* If it's open, close it */
	if (solard_check_state(s,SI_STATE_OPEN)) si_close(s);

	/* Save current driver & handle */
	dprintf(1,"s->can: %p, s->can_handle: %p\n", s->can, s->can_handle);
	if (s->can && s->can_handle) {
		old_driver = s->can;
		old_handle = s->can_handle;
	} else {
		old_driver = 0;
		old_handle = 0;
	}

	/* Find the driver */
	if (strlen(s->can_transport)) s->can = find_driver(si_transports,s->can_transport);
	dprintf(1,"s->can: %p\n", s->can);
	if (!s->can) {
		/* Restore old driver & handle, open it and return */
		dprintf(1,"old_driver: %p, old_handle: %p\n", old_driver, old_handle);
		if (old_driver && old_handle) {
			dprintf(1,"restoring...\n");
			s->can = old_driver;
			s->can_handle = old_handle;
			_can_init(s);
			return si_open(s);
		/* Otherwise fallback to null driver if enabled */
		} else if (s->can_fallback) {
			dprintf(1,"setting null driver\n");
			s->can = &null_driver;
		} else if (strlen(s->can_transport)) {
			sprintf(s->errmsg,"unable to find CAN driver for transport: %s",s->can_transport);
			return 1;
		} else {
			sprintf(s->errmsg,"can_transport is empty!\n");
			return 1;
		}
	}

	/* Open new one */
	dprintf(1,"using driver: %s\n", s->can->name);
	s->can_handle = s->can->new(0, s->can_target, s->can_topts);
	dprintf(1,"can_handle: %p\n", s->can_handle);
	if (!s->can_handle) {
		dprintf(1,"old_driver: %p, old_handle: %p\n", old_driver, old_handle);
		/* Restore old driver */
		if (old_driver && old_handle) {
			dprintf(1,"restoring...\n");
			s->can = old_driver;
			s->can_handle = old_handle;
			/* Make sure we dont close it below */
			old_driver = 0;
		/* Otherwise fallback to null driver if enabled */
		} else if (s->can_fallback) {
			s->can = &null_driver;
			s->can_handle = s->can->new(0, 0, 0);
		} else {
			sprintf(s->errmsg,"unable to create new instance of CAN driver: %s",s->can->name);
			return 1;
		}
	}

	/* Warn if using the null driver */
	if (strcmp(s->can->name,"null") == 0) log_warning("using null driver for CAN I/O");

	/* Init and Open it */
	if (_can_init(s)) log_warning("can_init failed: %s\n", s->errmsg);
	si_open(s);

	/* Close and free the old handle */
	dprintf(1,"old_driver: %p, old_handle: %p\n", old_driver, old_handle);
	if (old_driver && old_handle) {
		old_driver->close(old_handle);
		old_driver->destroy(old_handle);
	}
	return 0;
}


/*
Copyright (c) 2021, Stephen P. Shoecraft
All rights reserved.

This source code is licensed under the BSD-style license found in the
LICENSE file in the root directory of this source tree.
*/

#include "jbd.h"
#include <pthread.h>

static void *jbd_can_recv_thread(void *handle) {
	return 0;
}

int jbd_get_local_can_data(jbd_session_t *s, int id, uint8_t *data, int datasz) {
#if 0
	char what[16];
	uint16_t mask;
	int idx,retries,len;

	dprintf(5,"id: %03x, data: %p, len: %d\n", id, data, datasz);

	idx = id - 0x300;
	mask = 1 << idx;
	dprintf(5,"mask: %04x, bitmap: %04x\n", mask, s->bitmap);
	retries=5;
	while(retries--) {
		if ((s->bitmap & mask) == 0) {
			dprintf(5,"bit not set, sleeping...\n");
			sleep(1);
			continue;
		}
		sprintf(what,"%03x", id);
//		if (debug >= 5) bindump(what,&s->frames[idx],sizeof(struct can_frame));
		len = (datasz > 8 ? 8 : datasz);
		memcpy(data,s->frames[idx].data,len);
		return 0;
	}
#endif
	return 1;
}

int jbd_get_remote_can_data(jbd_session_t *s, int id, uint8_t *data, int datasz) {
	return 1;
}

/* Func for can data that is remote (dont use thread/messages) */
static int _can_init(jbd_session_t *s) {
	pthread_attr_t attr;
	pthread_t th;

	/* Create a detached thread */
	if (pthread_attr_init(&attr)) {
		sprintf(s->errmsg,"pthread_attr_init: %s",strerror(errno));
		goto _can_init_error;
	}
	pthread_attr_setscope(&attr, PTHREAD_SCOPE_SYSTEM);
	if (pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_DETACHED)) {
		sprintf(s->errmsg,"pthread_attr_setdetachstate: %s",strerror(errno));
		goto _can_init_error;
	}
	solard_set_state(s,JBD_STATE_RUNNING);
	if (pthread_create(&th,&attr,&jbd_can_recv_thread,s)) {
		sprintf(s->errmsg,"pthread_create: %s",strerror(errno));
		goto _can_init_error;
	}
	s->can_get = jbd_get_local_can_data;
	return 0;
_can_init_error:
	s->can_get = jbd_get_remote_can_data;
	return 1;
}

void *jbd_new(void *ap, void *transport, void *transport_handle) {
	jbd_session_t *s;

	s = calloc(1,sizeof(*s));
	if (!s) {
		perror("jbd_new: calloc");
		return 0;
	}
	s->ap = ap;
	if (transport && transport_handle) {
		s->tp = transport;
		s->tp_handle = transport_handle;
		if (strcmp(s->tp->name,"can") == 0 &&_can_init(s)) {
			free(s);
			return 0;
		}
	}

	/* Save a copy of the name */
	if (ap) strcpy(s->name,s->ap->instance_name);

	return s;
}

int jbd_open(void *handle) {
	jbd_session_t *s = handle;
	int r;

	dprintf(3,"s: %p\n", s);

	r = 0;
	if (!solard_check_state(s,JBD_STATE_OPEN)) {
		if (s->tp->open(s->tp_handle) == 0)
			solard_set_state(s,JBD_STATE_OPEN);
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
	if (solard_check_state(s,JBD_STATE_OPEN)) {
		if (s->tp->close(s->tp_handle) == 0)
			solard_clear_state(s,JBD_STATE_OPEN);
		else
			r = 1;
	}
	dprintf(3,"returning: %d\n", r);
	return r;
}

int jbd_free(void *handle) {
	jbd_session_t *s = handle;

	if (solard_check_state(s,JBD_STATE_OPEN)) jbd_close(s);
	if (s->tp->destroy) s->tp->destroy(s->tp_handle);
	free(s);
	return 0;
}

solard_driver_t jbd_driver = {
	SOLARD_DRIVER_AGENT,
	"jbd",
	jbd_new,			/* New */
	jbd_free,			/* Free */
	jbd_open,			/* Open */
	jbd_close,			/* Close */
	jbd_read,			/* Read */
	0,				/* Write */
	jbd_config			/* Config */
};

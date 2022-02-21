
/*
Copyright (c) 2021, Stephen P. Shoecraft
All rights reserved.

This source code is licensed under the BSD-style license found in the
LICENSE file in the root directory of this source tree.
*/

#include "smanet_internal.h"
#include "transports.h"

#define dlevel 4

static solard_driver_t *smanet_transports[] = { &serial_driver, &rdev_driver, 0 };

static int tp_get(void *handle, uint8_t *buffer, int buflen) {
	smanet_session_t *s = handle;
	int bytes;

	bytes = s->tp->read(s->tp_handle,0,buffer,buflen);
	dprintf(dlevel,"bytes: %d\n", bytes);
	return bytes;
}

int smanet_connect(smanet_session_t *s, char *transport, char *target, char *topts) {

	dprintf(dlevel,"transport: %s, target: %s, topts: %s\n", transport, target, topts);
	if (!transport || !target) return 0;
	if (!strlen(transport) || !strlen(target)) return 0;

	/* Find the driver */
	s->tp = find_driver(smanet_transports,transport);
	if (!s->tp) {
		sprintf(s->errmsg,"unable to find smanet transport: %s", transport);
		goto smanet_connect_error;
	}

	/* Create a new driver instance */
	s->tp_handle = s->tp->new(target, topts ? topts : "");
	if (!s->tp_handle) {
		sprintf(s->errmsg,"%s_new: %s", transport, strerror(errno));
		goto smanet_connect_error;
	}

	dprintf(dlevel,"opening transport...\n");
	if (!s->tp->open) {
		sprintf(s->errmsg,"smanet_connect: transport does not have open func!");
		goto smanet_connect_error;
	}
	if (s->tp->open(s->tp_handle)) {
		sprintf(s->errmsg,"smanet_connect: unable to open transport");
		goto smanet_connect_error;
	}
	dprintf(dlevel,"opened...\n");
	if (smanet_get_net_start(s,&s->serial,s->type,sizeof(s->type)-1)) {
		sprintf(s->errmsg,"smanet_connect: unable to start network");
		goto smanet_connect_error;
	}
	dprintf(dlevel,"serial: %ld, type: %s\n", s->serial, s->type);
	sleep(1);
	if (smanet_cfg_net_adr(s,0)) {
		sprintf(s->errmsg,"smanet_connect: unable to configure network address");
		goto smanet_connect_error;
	}
	s->connected = 1;
	return 0;
smanet_connect_error:
	dprintf(dlevel,"error!\n");
	s->connected = 0;
	return 1;
}

smanet_session_t *smanet_init(char *transport, char *target, char *topts) {
	smanet_session_t *s;

	dprintf(dlevel,"transport: %s, target: %s, topts: %s\n", transport, target, topts);

	s = calloc(1,sizeof(*s));
	if (!s) {
		log_write(LOG_SYSERR,"smanet_init: calloc");
		goto smanet_init_error;
	}
//	s->channels = list_create();
	pthread_mutex_init(&s->lock, 0);
	s->b = buffer_init(256,tp_get,s);
	if (!s->b) {
		log_write(LOG_SYSERR,"smanet_init: buffer_init");
		goto smanet_init_error;
	}

	/* We are addr 1 */
	s->src = 1;

	smanet_connect(s, transport, target, topts ? topts : "");
	return s;

smanet_init_error:
	free(s);
	return 0;
}

void smanet_destroy(smanet_session_t *s) {
	if (!s) return;
	if (s->tp) {
		if (s->connected) s->tp->close(s->tp_handle);
		if (s->tp->destroy) s->tp->destroy(s->tp_handle);
	}
	if (s->chans) free(s->chans);
//	if (s->values) free(values);
	buffer_free(s->b);
}

char *smanet_get_errmsg(smanet_session_t *s) {
	return s->errmsg;
}

int smanet_lock(smanet_session_t *s) {
	dprintf(dlevel,"locking %p\n",s);
	pthread_mutex_lock(&s->lock);
	dprintf(dlevel,"done!\n");
	return 0;
}

int smanet_unlock(smanet_session_t *s) {
	dprintf(dlevel,"unlocking %p\n",s);
	pthread_mutex_unlock(&s->lock);
	dprintf(dlevel,"done!\n");
	return 0;
}

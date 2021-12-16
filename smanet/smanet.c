
/*
Copyright (c) 2021, Stephen P. Shoecraft
All rights reserved.

This source code is licensed under the BSD-style license found in the
LICENSE file in the root directory of this source tree.
*/

#include "smanet_internal.h"
#include "common.h"

static int tp_get(void *handle, uint8_t *buffer, int buflen) {
	smanet_session_t *s = handle;
	int bytes;

	bytes = s->tp->read(s->tp_handle,buffer,buflen);
	dprintf(8,"bytes: %d\n", bytes);
	return bytes;
}


smanet_session_t *smanet_init(solard_driver_t *tp, void *tp_handle) {
	smanet_session_t *s;

	s = calloc(1,sizeof(*s));
	if (!s) {
		log_write(LOG_SYSERR,"smanet_init: calloc");
		return 0;
	}
	s->tp = tp;
	s->tp_handle = tp_handle;
	s->channels = list_create();
	pthread_mutex_init(&s->lock, 0);
	s->b = buffer_init(256,tp_get,s);
	if (!s->b) goto smanet_init_error;

	/* We are addr 1 */
	s->src = 1;

	dprintf(1,"opening transport...\n");
	if (!tp->open) {
		log_error("transport does not have open func!\n");
		goto smanet_init_error;
	}
	if (tp->open(tp_handle)) goto smanet_init_error;
	if (smanet_get_net_start(s,&s->serial,s->type,sizeof(s->type)-1)) goto smanet_init_error;
	dprintf(smanet_dlevel,"serial: %ld, type: %s\n", s->serial, s->type);
	sleep(1);
	if (smanet_cfg_net_adr(s,0)) goto smanet_init_error;
	sprintf(s->chanpath,"%s/%s.dat",SOLARD_LIBDIR,s->type);
	dprintf(1,"done!\n");
	return s;

smanet_init_error:
	if (tp && tp_handle && tp->close) tp->close(tp_handle);
	free(s);
	dprintf(1,"error!\n");
	return 0;
}

char *smanet_get_errmsg(smanet_session_t *s) {
	return s->errmsg;
}

int smanet_lock(smanet_session_t *s) {
	dprintf(1,"locking %p\n",s);
	pthread_mutex_lock(&s->lock);
	dprintf(1,"done!\n");
	return 0;
}

int smanet_unlock(smanet_session_t *s) {
	dprintf(1,"unlocking %p\n",s);
	pthread_mutex_unlock(&s->lock);
	dprintf(1,"done!\n");
	return 0;
}

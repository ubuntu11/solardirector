
/*
Copyright (c) 2021, Stephen P. Shoecraft
All rights reserved.

This source code is licensed under the BSD-style license found in the
LICENSE file in the root directory of this source tree.
*/

#include "smanet_internal.h"

smanet_session_t *smanet_init(solard_module_t *tp, void *tp_handle) {
	smanet_session_t *s;

	s = calloc(1,sizeof(*s));
	if (!s) {
		log_write(LOG_SYSERR,"smanet_init: calloc");
		return 0;
	}
	s->tp = tp;
	s->tp_handle = tp_handle;
	s->channels = list_create();

	if (!tp->open) goto smanet_init_error;
        if (tp->open(tp_handle)) goto smanet_init_error;
	if (smanet_get_net_start(s,&s->serial,s->type,sizeof(s->type)-1)) goto smanet_init_error;
        dprintf(1,"serial: %ld, type: %s\n", s->serial, s->type);
	sleep(1);
	if (smanet_cfg_net_adr(s,1)) goto smanet_init_error;
	return s;

smanet_init_error:
	if (tp && tp->close) tp->close(tp_handle);
	free(s);
	return 0;
}

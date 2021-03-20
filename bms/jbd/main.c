/*
Copyright (c) 2021, Stephen P. Shoecraft
All rights reserved.

This source code is licensed under the BSD-style license found in the
LICENSE file in the root directory of this source tree.
*/

#include "jbd.h"

void *jbd_new(void *conf, void *transport, void *transport_handle) {
	jbd_session_t *s;

	s = calloc(1,sizeof(*s));
	if (!s) {
		perror("jbd_new: calloc");
		return 0;
	}
	s->conf = conf;
	s->tp = transport;
	s->tp_handle = transport_handle;

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

static solard_module_t jbd_driver = {
	SOLARD_MODTYPE_BATTERY,
	"jbd",
	0,				/* Init */
	jbd_new,			/* New */
	jbd_info,			/* info */
	jbd_open,			/* Open */
	jbd_read,			/* Read */
	0,				/* Write */
	jbd_close,			/* Close */
	0,				/* Free */
	0,				/* Shutdown */
	jbd_control,			/* Control */
	jbd_config			/* Config */
};

int main(int argc, char **argv) {
	opt_proctab_t opts[] = {
		OPTS_END
	};
	solard_agent_t *ap;

//	jbd_driver.read = 0;
//	jbd_driver.info = 0;

	ap = agent_init(argc,argv,opts,&jbd_driver);
	dprintf(1,"ap: %p\n",ap);
	if (!ap) return 1;
	agent_run(ap);
	return 0;
}

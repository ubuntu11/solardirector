/*
Copyright (c) 2021, Stephen P. Shoecraft
All rights reserved.

This source code is licensed under the BSD-style license found in the
LICENSE file in the root directory of this source tree.
*/

#include "jk.h"

void *jk_new(void *conf, void *driver, void *driver_handle) {
	jk_session_t *s;

	s = calloc(1,sizeof(*s));
	if (!s) {
		perror("jk_new: calloc");
		return 0;
	}
	s->conf = conf;
	s->tp = driver;
	s->tp_handle = driver_handle;

//	strncat(s->name,conf->bat.name,sizeof(s->name)-1);

	return s;
}

int jk_open(void *handle) {
	jk_session_t *s = handle;
	int r;

	dprintf(3,"s: %p\n", s);

	r = 0;
	if (!solard_check_state(s,BATTERY_STATE_OPEN)) {
		dprintf(1,"tp: %p\n", s->tp);
		dprintf(1,"tp->open: %p\n", s->tp->open);
		if (s->tp->open(s->tp_handle) == 0)
			solard_set_state(s,BATTERY_STATE_OPEN);
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

static solard_module_t jk_driver = {
	SOLARD_MODTYPE_BATTERY,
	"jk",
	0,				/* Init */
	jk_new,				/* New */
	jk_info,			/* info */
	jk_open,			/* Open */
	jk_read,			/* Read */
	0,				/* Write */
	jk_close,			/* Close */
	0,				/* Free */
	0,				/* Shutdown */
	0,				/* Control */
	jk_config			/* Config */
};

int main(int argc, char **argv) {
	opt_proctab_t opts[] = {
		OPTS_END
	};
	solard_agent_t *ap;
        char *args[] = { "t2", "-d", "9", "-c", "pack_01.conf" };
        #define nargs (sizeof(args)/sizeof(char *))

//	jk_driver.read = 0;
//	jk_driver.info = 0;

//	ap = agent_init(argc,argv,opts,&jk_driver);
	ap = agent_init(nargs,args,opts,&jk_driver);
	dprintf(1,"ap: %p\n",ap);
	if (!ap) return 1;
//	ap->interval = 5;
	agent_run(ap);
	return 0;
}

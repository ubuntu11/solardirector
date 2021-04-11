/*
Copyright (c) 2021, Stephen P. Shoecraft
All rights reserved.

This source code is licensed under the BSD-style license found in the
LICENSE file in the root directory of this source tree.
*/

#include "sb.h"

void *sb_new(void *conf, void *tp, void *tp_handle) {
	sb_session_t *s;

	s = calloc(1,sizeof(*s));
	if (!s) {
		perror("sb_new: calloc");
		return 0;
	}
	s->conf = conf;
	s->tp = tp;
	s->tp_handle = tp_handle;

//	strncat(s->name,s->conf->name,sizeof(s->name)-1);

	return s;
}

int sb_open(void *handle) {
#if 0
	sb_session_t *s = handle;
	int r;

	dprintf(3,"s: %p\n", s);

	r = 0;
	if (!solard_check_state(s,SOLARD__OPEN)) {
		dprintf(1,"tp: %p\n", s->tp);
		dprintf(1,"tp->open: %p\n", s->tp->open);
		if (s->tp->open(s->tp_handle) == 0)
			solard_set_state(s,SOLARD__OPEN);
		else
			r = 1;
	}
	dprintf(3,"returning: %d\n", r);
	return r;
#endif
	return 0;
}

int sb_close(void *handle) {
#if 0
	sb_session_t *s = handle;
	int r;

	dprintf(3,"s: %p\n", s);
	r = 0;
	if (solard_check_state(s,SOLARD__OPEN)) {
		if (s->tp->close(s->tp_handle) == 0)
			solard_clear_state(s,SOLARD__OPEN);
		else
			r = 1;
	}
	dprintf(3,"returning: %d\n", r);
	return r;
#endif
	return 0;
}

static solard_module_t sb_driver = {
	SOLARD_MODTYPE_INVERTER,
	"sb",
	0,				/* Init */
	sb_new,				/* New */
	sb_info,			/* info */
	sb_open,			/* Open */
	sb_read,			/* Read */
	0,				/* Write */
	sb_close,			/* Close */
	0,				/* Free */
	0,				/* Shutdown */
	sb_control,			/* Control */
	sb_config			/* Config */
};

int main(int argc, char **argv) {
	opt_proctab_t opts[] = {
		OPTS_END
	};
	solard_agent_t *ap;
        char *args[] = { "t2", "-d", "5", "-c", "sb.conf" };
        #define nargs (sizeof(args)/sizeof(char *))

//	sb_driver.read = 0;
//	sb_driver.info = 0;
//	sb_driver.control = 0;
//	sb_driver.config = 0;

	ap = agent_init(nargs,args,opts,&sb_driver);
	dprintf(1,"ap: %p\n",ap);
	if (!ap) return 1;
//	ap->interval = 5;
	agent_run(ap);
	return 0;
}

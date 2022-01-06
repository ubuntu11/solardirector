/*
Copyright (c) 2021, Stephen P. Shoecraft
All rights reserved.

This source code is licensed under the BSD-style license found in the
LICENSE file in the root directory of this source tree.
*/

#include "sb.h"
#include "transports.h"

#define TESTING 0

char *sb_version_string = "1.0";
extern solard_driver_t sb_driver;

static int sb_agent_init(int argc, char **argv, opt_proctab_t *sb_opts, sb_session_t *s) {
	config_property_t sb_props[] = {
		{0}
	};
	config_function_t sb_funcs[] = {
		{ "get", 0 },
		{ "set", 0 },
		{0}
	};

	s->ap = agent_init(argc,argv,sb_opts,&sb_driver,s,sb_props,sb_funcs);
	dprintf(1,"ap: %p\n",s->ap);
	if (!s->ap) return 1;
	return 0;
}

static solard_driver_t *transports[] = { &http_driver, 0 };

/* Function cannot return error */
int sb_tp_init(sb_session_t *s) {
	solard_driver_t *old_driver;
	void *old_handle;

	/* If it's open, close it */
	if (solard_check_state(s,SB_STATE_OPEN)) sb_driver.close(s);

	/* Save current driver & handle */
	dprintf(1,"s->tp: %p, s->tp_handle: %p\n", s->tp, s->tp_handle);
	if (s->tp && s->tp_handle) {
		old_driver = s->tp;
		old_handle = s->tp_handle;
	} else {
		old_driver = 0;
		old_handle = 0;
	}

	/* Find the driver */
	if (strlen(s->transport)) s->tp = find_driver(transports,s->transport);
	dprintf(1,"s->tp: %p\n", s->tp);
	if (!s->tp) {
		/* Restore old driver & handle, open it and return */
		dprintf(1,"old_driver: %p, old_handle: %p\n", old_driver, old_handle);
		if (old_driver && old_handle) {
			dprintf(1,"restoring...\n");
			s->tp = old_driver;
			s->tp_handle = old_handle;
			return 1;
#if 0
		/* Otherwise fallback to null driver if enabled */
		} else if (s->tp_fallback) {
			dprintf(1,"setting null driver\n");
			s->tp = &null_driver;
#endif
		} else if (strlen(s->transport)) {
			sprintf(s->errmsg,"unable to find driver for transport: %s",s->transport);
			return 1;
		} else {
			sprintf(s->errmsg,"transport is empty!\n");
			return 1;
		}
	}

	/* Open new one */
	dprintf(1,"using driver: %s\n", s->tp->name);
	s->tp_handle = s->tp->new(0, s->target, s->topts);
	dprintf(1,"tp_handle: %p\n", s->tp_handle);
	if (!s->tp_handle) {
		dprintf(1,"old_driver: %p, old_handle: %p\n", old_driver, old_handle);
		/* Restore old driver */
		if (old_driver && old_handle) {
			dprintf(1,"restoring...\n");
			s->tp = old_driver;
			s->tp_handle = old_handle;
			/* Make sure we dont close it below */
			old_driver = 0;
#if 0
		/* Otherwise fallback to null driver if enabled */
		} else if (s->tp_fallback) {
			s->tp = &null_driver;
			s->tp_handle = s->tp->new(0, 0, 0);
#endif
		} else {
			sprintf(s->errmsg,"unable to create new instance %s driver",s->tp->name);
			return 1;
		}
	}

	/* Warn if using the null driver */
	if (strcmp(s->tp->name,"null") == 0) log_warning("using null driver for I/O");

#if 0
        /* Open the new driver */
        if (jk_open(s)) {
                sprintf(s->errmsg,"open failed!");
                return 1;
        }
#endif

	/* Close and free the old handle */
	dprintf(1,"old_driver: %p, old_handle: %p\n", old_driver, old_handle);
	if (old_driver && old_handle) {
		old_driver->close(old_handle);
		old_driver->destroy(old_handle);
	}
	return 0;
}

int main(int argc, char **argv) {
	sb_session_t *s;
	char tpinfo[128];
	opt_proctab_t opts[] = {
		/* Spec, dest, type len, reqd, default val, have */
		{ "-t::|transport,target,opts",&tpinfo,DATA_TYPE_STRING,sizeof(tpinfo)-1,0,"" },
		OPTS_END
	};
#if TESTING
	char *args[] = { "sb", "-d", "6", "-c", "sbtest.conf" };
	argc = sizeof(args)/sizeof(char *);
	argv = args;
#endif

	/* Init the driver */
	s = sb_driver.new(0,0,0);
	if (!s) return 1;

	/* Agent init */
	if (sb_agent_init(argc,argv,opts,s)) return 1;

	/* -t takes precedence over config */
	dprintf(1,"tpinfo: %s\n", tpinfo);
	if (strlen(tpinfo)) {
		*s->transport = *s->target = *s->topts = 0;
		strncat(s->transport,strele(0,",",tpinfo),sizeof(s->transport)-1);
		strncat(s->target,strele(1,",",tpinfo),sizeof(s->target)-1);
		strncat(s->topts,strele(2,",",tpinfo),sizeof(s->topts)-1);
	}
	dprintf(1,"s->transport: %s, s->target: %s, s->topts: %s\n", s->transport, s->target, s->topts);
	if (sb_tp_init(s)) {
		log_error(s->errmsg);
		return 1;
	}

	/* Main loop */
	log_write(LOG_INFO,"Running...\n");
	agent_run(s->ap);
	return 0;
}
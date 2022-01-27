/*
Copyright (c) 2021, Stephen P. Shoecraft
All rights reserved.

This source code is licensed under the BSD-style license found in the
LICENSE file in the root directory of this source tree.
*/

#include "jbd.h"
#include "transports.h"
#include <pthread.h>

#define TESTING 0

char *jbd_version_string = "1.0";
extern solard_driver_t jbd_driver;

static int jbd_agent_init(int argc, char **argv, opt_proctab_t *jbd_opts, jbd_session_t *s) {
	config_property_t jbd_props[] = {
#if 0
		{ "name", DATA_TYPE_STRING, &s->name, sizeof(s->name)-1, 0, 0,
			0, 0, 0, 0, 0, 0, 1, 0 },
#endif
		{ "transport", DATA_TYPE_STRING, s->transport, sizeof(s->transport)-1, 0, 0 },
		{ "target", DATA_TYPE_STRING, s->target, sizeof(s->target)-1, 0, 0 },
		{ "topts", DATA_TYPE_STRING, s->topts, sizeof(s->topts)-1, 0, 0 },
                { "capacity", DATA_TYPE_FLOAT, &s->data.capacity, 0, 0, CONFIG_FLAG_READONLY | CONFIG_FLAG_NOSAVE },
                { "voltage", DATA_TYPE_FLOAT, &s->data.voltage, 0, 0, CONFIG_FLAG_READONLY | CONFIG_FLAG_NOSAVE },
                { "current", DATA_TYPE_FLOAT, &s->data.current, 0, 0, CONFIG_FLAG_READONLY | CONFIG_FLAG_NOSAVE },
                { "ntemps", DATA_TYPE_INT, &s->data.ntemps, 0, 0, CONFIG_FLAG_READONLY | CONFIG_FLAG_NOSAVE },
                { "temps", DATA_TYPE_FLOAT_ARRAY, &s->data.temps, JBD_MAX_TEMPS, 0, CONFIG_FLAG_READONLY | CONFIG_FLAG_NOSAVE },
                { "ncells", DATA_TYPE_INT, &s->data.ncells, 0, 0, CONFIG_FLAG_READONLY | CONFIG_FLAG_NOSAVE },
                { "cellvolt", DATA_TYPE_FLOAT_ARRAY, &s->data.cellvolt, JBD_MAX_CELLS, 0, CONFIG_FLAG_READONLY | CONFIG_FLAG_NOSAVE },
                { "cell_min", DATA_TYPE_FLOAT, &s->data.cell_min, 0, 0, CONFIG_FLAG_READONLY | CONFIG_FLAG_NOSAVE },
                { "cell_max", DATA_TYPE_FLOAT, &s->data.cell_max, 0, 0, CONFIG_FLAG_READONLY | CONFIG_FLAG_NOSAVE },
                { "cell_diff", DATA_TYPE_FLOAT, &s->data.cell_diff, 0, 0, CONFIG_FLAG_READONLY | CONFIG_FLAG_NOSAVE },
                { "cell_avg", DATA_TYPE_FLOAT, &s->data.cell_avg, 0, 0, CONFIG_FLAG_READONLY | CONFIG_FLAG_NOSAVE },
                { "cell_total", DATA_TYPE_FLOAT, &s->data.cell_total, 0, 0, CONFIG_FLAG_READONLY | CONFIG_FLAG_NOSAVE },
                { "balancebits", DATA_TYPE_INT, &s->data.balancebits, 0, 0, CONFIG_FLAG_READONLY | CONFIG_FLAG_NOSAVE },
		{0}
	};

	s->ap = agent_init(argc,argv,jbd_opts,&jbd_driver,s,jbd_props,0);
	dprintf(1,"ap: %p\n",s->ap);
	if (!s->ap) return 1;
	return 0;
}

#if defined(__WIN32) || defined(__WIN64)
static solard_driver_t *transports[] = { &ip_driver, &serial_driver, 0 };
#else
#ifdef BLUETOOTH
static solard_driver_t *transports[] = { &ip_driver, &bt_driver, &serial_driver, &can_driver, 0 };
#else
static solard_driver_t *transports[] = { &ip_driver, &serial_driver, &can_driver, 0 };
#endif
#endif

/* Function cannot return error */
int jbd_tp_init(jbd_session_t *s) {
	solard_driver_t *old_driver;
	void *old_handle;

	/* If it's open, close it */
	if (solard_check_state(s,JBD_STATE_OPEN)) jbd_close(s);

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
			return jbd_open(s);
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
			return 2;
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

	/* Open the new driver */
	jbd_open(s);

	/* Close and free the old handle */
	dprintf(1,"old_driver: %p, old_handle: %p\n", old_driver, old_handle);
	if (old_driver && old_handle) {
		old_driver->close(old_handle);
		old_driver->destroy(old_handle);
	}
	return 0;
}

int main(int argc, char **argv) {
	jbd_session_t *s;
	char tpinfo[128];
	opt_proctab_t opts[] = {
		/* Spec, dest, type len, reqd, default val, have */
		{ "-t::|transport,target,opts",&tpinfo,DATA_TYPE_STRING,sizeof(tpinfo)-1,0,"" },
		OPTS_END
	};
	int status;
#if TESTING
	char *args[] = { "jbd", "-d", "5", "-c", "jbdtest.conf" };
	argc = sizeof(args)/sizeof(char *);
	argv = args;
#endif

	/* Init the driver */
	s = jbd_driver.new(0,0,0);
	if (!s) return 1;

	/* Agent init */
	if (jbd_agent_init(argc,argv,opts,s)) return 1;

	/* -t takes precedence over config */
	dprintf(1,"tpinfo: %s\n", tpinfo);
	if (strlen(tpinfo)) {
		*s->transport = *s->target = *s->topts = 0;
		strncat(s->transport,strele(0,",",tpinfo),sizeof(s->transport)-1);
		strncat(s->target,strele(1,",",tpinfo),sizeof(s->target)-1);
		strncat(s->topts,strele(2,",",tpinfo),sizeof(s->topts)-1);
	}
	dprintf(1,"s->transport: %s, s->target: %s, s->topts: %s\n", s->transport, s->target, s->topts);
	status = jbd_tp_init(s);
	if (status == 2) jbd_driver.read = jbd_driver.write = 0;
	else if (status) goto jbd_init_error;

	/* Get hwinfo */
//	if (jbd_get_hwinfo(s)) return 1;
//	jbd_get_hwinfo(s);

	/* Send info */
//	sprintf(tpinfo,"%s/info.js", s->ap->script_dir);
//	agent_start_script(s->ap, tpinfo);

	/* Main loop */
	log_write(LOG_INFO,"Running...\n");
	agent_run(s->ap);
	return 0;
jbd_init_error:
	log_error(s->errmsg);
	return 1;
}

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

int jbd_reset(void *h, int nargs, char **args, char *errmsg) {
	jbd_session_t *s = h;
	uint8_t payload[2];
	int opened,r;
#if 0
2021-06-05 16:33:25,846 INFO client 192.168.1.7:45868 -> server pack_01:23 (9 bytes)
-> 0000   DD 5A E3 02 43 21 FE B7 77                         .Z..C!..w

2021-06-05 16:33:26,032 INFO client 192.168.1.7:45868 <= server pack_01:23 (7 bytes)
<= 0000   DD E3 80 00 FF 80 77                               ......w
#endif

#if 0
2021-06-05 19:34:58,910 INFO client 192.168.1.7:39154 -> server pack_01:23 (9 bytes)
-> 0000   DD 5A 0A 02 18 81 FF 5B 77                         .Z.....[w

2021-06-05 19:34:59,150 INFO client 192.168.1.7:39154 <= server pack_01:23 (7 bytes)
<= 0000   DD 0A 80 00 FF 80 77                               ......w
#endif

	dprintf(1,"locking...\n");
	pthread_mutex_lock(&s->lock);
	if (!solard_check_state(s,JBD_STATE_OPEN)) {
		if (jbd_driver.open(s)) {
			pthread_mutex_unlock(&s->lock);
			return 1;
		}
		opened = 1;
	}
	jbd_putshort(payload,0x2143);
	r = jbd_rw(s, JBD_CMD_WRITE, JBD_REG_RESET, payload, sizeof(payload));
	if (opened) jbd_driver.close(s);
	dprintf(1,"unlocking...\n");
	pthread_mutex_unlock(&s->lock);
        return r;
}

static int jbd_agent_init(int argc, char **argv, opt_proctab_t *jbd_opts, jbd_session_t *s) {
	config_property_t jbd_props[] = {
#if 0
		{ "name", DATA_TYPE_STRING, &s->name, sizeof(s->name)-1, 0, 0,
			0, 0, 0, 0, 0, 0, 1, 0 },
#endif
		{ "transport", DATA_TYPE_STRING, s->transport, sizeof(s->transport)-1, 0, 0,
			0, 0, 0, 0, 0, 0, 1, 0 },
		{ "target", DATA_TYPE_STRING, s->target, sizeof(s->target)-1, 0, 0,
			0, 0, 0, 0, 0, 0, 1, 0 },
		{ "topts", DATA_TYPE_STRING, s->topts, sizeof(s->topts)-1, 0, 0,
			0, 0, 0, 0, 0, 0, 1, 0 },
		{0}
	};
	config_function_t jbd_funcs[] = {
#if 0
		{ "get", jbd_get, s, 1 },
		{ "set", jbd_set, s, 2 },
		{ "reset", jbd_reset, s, 0 },
#endif
		{0}
	};

	s->ap = agent_init(argc,argv,jbd_opts,&jbd_driver,s,jbd_props,jbd_funcs);
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
	int info_flag;
	opt_proctab_t opts[] = {
		/* Spec, dest, type len, reqd, default val, have */
		{ "-t::|transport,target,opts",&tpinfo,DATA_TYPE_STRING,sizeof(tpinfo)-1,0,"" },
		{ "-I|agent info",&info_flag,DATA_TYPE_LOGICAL,0,0,"no" },
		OPTS_END
	};
#if TESTING
	char *args[] = { "jbd", "-d", "6", "-c", "jbdtest.conf" };
	argc = sizeof(args)/sizeof(char *);
	argv = args;
#endif

	/* Init the driver */
	s = jbd_driver.new(0,0,0);
	if (!s) return 1;

	/* Agent init */
	if (jbd_agent_init(argc,argv,opts,s)) {
		log_error(s->errmsg);
		return 1;
	}

	/* -t takes precedence over config */
	dprintf(1,"tpinfo: %s\n", tpinfo);
	if (strlen(tpinfo)) {
		*s->transport = *s->target = *s->topts = 0;
		strncat(s->transport,strele(0,",",tpinfo),sizeof(s->transport)-1);
		strncat(s->target,strele(1,",",tpinfo),sizeof(s->target)-1);
		strncat(s->topts,strele(2,",",tpinfo),sizeof(s->topts)-1);
	}
	dprintf(1,"s->transport: %s, s->target: %s, s->topts: %s\n", s->transport, s->target, s->topts);
	if (jbd_tp_init(s)) {
		log_error(s->errmsg);
		return 1;
	}

	if (info_flag) return jbd_info(s);

	/* Main loop */
	log_write(LOG_INFO,"Running...\n");
	agent_run(s->ap);
	return 0;
}

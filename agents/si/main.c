/*
Copyright (c) 2021, Stephen P. Shoecraft
All rights reserved.

This source code is licensed under the BSD-style license found in the
LICENSE file in the root directory of this source tree.
*/

#include "si.h"
#include "transports.h"

#define TESTING 0

#define DEBUG_STARTUP 1

#if DEBUG_STARTUP
#define _ST_DEBUG LOG_DEBUG
#else
#define _ST_DEBUG 0
#endif

char *si_agent_version_string = "1.0";

static int si_cb(void *ctx) {
	si_session_t *s = ctx;

	/* Get the relay/func bits */
	if (s->can_connected && si_can_read_relays(s) == 0) {
		if (s->data.GdOn != s->grid_connected)
			log_info("Grid %s\n",(s->data.GdOn ? "connected" : "disconnected"));
		if (s->data.GnOn != s->gen_connected)
			log_info("Generator %s\n",(s->data.GnOn ? "connected" : "disconnected"));
		if ((s->data.GdOn != s->grid_connected) || (s->data.GnOn != s->gen_connected)) si_can_write_va(s);
		s->grid_connected = s->data.GdOn;
		s->gen_connected = s->data.GnOn;
	}
	return 0;
}

int main(int argc, char **argv) {
	char cantpinfo[256];
	char smatpinfo[256];
	opt_proctab_t si_opts[] = {
		{ "-t::|CAN transport,target,opts",&cantpinfo,DATA_TYPE_STRING,sizeof(cantpinfo)-1,0,"" },
		{ "-u::|SMANET transport,target,opts",&smatpinfo,DATA_TYPE_STRING,sizeof(smatpinfo)-1,0,"" },
		OPTS_END
	};
	si_session_t *s;
	time_t start,end,diff;

#if TESTING
	char *args[] = { "si", "-d", "7", "-c", "sitest.conf" };
	argc = (sizeof(args)/sizeof(char *));
	argv = args;
#endif

	time(&start);

	log_open("si",0,LOG_INFO|LOG_WARNING|LOG_ERROR|LOG_SYSERR|_ST_DEBUG);

//	log_write(LOG_INFO,"SMA Sunny Island Agent version %s\n",si_agent_version_string);
//	log_write(LOG_INFO,"Starting up...\n");

	/* Init the SI driver */
	s = si_driver.new(0,0,0);
	if (!s) return 1;

	if (si_agent_init(argc,argv,si_opts,s)) return 1;

	/* -t takes precedence over config */
	dprintf(1,"cantpinfo: %s\n", cantpinfo);
	if (strlen(cantpinfo)) {
		*s->can_transport = *s->can_target = *s->can_topts = 0;
		strncat(s->can_transport,strele(0,",",cantpinfo),sizeof(s->can_transport)-1);
		strncat(s->can_target,strele(1,",",cantpinfo),sizeof(s->can_target)-1);
		strncat(s->can_topts,strele(2,",",cantpinfo),sizeof(s->can_topts)-1);
	}
	dprintf(1,"s->can_transport: %s, s->can_target: %s, s->can_topts: %s\n",
		s->can_transport, s->can_target, s->can_topts);

	if (si_can_init(s)) {
		/* safety */
		s->can = &null_driver;
		s->can_handle = s->can->new(0,0,0);
		si_can_set_reader(s);
	} else {
		s->ap->read_interval = s->ap->write_interval = s->interval = 10;
	}

	/* init SMANET but do not connect */
	dprintf(1,"smatpinfo: %s\n", smatpinfo);
	if (strlen(smatpinfo)) {
		*s->smanet_transport = *s->smanet_target = *s->smanet_topts = 0;
		strncat(s->smanet_transport,strele(0,",",smatpinfo),sizeof(s->smanet_transport)-1);
		strncat(s->smanet_target,strele(1,",",smatpinfo),sizeof(s->smanet_target)-1);
		strncat(s->smanet_topts,strele(2,",",smatpinfo),sizeof(s->smanet_topts)-1);
        }
	s->smanet = smanet_init(0,0,0);

	/* Set callback */
	agent_set_callback(s->ap,si_cb,s);

	time(&end);
	diff = end - start;
	dprintf(1,"--> startup time: %d\n", diff);

	/* Go */
	agent_run(s->ap);
	return 0;
}

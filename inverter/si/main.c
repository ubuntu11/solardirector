
/*
Copyright (c) 2021, Stephen P. Shoecraft
All rights reserved.

This source code is licensed under the BSD-style license found in the
LICENSE file in the root directory of this source tree.
*/

#include "si.h"
#include <pthread.h>

char *si_agent_version_string = "1.0";

int si_open(si_session_t *s);

void *si_new(void *conf, void *driver, void *driver_handle) {
	si_session_t *s;

	log_write(LOG_INFO,"SMA Sunny Island Agent version %s\n",si_agent_version_string);
	log_write(LOG_INFO,"Starting up...\n");

	s = calloc(1,sizeof(*s));
	if (!s) {
		perror("si_new: calloc");
		return 0;
	}
	s->ap = conf;
	s->can = driver;
	s->can_handle = driver_handle;
	s->desc = list_create();

	/* Start background recv thread */
	dprintf(1,"driver name: %s\n", s->can->name);
	if (strcmp(s->can->name,"can") == 0) {
		pthread_attr_t attr;
		
		/* Create a detached thread */
		if (pthread_attr_init(&attr)) {
			perror("si_new: pthread_attr_init");
			goto si_new_error;
		}
		if (pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_DETACHED)) {
			perror("si_new: pthread_attr_setdetachstate");
			goto si_new_error;
		}
		solard_set_state(s,SI_STATE_RUNNING);
		if (pthread_create(&s->th,&attr,&si_recv_thread,s)) {
			perror("si_new: pthread_create");
			goto si_new_error;
		}
		dprintf(1,"setting func to local data\n");
		s->can_read = si_get_local_can_data;
	} else {
		dprintf(1,"setting func to remote data\n");
		s->can_read = si_get_remote_can_data;
	}

	/* Go ahead and open (and keep open) the can bus */
//	si_open(s);

	dprintf(1,"returning: %p\n", s);
	return s;
si_new_error:
	free(s);
	return 0;
}

int si_open(si_session_t *s) {
	int r;

	dprintf(3,"s: %p\n", s);

	r = 0;
	if (!solard_check_state(s,SI_STATE_OPEN)) {
		if (s->can->open(s->can_handle) == 0)
			solard_set_state(s,SI_STATE_OPEN);
		else
			r = 1;
	}
	dprintf(3,"returning: %d\n", r);
	return r;
}

int si_close(void *handle) {
	/* We only close when shutdown */
	return 0;
#if 0
	si_session_t *s = handle;
	int r;

	dprintf(3,"s: %p\n", s);
	r = 0;
	if (solard_check_state(s,SI_STATE_OPEN)) {
		if (s->can->close(s->can_handle) == 0)
			solard_clear_state(s,SI_STATE_OPEN);
		else
			r = 1;
	}
	dprintf(3,"returning: %d\n", r);
	return r;
#endif
}

static solard_module_t si_driver = {
	SOLARD_MODTYPE_INVERTER,
	"si",
	0,				/* Init */
	si_new,				/* New */
	si_info,			/* info */
	(solard_module_open_t)si_open,			/* Open */
	(solard_module_read_t)si_read,			/* Read */
	(solard_module_write_t)si_write,			/* Write */
	(solard_module_close_t)si_close,			/* Close */
	0,				/* Free */
	0,				/* Shutdown */
	si_control,			/* Control */
	si_config,			/* Config */
	0,			/* Get handle */
	0,				/* Get private */
};

int si_startstop(si_session_t *s, int op) {
	int retries;
	uint8_t data[8],b;

	dprintf(1,"op: %d\n", op);

	b = (op ? 1 : 2);
	dprintf(1,"b: %d\n", b);
	retries=10;
	while(retries--) {
		dprintf(1,"writing...\n");
		if (si_can_write(s,0x35c,&b,1) < 0) return 1;
		dprintf(1,"reading...\n");
		if (s->can_read(s,0x307,data,8) == 0) {
			if (debug >= 3) bindump("data",data,8);
			dprintf(1,"*** data[3] & 2: %d\n", data[3] & 0x0002);
			if (op) {
				if (data[3] & 0x0002) return 0;
			} else {
				if ((data[3] & 0x0002) == 0) return 0;
			}
		}
		sleep(1);
	}
	if (retries < 0) printf("start failed.\n");
	return (retries < 0 ? 1 : 0);
}

static void getconf(si_session_t *s) {
	cfg_proctab_t mytab[] = {
		{ "si", "readonly", 0, DATA_TYPE_LOGICAL, &s->readonly, 0, "N" },
		{ "si", "soc", 0, DATA_TYPE_FLOAT, &s->user_soc, 0, "-1" },
		{ "si", "charge_min_amps", 0, DATA_TYPE_FLOAT, &s->charge_min_amps, 0, "-1" },
		{ "si", "charge_focus_amps", "Increase charge voltage in order to maintain charge amps", DATA_TYPE_BOOL, &s->charge_creep, 0, "Y" },
		{ "si", "sim_step", 0, DATA_TYPE_FLOAT, &s->sim_step, 0, ".1" },
		{ "si", "interval", 0, DATA_TYPE_INT, &s->interval, 0, "10" },
		CFG_PROCTAB_END
	};

        cfg_get_tab(s->ap->cfg,mytab);
        if (debug) cfg_disp_tab(mytab,"si",1);
}

int main(int argc, char **argv) {
	opt_proctab_t opts[] = {
		OPTS_END
	};
	solard_agent_t *ap;
	si_session_t *s;
#if 0
	char *args[] = { "t2", "-d", "5", "-c", "si.conf" };
	#define nargs (sizeof(args)/sizeof(char *))
	argv = args;
	argc = nargs;
#endif

	ap = agent_init(argc,argv,opts,&si_driver);
	dprintf(1,"ap: %p\n",ap);
	if (!ap) return 1;

	/* Get our session */
	s = agent_get_driver_handle(ap);

	/* Read our specific config */
	getconf(s);

	/* Readonly? */
	dprintf(1,"readonly: %d\n", s->readonly);
	if (s->readonly)
		si_driver.write  = 0;
	else
		charge_init(s);

	/* Read and write intervals are 10s */
	ap->read_interval = ap->write_interval = s->interval;

#if 1
	{
		list lp;
		lp = list_create();
//#define CV "charge_voltage"
#define CV "ComBaud"
		list_add(lp,CV,strlen(CV)+1);
		si_config(s, "Get", "main", lp);
	}
#endif

	s->startup = 1;

	/* Go */
	log_write(LOG_INFO,"Agent Running!\n");
	agent_run(ap);
	return 0;
}

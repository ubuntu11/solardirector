
/*
Copyright (c) 2021, Stephen P. Shoecraft
All rights reserved.

This source code is licensed under the BSD-style license found in the
LICENSE file in the root directory of this source tree.
*/

#include "si.h"
#include <pthread.h>
#include "transports.h"
#include "roles.h"

#define TESTING 0
#define CONFIG_FROM_MQTT 0

char *si_agent_version_string = "1.0";

#if 0
solard_driver_t *si_transports[] = { &can_driver, &serial_driver, &rdev_driver, 0 };

static void *si_new(void *conf, void *driver, void *driver_handle) {
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

	dprintf(1,"returning: %p\n", s);
	return s;
si_new_error:
	free(s);
	return 0;
}

static int si_open(si_session_t *s) {
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

static int si_close(void *handle) {
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

solard_driver_t si_driver = {
	SOLARD_DRIVER_AGENT,
	"si",
	si_new,				/* New */
	(solard_driver_open_t)si_open,			/* Open */
	(solard_driver_close_t)si_close,			/* Close */
	(solard_driver_read_t)si_read,			/* Read */
	(solard_driver_write_t)si_write,			/* Write */
	si_config,			/* Config */
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
#endif

#if 0
static void getconf(si_session_t *s) {
	int from_mqtt;
	cfg_proctab_t mytab[] = {
		{ "si", "config_from_mqtt", 0, DATA_TYPE_LOGICAL, &from_mqtt, 0, "N" },
		{ "si", "readonly", 0, DATA_TYPE_LOGICAL, &s->readonly, 0, "N" },
		{ "si", "soc", 0, DATA_TYPE_FLOAT, &s->user_soc, 0, "-1" },
		{ "si", "startup_charge_mode", 0, DATA_TYPE_INT, &s->startup_charge_mode, 0, "1" },
		{ "si", "charge_min_amps", 0, DATA_TYPE_FLOAT, &s->charge_min_amps, 0, "-1" },
		{ "si", "charge_creep", "Increase charge voltage in order to maintain charge amps", DATA_TYPE_BOOL, &s->charge_creep, 0, "yes" },
		{ "si", "sim_step", 0, DATA_TYPE_FLOAT, &s->sim_step, 0, ".1" },
		{ "si", "interval", 0, DATA_TYPE_INT, &s->interval, 0, "10" },
		CFG_PROCTAB_END
	};
#if CONFIG_FROM_MQTT
	char topic[SOLARD_TOPIC_SIZE];
	solard_message_t *msg;
	list lp;
#endif

	from_mqtt = 0;
        cfg_get_tab(s->ap->cfg,mytab);
        if (debug) cfg_disp_tab(mytab,"si",1);
	dprintf(1,"fn: %p\n", s->ap->cfg->filename);
	if (s->ap->cfg->filename) return;

#if CONFIG_FROM_MQTT
	/* Sub to our config topic */
 //       sprintf(topic,"%s/%s/%s/%s",SOLARD_TOPIC_ROOT,SOLARD_ROLE_INVERTER"/%s/"SOLARD_FUNC_CONFIG"/+",s->ap->instance_name);
	agent_mktopic(topic,sizeof(topic)-1,s->ap,s->name,SOLARD_FUNC_CONFIG);
        if (mqtt_sub(s->ap->m,topic)) return;

	/* Ingest any config messages */
	dprintf(1,"ingesting...\n");
	solard_ingest(s->ap->mq,2);
	dprintf(1,"back from ingest...\n");

	/* Process messages as config requests */
	lp = list_create();
	dprintf(1,"mq count: %d\n", list_count(s->ap->mq));
	list_reset(s->ap->mq);
	while((msg = list_get_next(s->ap->mq)) != 0) {
		memset(&req,0,sizeof(req));
		strncat(req.name,msg->param,sizeof(req.name)-1);
		req.type = DATA_TYPE_STRING;
		dprintf(1,"data len: %d\n", msg->data_len);
		strncat(req.sval,msg->data,sizeof(req.sval)-1);
		dprintf(1,"req: name: %s, type: %d(%s), sval: %s\n", req.name, req.type, typestr(req.type), req.sval);
		list_add(lp,&req,sizeof(req));
		list_delete(s->ap->mq,msg);
	}
	dprintf(1,"lp count: %d\n", list_count(lp));
	if (list_count(lp)) si_config(s,"Set","si_control",lp);
	list_destroy(lp);
	mqtt_unsub(s->ap->m,topic);
#endif

	dprintf(1,"done\n");
	return;

}
#endif

int main(int argc, char **argv) {
	solard_agent_t *ap;
//	si_session_t *s;
#if TESTING
	char *args[] = { "si", "-d", "6", "-c", "sitest.conf" };
	argc = (sizeof(args)/sizeof(char *));
	argv = args;
#endif

        log_write(LOG_INFO,"SMA Sunny Island Agent version %s\n",si_agent_version_string);
        log_write(LOG_INFO,"Starting up...\n");

	ap = agent_init(argc,argv,0,&si_driver,si_transports,&inverter_driver);
	dprintf(1,"ap: %p\n",ap);
	if (!ap) return 1;

#if 0
	/* Get our session */
	if (ap->role->config(ap->role_handle,SOLARD_CONFIG_GET_HANDLE,&s)) return 1;

	/* Read our specific config */
	getconf(s);

	/* Readonly? */
	dprintf(1,"readonly: %d\n", s->readonly);
	if (s->readonly)
		si_driver.write  = 0;
	else
		charge_init(s);

	/* Read and write intervals are the same */
	ap->read_interval = ap->write_interval = s->interval;
	s->startup = 1;
#endif

	/* Go */
	log_write(LOG_INFO,"Agent Running!\n");
	agent_run(ap);
	return 0;
}

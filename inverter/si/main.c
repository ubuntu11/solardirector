
/*
Copyright (c) 2021, Stephen P. Shoecraft
All rights reserved.

This source code is licensed under the BSD-style license found in the
LICENSE file in the root directory of this source tree.
*/

#include "si.h"
#include <pthread.h>

int si_open(si_session_t *s);

void *si_new(void *conf, void *driver, void *driver_handle) {
	si_session_t *s;

	s = calloc(1,sizeof(*s));
	if (!s) {
		perror("si_new: calloc");
		return 0;
	}
	s->conf = conf;
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
		s->get_data = si_get_local_can_data;
	} else {
		dprintf(1,"setting func to remote data\n");
		s->get_data = si_get_remote_can_data;
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
	0,				/* Control */
	si_config,			/* Config */
	0,				/* Get handle */
	0,				/* Get private */
};

int si_callback(solard_agent_t *ap) {
	si_session_t *s = agent_get_driver_handle(ap);
	solard_inverter_t *inv = ap->role_data;
	dprintf(1,"s: %p\n", s);

#if 0
	conf->battery_temp = packs_reported ? (pack_temp_total / packs_reported) : (inv_reported ? inv->battery_temp : 25.0);
	if (packs_reported && (conf->use_packv || !inv_reported)) {
		conf->battery_voltage = pack_voltage_total / packs_reported;
		conf->battery_amps = pack_current_total / packs_reported;
	} else if (inv_reported) {
		conf->battery_voltage = inv->battery_voltage;
		conf->battery_amps = inv->battery_amps;
	}
#endif
#if 0
	/* XXX SIM */
	if (!(int)conf->tvolt) conf->tvolt = conf->battery_voltage;
	if (mybmm_check_state(conf,MYBMM_CHARGING)) {
		if (conf->charge_mode == 1) conf->tvolt += 1;
		else if (conf->charge_mode == 2) conf->cv_start_time -= (30 * 60);
	} else {
		conf->tvolt -= 1;
	}
	conf->battery_voltage = conf->tvolt;

	/* Charge */
	dprintf(2,"user_charge_voltage: %.1f, user_charge_amps: %.1f\n", s->user_charge_voltage, s->user_charge_amps);
	inv->charge_voltage = s->user_charge_voltage < 0.0 ? s->charge_voltage : s->user_charge_voltage;
	inv->charge_amps = s->user_charge_amps < 0.0 ? s->charge_amps : s->user_charge_amps;
	lprintf(0,"Charge voltage: %.1f, Charge amps: %.1f\n", inv->charge_voltage, inv->charge_amps);

	/* Discharge */
	dprintf(2,"user_discharge_voltage: %.1f, user_discharge_amps: %.1f\n",inv->user_discharge_voltage,inv->user_discharge_amps);
	inv->discharge_voltage = s->user_discharge_voltage < 0.0 ? s->discharge_voltage : s->user_discharge_voltage;
	inv->discharge_amps = s->user_discharge_amps < 0.0 ? inv->e_rate * inv->capacity : inv->user_discharge_amps;
	lprintf(0,"Discharge voltage: %.1f, Discharge amps: %.1f\n", inv->discharge_voltage, inv->discharge_amps);



	lprintf(0,"Battery: voltage: %.1f, current: %.1f, temp: %.1f\n", inv->battery_voltage, inv->battery_amps, inv->battery_temp);
#endif

	inv->soc = s->soc < 0.0 ? ( ( inv->battery_voltage - inv->discharge_voltage) / (inv->charge_voltage - inv->discharge_voltage) ) * 100.0 : s->soc;
	lprintf(0,"SoC: %.1f\n", inv->soc);
	inv->soh = 100.0;
	return 0;
}

int main(int argc, char **argv) {
	opt_proctab_t opts[] = {
		OPTS_END
	};
	solard_agent_t *ap;
	char *args[] = { "t2", "-d", "7", "-c", "si.conf" };
	#define nargs (sizeof(args)/sizeof(char *))
	si_session_t *s;

//	si_driver.read = 0;
//	si_driver.info = 0;

	ap = agent_init(nargs,args,opts,&si_driver);
//	ap = agent_init(argc,argv,opts,&si_driver);
	dprintf(1,"ap: %p\n",ap);
	if (!ap) return 1;

	/* Read and write intervals are 10s */
	ap->read_interval = ap->write_interval = 5;

	/* Get our session */
	s = agent_get_driver_handle(ap);

	/* Do we have RS485? */
	if (load_tp_from_cfg(&s->tty,&s->tty_handle,ap->cfg,"config")) return 1;

#if 1
	{
		list lp;
		lp = list_create();
//#define CV "charge_voltage"
#define CV "AutoStr"
		list_add(lp,CV,strlen(CV)+1);
		si_config(s, "Get", "main", lp);
	}
#endif

	/* Set startup */
	solard_set_state(s,SI_STATE_STARTUP);

	/* Go */
	agent_run(ap);
	return 0;
}

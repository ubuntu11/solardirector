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
	s->can = driver;
	s->can_handle = driver_handle;

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
	0,			/* info */
	(solard_module_open_t)si_open,			/* Open */
	(solard_module_read_t)si_read,			/* Read */
	(solard_module_write_t)si_write,			/* Write */
	(solard_module_close_t)si_close,			/* Close */
	0,				/* Free */
	0,				/* Shutdown */
	0,				/* Control */
	si_config			/* Config */
};

int si_callback(solard_agent_t *ap, void *info) {
#if 0
	solard_inverter_t *inv = info;
	si_session_t *s = agent_get_handle(ap);
	dprintf(1,"s: %p\n", s);
	if (!(int)conf->capacity) {
		/* For capacity, if user capacity is set, use that.  */
		if (conf->user_capacity > 0.0) {
			conf->capacity = conf->user_capacity;
			repcap = 1;
		/* otherwise if we have packs and all packs have reported in, use total */
		} else if (npacks && packs_reported == npacks) {
			conf->capacity = pack_capacity_total;
			repcap = 1;
		}
	}
	if (repcap) {
		lprintf(0,"Capacity: %.1f\n", conf->capacity);
		conf->kwh = (conf->capacity * conf->system_voltage) / 1000.0;
		lprintf(0,"kWh: %.1f\n", conf->kwh);
		repcap = 0;
	}

	conf->battery_temp = packs_reported ? (pack_temp_total / packs_reported) : (inv_reported ? inv->battery_temp : 25.0);
	if (packs_reported && (conf->use_packv || !inv_reported)) {
		conf->battery_voltage = pack_voltage_total / packs_reported;
		conf->battery_amps = pack_current_total / packs_reported;
	} else if (inv_reported) {
		conf->battery_voltage = inv->battery_voltage;
		conf->battery_amps = inv->battery_amps;
	}
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
#endif

        conf->charge_voltage = conf->user_charge_voltage < 0.0 ? conf->cell_high * conf->cells : conf->user_charge_voltage;

        dprintf(2,"user_discharge_voltage: %.1f, user_discharge_amps: %.1f\n", conf->user_discharge_voltage, conf->user_discharge_amps);
        conf->discharge_voltage = conf->user_discharge_voltage < 0.0 ? conf->cell_low * conf->cells : conf->user_discharge_voltage;
        dprintf(2,"conf->e_rate: %f, conf->capacity: %f\n", conf->e_rate, conf->capacity);
        conf->discharge_amps = conf->user_discharge_amps < 0.0 ? conf->e_rate * conf->capacity : conf->user_discharge_amps;
        lprintf(0,"Discharge voltage: %.1f, Discharge amps: %.1f\n", conf->discharge_voltage, conf->discharge_amps);

        dprintf(2,"user_charge_voltage: %.1f, user_charge_amps: %.1f\n", conf->user_charge_voltage, conf->user_charge_amps);
        conf->charge_voltage = conf->charge_target_voltage = conf->user_charge_voltage < 0.0 ? conf->cell_high * conf->cells : conf->user_charge_voltage;
        dprintf(2,"conf->c_rate: %f, conf->capacity: %f\n", conf->c_rate, conf->capacity);
        conf->charge_amps = conf->user_charge_amps < 0.0 ? conf->c_rate * conf->capacity : conf->user_charge_amps;
        lprintf(0,"Charge voltage: %.1f, Charge amps: %.1f\n", conf->charge_voltage, conf->charge_amps);


		lprintf(0,"Battery: voltage: %.1f, current: %.1f, temp: %.1f\n", conf->battery_voltage, conf->battery_amps, conf->battery_temp);

		conf->soc = conf->user_soc < 0.0 ? ( ( conf->battery_voltage - conf->discharge_voltage) / (conf->charge_voltage - conf->discharge_voltage) ) * 100.0 : conf->user_soc;
		lprintf(0,"SoC: %.1f\n", conf->soc);
		conf->soh = 100.0;

		charge_check(conf);
#endif
	return 0;
}

int main(int argc, char **argv) {
	opt_proctab_t opts[] = {
		OPTS_END
	};
	solard_agent_t *ap;
	char *args[] = { "t2", "-d", "2", "-c", "si.conf", "-i", "5" };
	#define nargs (sizeof(args)/sizeof(char *))

//	si_driver.read = 0;
//	si_driver.info = 0;

//	ap = agent_init(argc,argv,opts,&si_driver);
	ap = agent_init(nargs,args,opts,&si_driver);
	dprintf(1,"ap: %p\n",ap);
	if (!ap) return 1;
	mqtt_disconnect(ap->m,0);
	ap->read_interval = ap->write_interval = 10;
	ap->rcbw = si_callback;
	agent_run(ap);
	return 0;
}


/*
Copyright (c) 2021, Stephen P. Shoecraft
All rights reserved.

This source code is licensed under the BSD-style license found in the
LICENSE file in the root directory of this source tree.
*/

#include "si.h"

#define HOURS(n) (n * 3600)

void charge_init(si_session_t *s) {
	solard_inverter_t *inv = s->ap->role_data;

	if (inverter_check_parms(inv)) {
		log_write(LOG_ERROR,"%s\n", inv->errmsg);
		charge_stop(s,0);
		return;
	}
	s->charge_voltage = inv->charge_end_voltage;
	s->charge_amps = s->charge_min_amps > 0.1 ? s->charge_min_amps : 0.1;
	solard_clear_state(s,SI_STATE_CHARGING);
	s->charge_mode = 0;
}

void charge_max_start(si_session_t *s) {
	solard_inverter_t *inv = s->ap->role_data;

	if (inverter_check_parms(inv)) {
		log_write(LOG_ERROR,"%s\n", inv->errmsg);
		charge_stop(s,0);
		return;
	}
	log_write(LOG_INFO,"*** CHARGING AT MAX ***\n");
	s->charge_voltage = inv->max_voltage;
	inv->charge_at_max = 1;
}

void charge_max_stop(si_session_t *s) {
	solard_inverter_t *inv = s->ap->role_data;

	if (inverter_check_parms(inv)) {
		log_write(LOG_ERROR,"%s\n", inv->errmsg);
		charge_stop(s,0);
		return;
	}
	log_write(LOG_INFO,"*** CHARGING AT END ***\n");
	s->charge_voltage = inv->charge_end_voltage;
	inv->charge_at_max = 0;
}

void charge_stop(si_session_t *s, int rep) {
	solard_inverter_t *inv = s->ap->role_data;

	if (!solard_check_state(s,SI_STATE_CHARGING)) return;

	if (rep) log_write(LOG_INFO,"*** ENDING CHARGE ***\n");
	solard_clear_state(s,SI_STATE_CHARGING);
	s->charge_mode = 0;

	/* Need this AFTER ending charge */
	if (inverter_check_parms(inv)) {
		log_write(LOG_ERROR,"%s\n",inv->errmsg);
		return;
	}

	dprintf(1,"charge_end_voltage: %.1f, charge_min_amps: %.1f\n", inv->charge_end_voltage, s->charge_min_amps);
	s->charge_voltage = inv->charge_end_voltage;
	s->charge_amps = s->charge_min_amps > 0.1 ? s->charge_min_amps : 0.1;
}

void charge_start(si_session_t *s, int rep) {
	solard_inverter_t *inv = s->ap->role_data;

	if (!s->bits.Run) return;

	if (solard_check_state(s,SI_STATE_CHARGING)) return;
	if (inverter_check_parms(inv)) {
		log_write(LOG_ERROR,"%s\n",inv->errmsg);
		charge_stop(s,0);
		return;
	}

	if (s->have_battery_temp && inv->battery_temp <= 0.0) {
		log_write(LOG_WARNING,"battery_temp <= 0.0, not starting charge\n");
		return;
	}

	if (inv->charge_at_max) charge_max_start(s);
	else s->charge_voltage = inv->charge_end_voltage;

	dprintf(1,"inv->charge_amps: %.1f\n", inv->charge_amps);
	s->charge_amps = inv->charge_amps;
	dprintf(1,"charge_voltage: %.1f, charge_amps: %.1f\n", s->charge_voltage, s->charge_amps);

	if (rep) log_write(LOG_INFO,"*** STARTING CC CHARGE ***\n");
	solard_set_state(s,SI_STATE_CHARGING);
	s->charge_mode = 1;
	s->tvolt = inv->battery_voltage;
	s->start_temp = inv->battery_temp;
}

static void cvremain(time_t start, time_t end) {
	int diff,hours,mins;

	diff = (int)difftime(end,start);
	dprintf(1,"start: %ld, end: %ld, diff: %d\n", start, end, diff);
	if (diff > 0) {
		hours = diff / 3600;
		if (hours) diff -= (hours * 3600);
		dprintf(1,"hours: %d, diff: %d\n", hours, diff);
		mins = diff / 60;
		if (mins) diff -= (mins * 60);
		dprintf(1,"mins: %d, diff: %d\n", mins, diff);
	} else {
		hours = mins = diff = 0;
	}
	log_write(LOG_INFO,"CV Time remaining: %02d:%02d:%02d\n",hours,mins,diff);
}

void charge_start_cv(si_session_t *s, int rep) {
	solard_inverter_t *inv = s->ap->role_data;

	if (!s->bits.Run) return;

//	charge_stop(s,0);
	if (inverter_check_parms(inv)) {
		log_write(LOG_ERROR,"%s\n",inv->errmsg);
		charge_stop(s,0);
		return;
	}
	if (rep) log_write(LOG_INFO,"*** STARTING CV CHARGE ***\n");
	solard_set_state(s,SI_STATE_CHARGING);
	s->charge_mode = 2;
	s->charge_voltage = inv->charge_end_voltage;
	s->charge_amps = inv->charge_amps;
	time(&s->cv_start_time);
	s->start_temp = inv->battery_temp;
}

int charge_control(si_session_t *s, int mode, int rep) {
	switch(mode) {
	case 0:
		charge_stop(s,rep);
		break;
	case 1:
		charge_start(s,rep);
		break;
	case 2:
		charge_start_cv(s,rep);
		break;
	default:
		return 1;
	}
	return 0;
}

float pct(float val1, float val2) {
	float val;

	dprintf(1,"val1: %.3f, val: %.3f\n", val1, val2);
	val = 100 - ((val2 / val1)  * 100.0);
	dprintf(1,"val: %.3f\n", val);
	return val;
}

static void incvolt(si_session_t *s, solard_inverter_t *inv) {
	float battery_amps;

	battery_amps = inv->battery_amps * -1;;
	dprintf(0,"battery_amps: %.1f, charge_amps: %.1f, pct: %f\n", battery_amps, s->charge_amps, pct(battery_amps,s->charge_amps));
//	if (battery_amps >= s->charge_amps) return;
	if (pct(battery_amps,s->charge_amps) > -5.0) return;
	dprintf(0,"frequency: %.1f\n", inv->load_frequency);
	if (inv->load_frequency > 0.0 && (inv->load_frequency < 50.0 || inv->load_frequency < 60.0)) return;
	dprintf(0,"charge_voltage: %.1f, max_voltage: %.1f\n", s->charge_voltage, inv->max_voltage);
	if ((s->charge_voltage + 0.1) >= inv->max_voltage) return;
	s->charge_voltage += 0.1;
	if (s->charge_voltage > inv->max_voltage) {
		log_write(LOG_ERROR,"charge_voltage > max_voltage!!!\n");
		s->charge_voltage = inv->max_voltage;
	}
}

void charge_check(si_session_t *s) {
	solard_inverter_t *inv = s->ap->role_data;
	time_t current_time;

//	if (!s->bits.Run) return;
	dprintf(1,"battery_voltage: %f, charge_start_voltage: %f\n", inv->battery_voltage, inv->charge_start_voltage);
	if ((inv->battery_voltage-0.0001) <= inv->charge_start_voltage) {
		/* Start charging */
		charge_start(s,1);
	}

	time(&current_time);
	if (solard_check_state(s,SI_STATE_CHARGING)) {
		if (s->have_battery_temp) {
			/* If battery temp is <= 0, stop charging immediately */
			dprintf(1,"battery_temp: %2.1f\n", inv->battery_temp);
			if (inv->battery_temp <= 0.0) {
				charge_stop(s,1);
				return;
			}
			/* If battery temp <= 5C, reduce charge rate by 1/4 */
			if (inv->battery_temp <= 5.0) s->charge_amps /= 4.0;

			/* Watch for rise in battery temp, anything above 5 deg C is an error */
			/* We could lower charge amps until temp goes down and then set that as max amps */
			if (pct(inv->battery_temp,s->start_temp) > 5) {
				dprintf(0,"ERROR: current_temp: %.1f, start_temp: %.1f\n", inv->battery_temp, s->start_temp);
				charge_stop(s,1);
				return;
			}
		}

		/* CC */
		if (s->charge_mode == 1) {
			dprintf(1,"battery_voltage: %f, inv->charge_end_voltage: %f\n",
				inv->battery_voltage, inv->charge_end_voltage);
			dprintf(1,"charge_at_max: %d, charge_creep: %d\n", inv->charge_at_max, s->charge_creep);
			if ((inv->battery_voltage+0.0001) >= inv->charge_end_voltage) {
				charge_start_cv(s,1);
			} else if (!inv->charge_at_max && s->charge_creep) {
				incvolt(s,inv);
			}

		/* CV */
		} else if (s->charge_mode == 2) {
			time_t end_time = s->cv_start_time + HOURS(2);

			/* End saturation mode after 2 hours */
			cvremain(current_time,end_time);
			if (current_time >= end_time) {
				charge_stop(s,1);
			}
		}
	}
}

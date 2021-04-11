
#if 0
#include "si.h"

#define HOURS(n) (n * 3600)

#if 0
int battery_init(si_session_t *s) {
	/* These values are probably wrong and I'll need to revisit this */
	switch(s->battery_chem) {
	default:
	case BATTERY_CHEM_LITHIUM:
		dprintf(1,"battery_chem: LITHIUM\n");
		if (s->cell_low < 0) s->cell_low = 3.0;
		if (s->cell_crit_low < 0) s->cell_crit_low = 2.95;
		if (s->cell_high < 0) s->cell_high = 4.2;
		if (s->cell_crit_high < 0) s->cell_crit_high = 4.25;
		if (s->c_rate < 0) s->c_rate = .7;
		if (s->e_rate < 0) s->e_rate = 1;
		if (s->cells < 0) s->cells = 14;
		s->cell_min = 2.8;
		s->cell_max = 4.2;
		break;
	case BATTERY_CHEM_LIFEPO4:
		dprintf(1,"battery_chem: LIFEPO4\n");
		if (s->cell_low < 0) s->cell_low = 2.5;
		if (s->cell_crit_low < 0) s->cell_crit_low = 2.0;
		if (s->cell_high < 0) s->cell_high = 3.4;
		if (s->cell_crit_high < 0) s->cell_crit_high = 3.70;
		if (s->c_rate < 0) s->c_rate = 1;
		if (s->e_rate < 0) s->e_rate = 1;
		if (s->cells < 0) s->cells = 16;
		s->cell_min = 2.5;
		s->cell_max = 3.65;
		break;
	case BATTERY_CHEM_TITANATE:
		dprintf(1,"battery_chem: TITANATE\n");
		if (s->cell_low < 0) s->cell_low = 1.81;
		if (s->cell_crit_low < 0) s->cell_crit_low = 1.8;
		if (s->cell_high < 0) s->cell_high = 2.85;
		if (s->cell_crit_high < 0) s->cell_crit_high = 2.88;
		if (s->c_rate < 0) s->c_rate = 1;
		if (s->e_rate < 0) s->e_rate = 10;
		if (s->cells < 0) s->cells = 22;
		s->cell_min = 1.8;
		s->cell_max = 2.85;
		break;
	}
//	s->discharge_min_voltage = s->user_discharge_voltage < 0.0 ? s->cell_min * s->cells : s->user_discharge_max_voltage;
	s->charge_max_voltage = s->user_charge_voltage < 0.0 ? s->cell_max * s->cells : s->user_charge_max_voltage;
	dprintf(1,"charge_max_voltage: %.1f\n", s->charge_max_voltage);
	s->charge_target_voltage = s->cell_high * s->cells;
	dprintf(1,"charge_target_voltage: %.1f\n", s->charge_target_voltage);
	dprintf(1,"cell_low: %.1f\n", s->cell_low);
	dprintf(1,"cell_crit_low: %.1f\n", s->cell_crit_low);
	dprintf(1,"cell_high: %.1f\n", s->cell_high);
	dprintf(1,"cell_crit_high: %.1f\n", s->cell_crit_high);
	dprintf(1,"c_rate: %.1f\n", s->c_rate);
	dprintf(1,"e_rate: %.1f\n", s->e_rate);
	/* Default capacity is 0 if none specified (cant risk) */
	s->capacity = s->user_capacity < 0.0 ? 0 : s->user_capacity;
	charge_start(s,0);
	charge_stop(s,0);
	return 0;
}
#endif

void charge_stop(si_session_t *s, int rep) {
	solard_inverter_t *inv = s->conf->role_data;

	if (rep) lprintf(0,"*** ENDING CHARGE ***\n");
	solard_clear_state(s,SI_STATE_CHARGING);
//	s->charge_amps = 0.1;
	s->charge_amps = 120;
//	s->charge_voltage = s->user_charge_voltage < 0.0 ? s->cell_high * s->cells : s->user_charge_voltage;

	inv->charge_voltage = s->charge_voltage;

//	dprintf(2,"user_discharge_voltage: %.1f, user_discharge_amps: %.1f\n", s->user_discharge_voltage, s->user_discharge_amps);
//	s->discharge_voltage = s->user_discharge_voltage < 0.0 ? s->cell_low * s->cells : s->user_discharge_voltage;
	inv->discharge_voltage = s->discharge_voltage;
//	dprintf(2,"s->e_rate: %f, s->capacity: %f\n", s->e_rate, s->capacity);
//	s->discharge_amps = s->user_discharge_amps < 0.0 ? s->e_rate * s->capacity : s->user_discharge_amps;
	inv->discharge_amps = s->discharge_amps;
	if (rep) lprintf(0,"Discharge voltage: %.1f, Discharge amps: %.1f\n", s->discharge_voltage, s->discharge_amps);
}

void charge_start(si_session_t *s, int rep) {
	solard_invertter_t *inv = s->conf->role_data;

	/* Do not start the charge until we have a capacity */
	if (rep) lprintf(0,"*** STARTING CC CHARGE ***\n");
	solard_set_state(s,SI_STATE_CHARGING);
//	dprintf(2,"user_charge_voltage: %.1f, user_charge_amps: %.1f\n", s->user_charge_voltage, s->user_charge_amps);
//	s->charge_voltage = s->charge_target_voltage = s->user_charge_voltage < 0.0 ? s->cell_high * s->cells : s->user_charge_voltage;
	inv->charge_voltage = s->charge_voltage;
//	dprintf(2,"s->c_rate: %f, s->capacity: %f\n", s->c_rate, s->capacity);
//	s->charge_amps = s->user_charge_amps < 0.0 ? s->c_rate * s->capacity : s->user_charge_amps;
	inv->charge_amps = s->charge_amps;
	if (rep) lprintf(0,"Charge voltage: %.1f, Charge amps: %.1f\n", s->charge_voltage, s->charge_amps);
	s->charge_mode = 1;
	s->tvolt = s->battery_voltage;
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
	lprintf(0,"CV Time remaining: %02d:%02d:%02d\n",hours,mins,diff);
}

void charge_check(si_session_t *s) {
	time_t current_time;

	time(&current_time);
	if (solard_check_state(s,SOLARD_CHARGING)) {
		/* If battery temp is <= 0, stop charging immediately */
		if (s->battery_temp <= 0.0) {
			charge_stop(s,1);
			return;
		}
		/* If battery temp <= 5C, reduce charge rate by 1/4 */
		if (s->battery_temp <= 5.0) s->charge_amps /= 4.0;

		/* Watch for rise in battery temp, anything above 5 deg C is an error */
		/* We could lower charge amps until temp goes down and then set that as max amps */
		if (pct(s->battery_temp,s->start_temp) > 5) {
			dprintf(0,"ERROR: current_temp: %.1f, start_temp: %.1f\n", s->battery_temp, s->start_temp);
			charge_stop(s,1);
			return;
		}

		/* CC */
		if (s->charge_mode == 1) {
			dprintf(1,"battery_amps: %.1f, charge_amps: %.1f, battery_voltage: %.1f, charge_target_voltage: %.1f, charge_voltage: %.1f, charge_max_voltage: %.1f\n", s->battery_amps,s->charge_amps,s->battery_voltage,s->charge_target_voltage,s->charge_voltage,s->charge_max_voltage);

			if (s->battery_voltage > s->charge_target_voltage || s->pack_cell_max >= s->cell_max) {
				time(&s->cv_start_time);
				s->charge_mode = 2;
				s->charge_voltage = s->user_charge_voltage < 0.0 ? s->cell_high * s->cells : s->user_charge_voltage;
				lprintf(0,"*** STARTING CV CHARGE ***\n");
			} else if (s->battery_amps < s->charge_amps && s->charge_voltage < s->charge_max_voltage) {
				s->charge_voltage += 0.1;
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
	} else {
		/* Discharging */
		dprintf(1,"battery_voltage: %.1f, discharge_voltage: %.1f, pack_cell_min: %.3f, cell_low: %.3f\n",
			s->battery_voltage, s->discharge_voltage, s->pack_cell_min, s->cell_low);
		if (s->battery_voltage < s->discharge_voltage || s->pack_cell_min <= s->cell_min) {
			/* Start charging */
			charge_start(s,1);
		}
	}
}
#endif

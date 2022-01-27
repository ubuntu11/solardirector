
/*
Copyright (c) 2021, Stephen P. Shoecraft
All rights reserved.

This source code is licensed under the BSD-style license found in the
LICENSE file in the root directory of this source tree.
*/

#include "si.h"

#define HOURS(n) (n * 3600)
#define MINUTES(n) (n * 60)

//#define isvrange(v) ((v >= 1.0) && (v  <= 1000.0))

int si_check_config(si_session_t *s) {
	int r;
	char *msg;
	float spread;

	dprintf(1,"min_voltage: %.1f, max_voltage: %.1f\n", s->min_voltage, s->max_voltage);
	if (!si_isvrange(s->min_voltage) && !fequal(s->min_voltage,0.0)) {
		log_warning("min_voltage (%.1f) out of range(%.1f to %.1f)\n",
			s->min_voltage, SI_VOLTAGE_MIN, SI_VOLTAGE_MAX);
		s->min_voltage = 0.0;
	} else if (s->min_voltage >= s->max_voltage) {
		log_warning("min_voltage >= max_voltage\n");
		s->min_voltage = 0.0;
	}
	if (!si_isvrange(s->max_voltage) && !fequal(s->max_voltage,0.0)) {
		log_warning("max_voltage (%.1f) out of range(%.1f to %.1f)\n",
			s->max_voltage, SI_VOLTAGE_MIN, SI_VOLTAGE_MAX);
		s->max_voltage = 0.0;
	} else if (s->max_voltage <= s->min_voltage) {
		log_warning("min_voltage >= max_voltage\n");
		s->max_voltage = 0.0;
	}
	if (fequal(s->min_voltage,0.0)) {
		log_warning("setting min_voltage to %.1f\n", 41.0);
		s->min_voltage = 41.0;
	}
	if (fequal(s->max_voltage,0.0)) {
		log_warning("setting max_voltage to %.1f\n", 58.1);
		s->max_voltage = 58.1;
	}

#if 0
	/* min and max must be set */
	if (!si_isvrange(s->min_voltage)) {
		msg = "min_voltage not set or out of range\n";
		goto si_check_config_error;
	}
	if (!si_isvrange(s->max_voltage)) {
		msg = "max_voltage not set or out of range\n";
		goto si_check_config_error;
	}
	/* min must be < max */
	if (s->min_voltage >= s->max_voltage) {
		msg = "min_voltage > max_voltage\n";
		goto si_check_config_error;
	}
	/* max must be > min */
	if (s->max_voltage <= s->min_voltage) {
		msg = "min_voltage > max_voltage\n";
		goto si_check_config_error;
	}
#endif

#if 0
	if (fequal(s->charge_start_soc,0.0)) {
		log_warning("setting charge_start_soc to 25\n");
		s->charge_start_soc = 85.0;
	}
#endif

	spread = s->max_voltage - s->min_voltage;
	if (!fequal(s->charge_start_voltage,0.0)) {
		if (!si_isvrange(s->charge_start_voltage)) {
			log_warning("charge_start_voltage (%.1f) out of range(%.1f to %.1f)\n",
				s->charge_start_voltage, SI_VOLTAGE_MIN, SI_VOLTAGE_MAX);
			s->charge_start_voltage = 0.0;
		} else if (s->charge_start_voltage >= s->charge_end_voltage) {
			log_warning("charge_start_voltage >= charge_end_voltage");
			s->charge_start_voltage = 0.0;
		} else if (s->charge_start_voltage < s->min_voltage) {
			log_warning("charge_start_voltage < min_voltage");
			s->charge_start_voltage = 0.0;
		} else if (s->charge_start_voltage > s->max_voltage) {
			log_warning("charge_start_voltage > max_voltage");
			s->charge_start_voltage = 0.0;
		}
	}
	if (fequal(s->charge_start_voltage,0.0)) {
		s->charge_start_voltage = s->min_voltage + (spread * 0.25);
		log_warning("setting charge_start_voltage to %.1f\n", s->charge_start_voltage);
	}
	if (!fequal(s->charge_end_voltage,0.0)) {
		if (!si_isvrange(s->charge_end_voltage)) {
			log_warning("charge_end_voltage (%.1f) out of range(%.1f to %.1f)\n",
				s->charge_end_voltage, SI_VOLTAGE_MIN, SI_VOLTAGE_MAX);
			s->charge_end_voltage = 0.0;
		} else if (s->charge_end_voltage <= s->charge_start_voltage) {
			log_warning("charge_end_voltage <= charge_start_voltage");
			s->charge_end_voltage = 0.0;
		} else if (s->charge_end_voltage < s->min_voltage) {
			log_warning("charge_end_voltage < min_voltage");
			s->charge_end_voltage = 0.0;
		} else if (s->charge_end_voltage > s->max_voltage) {
			log_warning("charge_end_voltage > max_voltage");
			s->charge_end_voltage = 0.0;
		}
	}
	if (fequal(s->charge_end_voltage,0.0)) {
		s->charge_end_voltage = s->min_voltage + (spread * 0.85);
		log_warning("setting charge_end_voltage to %.1f\n", s->charge_end_voltage);
	}

#if 0
//	if (!s->readonly) {
		/* charge_start_voltage must be >= min */
		dprintf(1,"charge_start_voltage: %.1f, charge_end_voltage: %.1f\n",
			s->charge_start_voltage, s->charge_end_voltage);
		if (s->charge_start_voltage < s->min_voltage) {
			msg = "charge_start_voltage < min_voltage";
			goto si_check_config_error;
		}
		/* charge_start_voltage must be <= max */
		if (s->charge_start_voltage > s->max_voltage) {
			msg = "charge_start_voltage > max_voltage";
			goto si_check_config_error;
		}
		/* charge_end_voltage must be >= min */
		if (s->charge_end_voltage < s->min_voltage) {
			msg = "charge_end_voltage < min_voltage";
			goto si_check_config_error;
		}
		/* charge_end_voltage must be <= max */
		if (s->charge_end_voltage > s->max_voltage) {
			msg = "charge_end_voltage > max_voltage";
			goto si_check_config_error;
		}
		/* charge_start_voltage must be < charge_end_voltage */
		if (s->charge_start_voltage > s->charge_end_voltage) {
			msg = "charge_start_voltage > charge_end_voltage";
			goto si_check_config_error;
		}
//	}

	r = 0;
	msg = "";

/si_check_config_error:
	strcpy(s->errmsg,msg);
	return r;
#endif
	return 0;
}

void charge_init(si_session_t *s) {
	if (si_check_config(s)) {
		log_write(LOG_ERROR,"%s\n", s->errmsg);
		charge_stop(s,0);
		return;
	}
	s->charge_voltage = s->charge_end_voltage;
	s->charge_amps = s->charge_min_amps;
	s->charge_amps_temp_modifier = 1.0;
	s->charge_amps_soc_modifier = 1.0;
	solard_clear_state(s,SI_STATE_CHARGING);
	s->charge_mode = 0;
	s->force_charge = 0;
	if ((int)s->grid_charge_amps <= 0) s->grid_charge_amps = s->charge_max_amps;
	if ((int)s->gen_charge_amps <= 0) s->gen_charge_amps = s->charge_max_amps;
}

void charge_max_start(si_session_t *s) {
	if (si_check_config(s)) {
		log_write(LOG_ERROR,"%s\n", s->errmsg);
		charge_stop(s,0);
		return;
	}
	log_write(LOG_INFO,"*** CHARGING AT MAX ***\n");
	s->charge_voltage = s->max_voltage;
	s->charge_amps = s->charge_max_amps;
	s->charge_at_max = 1;
}

void charge_max_stop(si_session_t *s) {
	if (si_check_config(s)) {
		log_write(LOG_ERROR,"%s\n", s->errmsg);
		charge_stop(s,0);
		return;
	}
	log_write(LOG_INFO,"*** CHARGING AT END ***\n");
	s->charge_voltage = s->charge_end_voltage;
	s->charge_amps = s->charge_max_amps;
	s->charge_at_max = 0;
}

void charge_stop(si_session_t *s, int rep) {

	if (!solard_check_state(s,SI_STATE_CHARGING)) return;

	if (rep) log_write(LOG_INFO,"*** ENDING CHARGE ***\n");
	solard_clear_state(s,SI_STATE_CHARGING);
	s->charge_mode = 0;

	/* Need this AFTER ending charge */
	if (si_check_config(s)) {
		log_write(LOG_ERROR,"%s\n",s->errmsg);
		return;
	}

	dprintf(1,"charge_end_voltage: %.1f, charge_min_amps: %.1f\n", s->charge_end_voltage, s->charge_min_amps);
	s->charge_voltage = s->charge_end_voltage;
	s->charge_amps = s->charge_min_amps;
	s->force_charge = 0;
}

void charge_start(si_session_t *s, int rep) {

	if (!s->data.Run) return;

	if (solard_check_state(s,SI_STATE_CHARGING)) return;
	if (si_check_config(s)) {
		log_write(LOG_ERROR,"%s\n",s->errmsg);
		charge_stop(s,0);
		return;
	}

	if (s->have_battery_temp && s->data.battery_temp <= 0.0) {
		log_write(LOG_WARNING,"battery_temp <= 0.0, not starting charge\n");
		return;
	}

	if (s->charge_at_max) charge_max_start(s);
	else s->charge_voltage = s->charge_end_voltage;

	s->charge_amps = s->charge_max_amps;
	dprintf(1,"charge_voltage: %.1f, charge_amps: %.1f\n", s->charge_voltage, s->charge_amps);

	if (rep) log_write(LOG_INFO,"*** STARTING CC CHARGE ***\n");
	solard_set_state(s,SI_STATE_CHARGING);
	s->charge_mode = 1;
	s->charge_amps_soc_modifier = 1.0;
	s->charge_amps_temp_modifier = 1.0;
	s->start_temp = s->data.battery_temp;
}

static void cvremain(time_t start, time_t end) {
	int diff,hours,mins;

	diff = (int)difftime(end,start);
	dprintf(6,"start: %ld, end: %ld, diff: %d\n", start, end, diff);
	if (diff > 0) {
		hours = diff / 3600;
		if (hours) diff -= (hours * 3600);
		dprintf(6,"hours: %d, diff: %d\n", hours, diff);
		mins = diff / 60;
		if (mins) diff -= (mins * 60);
		dprintf(6,"mins: %d, diff: %d\n", mins, diff);
	} else {
		hours = mins = diff = 0;
	}
	log_write(LOG_INFO,"CV Time remaining: %02d:%02d:%02d\n",hours,mins,diff);
}

void charge_start_cv(si_session_t *s, int rep) {

	if (!s->data.Run) return;

//	charge_stop(s,0);
	if (si_check_config(s)) {
		log_write(LOG_ERROR,"%s\n",s->errmsg);
		charge_stop(s,0);
		return;
	}
	if (rep) log_write(LOG_INFO,"*** STARTING CV CHARGE ***\n");
	solard_set_state(s,SI_STATE_CHARGING);
	s->charge_mode = 2;
	s->charge_voltage = s->charge_end_voltage;
	s->charge_amps = s->charge_max_amps;
	time(&s->cv_start_time);
	s->start_temp = s->data.battery_temp;
	s->baidx = s->bafull = 0;
}

int charge_control(si_session_t *s, int mode, int rep) {
	switch(mode) {
	default:
	case 0:
		charge_stop(s,rep);
		break;
	case 1:
		charge_start(s,rep);
		break;
	case 2:
		charge_start_cv(s,rep);
		break;
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

static void incvolt(si_session_t *s) {
	float battery_amps;

	battery_amps = s->data.battery_current * -1;;
	dprintf(0,"battery_amps: %.1f, charge_amps: %.1f, pct: %f\n", battery_amps, s->charge_amps, pct(battery_amps,s->charge_amps));
//	if (battery_amps >= s->charge_amps) return;
	if (pct(battery_amps,s->charge_amps) > -5.0) return;
	dprintf(0,"ac1_frequency: %.1f\n", s->data.ac1_frequency);
	if (s->data.ac1_frequency > 0.0 && (s->data.ac1_frequency < 50.0 || s->data.ac1_frequency < 60.0)) return;
	dprintf(0,"charge_voltage: %.1f, max_voltage: %.1f\n", s->charge_voltage, s->max_voltage);
	if ((s->charge_voltage + 0.1) >= s->max_voltage) return;
	s->charge_voltage += 0.1;
	if (s->charge_voltage > s->max_voltage) {
		log_write(LOG_ERROR,"charge_voltage > max_voltage!!!\n");
		s->charge_voltage = s->max_voltage;
	}
}

#if 0
enum EC_STATES {
	EC_STATE_NONE,
	EC_STATE_GENSTART,
	EC_STATE_GENWAITON,
	EC_STATE_GENSTOP,
	EC_STATE_GENWAITOFF,
	EC_STATE_GENRESTORE,
	EC_STATE_GRIDSTART,
	EC_STATE_GRIDWAITON,
	EC_STATE_GRIDSTOP,
	EC_STATE_GRIDWAITOFF,
	EC_STATE_CHARGING,
	EC_STATE_MAX
};

static char *states[] = {
		"None",
		"GenStart",
		"GenWaitOn",
		"GenStop",
		"GenWaitOff",
		"GridStart",
		"GridWaitOn",
		"GridStop",
		"GridWaitOff",
		"Charging"
};

static char *statestr(int state) {
	if (state < EC_STATE_NONE || state >= EC_STATE_MAX)
		return "Unknown";
	else
		return states[state];
}
#endif

void charge_check(si_session_t *s) {
#if 0
	char *p;

ec_again:
	dprintf(1,"state: %d (%s)\n", s->ec_state, statestr(s->ec_state));
	switch(s->ec_state) {
	case EC_STATE_NONE:
		if ((s->data.battery_voltage-0.0001) <= s->charge_start_voltage) {
			/* Start the charge asap */
			charge_start(s,1);

			/* If we have no AC2 freq */
			dprintf(1,"ac2_frequency: %.1f\n", s->data.ac2_frequency);
			if (s->data.ac2_frequency < 2.0) {
				/* Do we have a gen? */
				dprintf(1,"ExtSrc: %s\n", s->ExtSrc);
				if (!strstr(s->ExtSrc,"Gen")) {
					si_notify(s,"ERROR: voltage (%.1f) below EC threshold (%.1f) and no grid/gen!", 
						s->data.battery_voltage, s->charge_start_voltage);
					s->ec_state = EC_STATE_CHARGING;
					goto ec_again;
				} else {
					s->ec_state = EC_STATE_GENSTART;
					goto ec_again;
				}
			} else if (!s->data.GdOn) {
				s->ec_state = EC_STATE_GRIDSTART;
				goto ec_again;
			}
		}
		break;
	case EC_STATE_GENSTART:
		if (s->smanet) {
			/* Save current val */
			if (smanet_get_value(s->smanet,"GnManStr",0,&p) == 0) {
				if (strcmp(p,"---")==0) p = "Auto";
				strncpy(s->gen_save,p,sizeof(s->gen_save)-1);
			} else {
				log_warning("unable to get current value for GnManStr");
				/* Dont error out, try to start it anyway */
				strcpy(s->gen_save,"Auto");
			}
			/* Start the Gen */
			if (smanet_set_value(s->smanet,"GnManStr",0,"Start") == 0) {
				s->ec_state = EC_STATE_GENWAITON;
				time(&s->gen_op_time);
				goto ec_again;
			} else {
				si_notify(s,"ERROR: unable to start Gen!");
				s->ec_state = EC_STATE_CHARGING;
			}
		} else {
			si_notify(s,"WARNING: unable to start Gen for emergency charge!");
			s->ec_state = EC_STATE_CHARGING;
		}
		break;
	case EC_STATE_GENWAITON:
		dprintf(1,"GnRn: %d, ac2_frequency: %.1f\n", s->data.GnRn, s->data.ac2_frequency);
		if (s->data.GnRn && s->data.ac2_frequency > 2.0) {
			/* Gen is running, connect it */
			s->gen_started = 1;
			s->ec_state = EC_STATE_GRIDSTART;
			break;
		} else {
			time_t t;
			int diff;

			/* Timeout expired? */
			time(&t);
			diff = t - s->gen_op_time;
			dprintf(1,"diff: %d, gen_start_timeout: %d\n", diff, s->gen_start_timeout);
			if (diff > s->gen_start_timeout) {
				si_notify(s,"ERROR: timeout waiting for Gen to start!");
				/* XXX Maybe switch to charging? */
				s->ec_state = EC_STATE_GRIDSTART;
			}
		}
		break;
	case EC_STATE_GENSTOP:
		/* Send stop to Gen */
		if (smanet_set_value(s->smanet,"GnManStr",0,"Stop"))
			log_error("unable to stop Generator!");
		/* Restore previous value on next entry */
		s->ec_state = EC_STATE_GENRESTORE;
		break;
	case EC_STATE_GENRESTORE:
		/* Restore GnManStr value */
		if (smanet_set_value(s->smanet,"GnManStr",0,s->gen_save))
			log_error("unable to restore Gen state to: %s\n",s->gen_save);
		if (s->data.GnRn)
			s->ec_state = EC_STATE_GENWAITOFF;
		else
			s->ec_state = EC_STATE_NONE;
		goto ec_again;
		break;
	case EC_STATE_GENWAITOFF:
		dprintf(1,"GnRn: %d\n", s->data.GnRn);
		if (s->data.GnRn) {
			time_t t;
			int diff;

			/* Timeout expired? */
			time(&t);
			diff = t - s->gen_op_time;
			dprintf(1,"diff: %d, gen_stop_timeout: %d\n", diff, s->gen_stop_timeout);
			if (diff > s->gen_stop_timeout) {
				si_notify(s,"ERROR: timeout waiting for Gen to stop!");
				s->ec_state = EC_STATE_NONE;
			}
		}
		break;
	case EC_STATE_GRIDSTART:
		if (s->smanet) {
			if (smanet_get_value(s->smanet,"GdManStr",0,&p) == 0) {
				if (strcmp(p,"---")==0) p = "Auto";
				strncpy(s->grid_save,p,sizeof(s->grid_save)-1);
			} else {
				log_warning("unable to get current value for GdManStr");
				/* Dont error out, try to start it anyway */
				strcpy(s->grid_save,"Auto");
			}
			/* Start the Grid */
			if (smanet_set_value(s->smanet,"GdManStr",0,"Start") == 0) {
				s->ec_state = EC_STATE_GRIDWAITON;
				time(&s->grid_op_time);
				goto ec_again;
			} else {
				si_notify(s,"ERROR: unable to start Grid!");
				s->ec_state = EC_STATE_CHARGING;
			}
		} else {
			si_notify(s,"WARNING: unable to start Grid for emergridcy charge!");
			s->ec_state = EC_STATE_CHARGING;
		}
		break;
	case EC_STATE_GRIDSTOP:
		/* Restore GdManStr value */
		if (smanet_set_value(s->smanet,"GdManStr",0,s->grid_save))
			log_error("unable to restore grid state to: %s\n",s->grid_save);
		/* Dont bother waiting */
		if (s->gen_started)
			s->ec_state = EC_STATE_GENSTOP;
		else
			s->ec_state = EC_STATE_NONE;
		break;
	case EC_STATE_CHARGING:
		/* Has the charge completed? */
		if (s->charge_mode == 0) {
			if (s->gen_started)
				s->ec_state = EC_STATE_GENSTOP;
			else if (s->grid_started)
				s->ec_state = EC_STATE_GRIDSTOP;
			else
				s->ec_state = EC_STATE_NONE;
		}	
		break;
	}
#endif

	if (s->data.battery_voltage < s->min_voltage) return;

	dprintf(1,"battery_voltage: %.1f, charge_start_voltage: %.1f, charge_end_voltage: %.1f\n",
		s->data.battery_voltage, s->charge_start_voltage, s->charge_end_voltage);
	if (s->data.battery_voltage >= s->min_voltage && ((s->data.battery_voltage-0.0001) <= s->charge_start_voltage)) {
		/* Start charging */
		s->force_charge = 1;
		charge_start(s,1);
	}

	if (solard_check_state(s,SI_STATE_CHARGING)) {
		if (s->charge_voltage != s->last_charge_voltage || s->data.battery_current != s->last_battery_amps) {
			lprintf(0,"Charge Voltage: %.1f, Battery Amps: %.1f\n",s->charge_voltage,s->data.battery_current);
			s->last_charge_voltage = s->charge_voltage;
			s->last_battery_amps = s->data.battery_current;
		}
		if (s->have_battery_temp) {
			/* If battery temp is <= 0, stop charging immediately */
			dprintf(1,"battery_temp: %2.1f\n", s->data.battery_temp);
			if (s->data.battery_temp <= 0.0) {
				charge_stop(s,1);
				return;
			}
			/* If battery temp <= 5C, reduce charge rate by 1/4 */
			if (s->data.battery_temp <= 5.0) s->charge_amps /= 4.0;

			/* Watch for rise in battery temp, anything above 5 deg C is an error */
			/* We could lower charge amps until temp goes down and then set that as max amps */
			if (pct(s->data.battery_temp,s->start_temp) > 5) {
				log_error("current_temp: %.1f, start_temp: %.1f\n", s->data.battery_temp, s->start_temp);
				charge_stop(s,1);
				return;
			}
		}

		/* CC */
		if (s->charge_mode == 1) {
			dprintf(1,"charge_at_max: %d, charge_creep: %d\n", s->charge_at_max, s->charge_creep);
			if ((s->data.battery_voltage+0.0001) >= s->charge_end_voltage) {
				if (s->charge_method == 1) charge_start_cv(s,1);
				else charge_stop(s,1);
			} else if (!s->charge_at_max && s->charge_creep) {
				incvolt(s);
			}

		/* CV */
		} else if (s->charge_mode == 2) {
			if (s->cv_method == CV_METHOD_TIME) {
				time_t current_time,end_time;

				time(&current_time);

				/* End saturation mode after X minutes */
 				end_time = s->cv_start_time + MINUTES(s->cv_time);
				cvremain(current_time,end_time);
				if (current_time >= end_time) charge_stop(s,1);
			} else if (s->cv_method == CV_METHOD_AMPS) {
				float amps,avg;
				int i;

				amps = s->data.battery_current * -1;
				dprintf(0,"battery_amps: %f, charge_amps: %f\n", amps, s->charge_amps);

				/* Amps < 0 (battery drain) will clear the hist */
				if (amps < 0) {
					s->baidx = s->bafull = 0;
					return;
				}

				/* Average the last X amp samples */
				dprintf(0,"amps: %f, baidx: %d\n", amps, s->baidx);
				s->ba[s->baidx++] = amps;
				if (s->baidx == SI_MAX_BA) {
					s->baidx = 0;
					s->bafull = 1;
				}
				if (s->bafull) {
					amps = 0;
					for(i=0; i < SI_MAX_BA; i++) {
						dprintf(0,"ba[%d]: %f\n", i, s->ba[i]);
						amps += s->ba[i];
					}
					avg = amps / SI_MAX_BA;
					dprintf(0,"avg: %.1f, cv_cutoff: %.1f\n", avg, s->cv_cutoff);
					if (avg < s->cv_cutoff) charge_stop(s,1);
				}
			}
		}
	}
}

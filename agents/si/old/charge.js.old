
/*
Copyright (c) 2021, Stephen P. Shoecraft
All rights reserved.

This source code is licensed under the BSD-style license found in the
LICENSE file in the root directory of this source tree.
*/

agent.libdir="../../core";
include(agent.libdir+"/utils.js");

function HOURS(n) { return(n * 3600); }
function MINUTES(n) { return(n * 60); }

function si_check_config() {

	/* min and max must be set */
	dprintf(1,"min_voltage: %.1f, max_voltage: %.1f\n", si.min_voltage, si.max_voltage);
	if (!si_isvrange(si.min_voltage)) {
		si.errmsg = "min_voltage not set or out of range\n";
		return true;
	}
	if (!si_isvrange(si.max_voltage)) {
		si.errmsg = "max_voltage not set or out of range\n";
		return true;
	}
	/* min must be < max */
	if (si.min_voltage >= si.max_voltage) {
		si.errmsg = "min_voltage > max_voltage\n";
		return true;
	}
	/* max must be > min */
	if (si.max_voltage <= si.min_voltage) {
		si.errmsg = "min_voltage > max_voltage\n";
		return true;
	}
	if (!si.readonly) {
		/* charge_start_voltage must be >= min */
		dprintf(1,"charge_start_voltage: %.1f, charge_end_voltage: %.1f\n",
			si.charge_start_voltage, si.charge_end_voltage);
		if (si.charge_start_voltage < si.min_voltage) {
			si.errmsg = "charge_start_voltage < min_voltage";
			return true;
		}
		/* charge_start_voltage must be <= max */
		if (si.charge_start_voltage > si.max_voltage) {
			si.errmsg = "charge_start_voltage > max_voltage";
			return true;
		}
		/* charge_end_voltage must be >= min */
		if (si.charge_end_voltage < si.min_voltage) {
			si.errmsg = "charge_end_voltage < min_voltage";
			return true;
		}
		/* charge_end_voltage must be <= max */
		if (si.charge_end_voltage > si.max_voltage) {
			si.errmsg = "charge_end_voltage > max_voltage";
			return true;
		}
		/* charge_start_voltage must be < charge_end_voltage */
		if (si.charge_start_voltage > si.charge_end_voltage) {
			si.errmsg = "charge_start_voltage > charge_end_voltage";
			return true;
		}
	}
	return false;
}

function charge_init() {
	if (si_check_config(s)) {
		log_error("%s\n", si.errmsg);
		charge_stop(s,0);
		return true;
	}
	si.charge_voltage = si.charge_end_voltage;
	si.charge_amps = si.charge_min_amps;
	si.charge_amps_temp_modifier = 1.0;
	si.charge_amps_soc_modifier = 1.0;
	solard_clear_state(s,SI_STATE_CHARGING);
	si.charge_mode = 0;
	si.force_charge = 0;
	if (si.grid_charge_amps <= 0) si.grid_charge_amps = si.std_charge_amps;
	if (si.gen_charge_amps <= 0) si.gen_charge_amps = si.std_charge_amps;
	return false;
}

function charge_max_start() {
	if (si_check_config(s)) {
		log_write(LOG_ERROR,"%s\n", si.errmsg);
		charge_stop(s,0);
		return;
	}
	log_write(LOG_INFO,"*** CHARGING AT MAX ***\n");
	si.charge_voltage = si.max_voltage;
	si.charge_amps = si.std_charge_amps;
	si.charge_at_max = 1;
}

function charge_max_stop() {
	if (si_check_config(s)) {
		log_write(LOG_ERROR,"%s\n", si.errmsg);
		charge_stop(s,0);
		return;
	}
	log_write(LOG_INFO,"*** CHARGING AT END ***\n");
	si.charge_voltage = si.charge_end_voltage;
	si.charge_amps = si.std_charge_amps;
	si.charge_at_max = 0;
}

function charge_stop(rep) {

	if (!solard_check_state(s,SI_STATE_CHARGING)) return;

	if (rep) log_write(LOG_INFO,"*** ENDING CHARGE ***\n");
	solard_clear_state(s,SI_STATE_CHARGING);
	si.charge_mode = 0;

	/* Need this AFTER ending charge */
	if (si_check_config(s)) {
		log_write(LOG_ERROR,"%s\n",si.errmsg);
		return;
	}

	dprintf(1,"charge_end_voltage: %.1f, charge_min_amps: %.1f\n", si.charge_end_voltage, si.charge_min_amps);
	si.charge_voltage = si.charge_end_voltage;
	si.charge_amps = si.charge_min_amps;
	si.force_charge = 0;
}

function charge_start(rep) {

	if (!si.data.Run) return;

	if (solard_check_state(s,SI_STATE_CHARGING)) return;
	if (si_check_config(s)) {
		log_write(LOG_ERROR,"%s\n",si.errmsg);
		charge_stop(s,0);
		return;
	}

	if (si.have_battery_temp && si.data.battery_temp <= 0.0) {
		log_write(LOG_WARNING,"battery_temp <= 0.0, not starting charge\n");
		return;
	}

	if (si.charge_at_max) charge_max_start(s);
	else si.charge_voltage = si.charge_end_voltage;

	si.charge_amps = si.std_charge_amps;
	dprintf(1,"charge_voltage: %.1f, charge_amps: %.1f\n", si.charge_voltage, si.charge_amps);

	if (rep) log_write(LOG_INFO,"*** STARTING CC CHARGE ***\n");
	solard_set_state(s,SI_STATE_CHARGING);
	si.charge_mode = 1;
	si.charge_amps_soc_modifier = 1.0;
	si.charge_amps_temp_modifier = 1.0;
	si.start_temp = si.data.battery_temp;
}

function cvremain(start, end) {
	var diff,hours,mins;

	diff = difftime(end,start);
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

function charge_start_cv(rep) {

	if (!si.data.Run) return;

//	charge_stop(s,0);
	if (si_check_config(s)) {
		log_write(LOG_ERROR,"%s\n",si.errmsg);
		charge_stop(s,0);
		return;
	}
	if (rep) log_write(LOG_INFO,"*** STARTING CV CHARGE ***\n");
	solard_set_state(s,SI_STATE_CHARGING);
	si.charge_mode = 2;
	si.charge_voltage = si.charge_end_voltage;
	si.charge_amps = si.std_charge_amps;
//	si.cv_start_time = time();
//	si.cv_start_time = Date();
	si.start_temp = si.data.battery_temp;
	si.baidx = si.bafull = 0;
}

function charge_control(mode, rep) {
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

function pct(val1,val2) {
	var val;

	dprintf(1,"val1: %.3f, val: %.3f\n", val1, val2);
	val = 100 - ((val2 / val1)  * 100.0);
	dprintf(1,"val: %.3f\n", val);
	return val;
}

function incvolt() {
	var battery_amps;

	battery_amps = si.data.battery_current * -1;;
	dprintf(0,"battery_amps: %.1f, charge_amps: %.1f, pct: %f\n", battery_amps, si.charge_amps, pct(battery_amps,si.charge_amps));
//	if (battery_amps >= si.charge_amps) return;
	if (pct(battery_amps,si.charge_amps) > -5.0) return;
	dprintf(0,"ac1_frequency: %.1f\n", si.data.ac1_frequency);
	if (si.data.ac1_frequency > 0.0 && (si.data.ac1_frequency < 50.0 || si.data.ac1_frequency < 60.0)) return;
	dprintf(0,"charge_voltage: %.1f, max_voltage: %.1f\n", si.charge_voltage, si.max_voltage);
	if ((si.charge_voltage + 0.1) >= si.max_voltage) return;
	si.charge_voltage += 0.1;
	if (si.charge_voltage > si.max_voltage) {
		log_write(LOG_ERROR,"charge_voltage > max_voltage!!!\n");
		si.charge_voltage = si.max_voltage;
	}
}

//const EC_STATES = {
const EC_STATE_NONE = 0;
const	EC_STATE_GENSTARTi = 1;
const	EC_STATE_GENWAITON = 2;
const	EC_STATE_GENSTOP = 3;
const	EC_STATE_GENWAITOFF = 4;
const	EC_STATE_GENRESTORE = 5;
const	EC_STATE_GRIDSTART = 6;
const	EC_STATE_GRIDWAITON = 7;
const	EC_STATE_GRIDSTOP = 8;
const	EC_STATE_GRIDWAITOFF = 9;
const	EC_STATE_CHARGING = 10;
const	EC_STATE_MAX = 11;

const states = [
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
];

function statestr(state) {
	if (state < EC_STATE_NONE || state >= EC_STATE_MAX)
		return "Unknown";
	else
		return states[state];
}

function charge_check() {
	dprintf(1,"state: %d (%s)\n", si.ec_state, statestr(si.ec_state));
	switch(si.ec_state) {
	case EC_STATE_NONE:
		if ((si.data.battery_voltage-0.0001) <= si.charge_start_voltage) {
			/* Start the charge asap */
			charge_start(s,1);

			/* If we have no AC2 freq */
			dprintf(1,"ac2_frequency: %.1f\n", si.data.ac2_frequency);
			if (si.data.ac2_frequency < 2.0) {
				/* Do we have a gen? */
				dprintf(1,"ExtSrc: %s\n", si.ExtSrc);
				if (!strstr(si.ExtSrc,"Gen")) {
					si_notify(s,"ERROR: voltage (%.1f) below EC threshold (%.1f) and no grid/gen!", 
						si.data.battery_voltage, si.charge_start_voltage);
					si.ec_state = EC_STATE_CHARGING;
					return charge_check();
				} else {
					si.ec_state = EC_STATE_GENSTART;
					return charge_check();
				}
			} else if (!si.data.GdOn) {
				si.ec_state = EC_STATE_GRIDSTART;
				return charge_check();
			}
		}
		break;
	case EC_STATE_GENSTART:
		if (si.smanet) {
			/* Save current val */
			si.gen_save = smanet.get("GnManStr");
			if (typeof(si.gen_save) == "null") {
				log_warning("unable to get current value for GnManStr");
				/* Dont error out, try to start it anyway */
				si.gen_save = "Auto";
			} else {
				if (si.gen_save == "---") si.gen_save = "Auto";
			}
			/* Start the Gen */
			if (!smanet.set("GnManStr","Start")) {
				si.ec_state = EC_STATE_GENWAITON;
				si.gen_op_time = Date();
				return charge_check();
			} else {
				si_notify(s,"ERROR: unable to start Gen!");
				si.ec_state = EC_STATE_CHARGING;
			}
		} else {
			si_notify(s,"WARNING: unable to start Gen for emergency charge!");
			si.ec_state = EC_STATE_CHARGING;
		}
		break;
	case EC_STATE_GENWAITON:
		dprintf(1,"GnRn: %d, ac2_frequency: %.1f\n", si.data.GnRn, si.data.ac2_frequency);
		if (si.data.GnRn && si.data.ac2_frequency > 2.0) {
			/* Gen is running, connect it */
			si.gen_started = 1;
			si.ec_state = EC_STATE_GRIDSTART;
			break;
		} else {
			/* Timeout expired? */
			cur = Date();
			diff = cur - si.gen_op_time;
			dprintf(1,"diff: %d, gen_start_timeout: %d\n", diff, si.gen_start_timeout);
			if (diff > si.gen_start_timeout) {
				si_notify(s,"ERROR: timeout waiting for Gen to start!");
				/* XXX Maybe switch to charging? */
				si.ec_state = EC_STATE_GRIDSTART;
			}
		}
		break;
	case EC_STATE_GENSTOP:
		/* Send stop to Gen */
		if (smanet_set_value(si.smanet,"GnManStr",0,"Stop"))
			log_error("unable to stop Generator!");
		/* Restore previous value on next entry */
		si.ec_state = EC_STATE_GENRESTORE;
		break;
	case EC_STATE_GENRESTORE:
		/* Restore GnManStr value */
		if (smanet_set_value(si.smanet,"GnManStr",0,si.gen_save))
			log_error("unable to restore Gen state to: %s\n",si.gen_save);
		if (si.data.GnRn)
			si.ec_state = EC_STATE_GENWAITOFF;
		else
			si.ec_state = EC_STATE_NONE;
		return charge_check();
		break;
	case EC_STATE_GENWAITOFF:
		dprintf(1,"GnRn: %d\n", si.data.GnRn);
		if (si.data.GnRn) {
			/* Timeout expired? */
			diff = Date() - si.gen_op_time;
			dprintf(1,"diff: %d, gen_stop_timeout: %d\n", diff, si.gen_stop_timeout);
			if (diff > si.gen_stop_timeout) {
				si_notify(s,"ERROR: timeout waiting for Gen to stop!");
				si.ec_state = EC_STATE_NONE;
			}
		}
		break;
	case EC_STATE_GRIDSTART:
		if (si.smanet) {
			si.grid_save = smanet.get("GdManStr");
			if (typedef(si.grid_save) == "undefined") {
				log_warning("unable to get current value for GdManStr");
				/* Dont error out, try to start it anyway */
				strcpy(si.grid_save,"Auto");
			} else {
				if (si.grid_save == "---") si.grid_save = "Auto";
			}
			/* Start the Grid */
			if (smanet.set("GdManStr","Start")) {
				si_notify(s,"ERROR: unable to start Grid!");
				si.ec_state = EC_STATE_CHARGING;
			} else {
				si.ec_state = EC_STATE_GRIDWAITON;
				si.grid_op_time = Date();
				return charge_check();
			}
		} else {
			si_notify(s,"WARNING: unable to start Grid for emergridcy charge!");
			si.ec_state = EC_STATE_CHARGING;
		}
		break;
	case EC_STATE_GRIDSTOP:
		/* Restore GdManStr value */
		if (smanet_set_value(si.smanet,"GdManStr",0,si.grid_save))
			log_error("unable to restore grid state to: %s\n",si.grid_save);
		/* Dont bother waiting */
		if (si.gen_started)
			si.ec_state = EC_STATE_GENSTOP;
		else
			si.ec_state = EC_STATE_NONE;
		break;
	case EC_STATE_CHARGING:
		/* Has the charge completed? */
		if (si.charge_mode == 0) {
			if (si.gen_started)
				si.ec_state = EC_STATE_GENSTOP;
			else if (si.grid_started)
				si.ec_state = EC_STATE_GRIDSTOP;
			else
				si.ec_state = EC_STATE_NONE;
		}	
		break;
	}

	dprintf(1,"battery_voltage: %.1f, charge_start_voltage: %.1f, charge_end_voltage: %.1f\n",
		si.data.battery_voltage, si.charge_start_voltage, si.charge_end_voltage);
	if ((si.data.battery_voltage-0.0001) <= si.charge_start_voltage) {
		/* Start charging */
		si.force_charge = 1;
		charge_start(s,1);
	}

	if (solard_check_state(s,SI_STATE_CHARGING)) {
		if (si.charge_voltage != si.last_charge_voltage || si.data.battery_current != si.last_battery_amps) {
			lprintf(0,"Charge Voltage: %.1f, Battery Amps: %.1f\n",si.charge_voltage,si.data.battery_current);
			si.last_charge_voltage = si.charge_voltage;
			si.last_battery_amps = si.data.battery_current;
		}
		if (si.have_battery_temp) {
			/* If battery temp is <= 0, stop charging immediately */
			dprintf(1,"battery_temp: %2.1f\n", si.data.battery_temp);
			if (si.data.battery_temp <= 0.0) {
				charge_stop(s,1);
				return;
			}
			/* If battery temp <= 5C, reduce charge rate by 1/4 */
			if (si.data.battery_temp <= 5.0) si.charge_amps /= 4.0;

			/* Watch for rise in battery temp, anything above 5 deg C is an error */
			/* We could lower charge amps until temp goes down and then set that as max amps */
			if (pct(si.data.battery_temp,si.start_temp) > 5) {
				log_error("current_temp: %.1f, start_temp: %.1f\n", si.data.battery_temp, si.start_temp);
				charge_stop(s,1);
				return;
			}
		}

		/* CC */
		if (si.charge_mode == 1) {
			dprintf(1,"charge_at_max: %d, charge_creep: %d\n", si.charge_at_max, si.charge_creep);
			if ((si.data.battery_voltage+0.0001) >= si.charge_end_voltage) {
				if (si.charge_method == 1) charge_start_cv(s,1);
				else charge_stop(s,1);
			} else if (!si.charge_at_max && si.charge_creep) {
				incvolt(s);
			}

		/* CV */
		} else if (si.charge_mode == 2) {
			if (si.cv_method == 0) {
				var current_time,end_time;

				current_time = Date();

				/* End saturation mode after X minutes */
 				end_time = si.cv_start_time + MINUTES(si.cv_time);
				cvremain(current_time,end_time);
				if (current_time >= end_time) charge_stop(s,1);
			} else if (si.cv_method == 1) {
				var amps,avg;
				var i;

				amps = si.data.battery_current * -1;
				dprintf(0,"battery_amps: %f, charge_amps: %f\n", amps, si.charge_amps);

				/* Amps < 0 (battery drain) will clear the hist */
				if (amps < 0) {
					si.baidx = si.bafull = 0;
					return;
				}

				/* Average the last X amp samples */
				dprintf(0,"amps: %f, baidx: %d\n", amps, si.baidx);
				si.ba[si.baidx++] = amps;
				if (si.baidx == SI_MAX_BA) {
					si.baidx = 0;
					si.bafull = 1;
				}
				if (si.bafull) {
					amps = 0;
					for(i=0; i < SI_MAX_BA; i++) {
						dprintf(0,"ba[%d]: %f\n", i, si.ba[i]);
						amps += si.ba[i];
					}
					avg = amps / SI_MAX_BA;
					dprintf(0,"avg: %.1f, cv_cutoff: %.1f\n", avg, si.cv_cutoff);
					if (avg < si.cv_cutoff) charge_stop(s,1);
				}
			}
		}
	}
}

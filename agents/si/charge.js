
/*
Copyright (c) 2022, Stephen P. Shoecraft
All rights reserved.

This source code is licensed under the BSD-style license found in the
LICENSE file in the root directory of this source tree.
*/

function si_check_config() {
	var spread;

	dprintf(1,"min_voltage: %.1f, max_voltage: %.1f\n", si.min_voltage, si.max_voltage);
	if (!si_isvrange(si.min_voltage) && !float_equals(si.min_voltage,0.0)) {
		log_warning("min_voltage (%.1f) out of range(%.1f to %.1f)\n",
			si.min_voltage, SI_VOLTAGE_MIN, SI_VOLTAGE_MAX);
		si.min_voltage = 0.0;
	} else if (si.min_voltage >= si.max_voltage) {
		if (si.min_voltage > 0.0) log_warning("min_voltage >= max_voltage\n");
		si.min_voltage = 0.0;
	}
	if (!si_isvrange(si.max_voltage) && !float_equals(si.max_voltage,0.0)) {
		log_warning("max_voltage (%.1f) out of range(%.1f to %.1f)\n",
			si.max_voltage, SI_VOLTAGE_MIN, SI_VOLTAGE_MAX);
		si.max_voltage = 0.0;
	} else if (si.max_voltage <= si.min_voltage) {
		if (si.max_voltage > 0.0) log_warning("max_voltage <= min_voltage\n");
		si.max_voltage = 0.0;
	}
	if (float_equals(si.min_voltage,0.0)) {
		log_warning("setting min_voltage to %.1f\n", 41.0);
		si.min_voltage = 41.0;
	}
	if (float_equals(si.max_voltage,0.0)) {
		log_warning("setting max_voltage to %.1f\n", 58.1);
		si.max_voltage = 58.1;
	}

	while(1) {
		spread = si.max_voltage - si.min_voltage;
		if (!float_equals(si.charge_start_voltage,0.0)) {
			if (!si_isvrange(si.charge_start_voltage)) {
				log_warning("charge_start_voltage (%.1f) out of range(%.1f to %.1f)\n",
					si.charge_start_voltage, SI_VOLTAGE_MIN, SI_VOLTAGE_MAX);
				si.charge_start_voltage = 0.0;
			} else if (si.charge_start_voltage >= si.charge_end_voltage) {
				log_warning("charge_start_voltage >= charge_end_voltage");
				si.charge_start_voltage = 0.0;
			} else if (si.charge_start_voltage < si.min_voltage) {
				log_warning("charge_start_voltage < min_voltage");
				si.charge_start_voltage = 0.0;
			} else if (si.charge_start_voltage > si.max_voltage) {
				log_warning("charge_start_voltage > max_voltage");
				si.charge_start_voltage = 0.0;
			}
		}
		if (float_equals(si.charge_start_voltage,0.0)) {
			si.charge_start_voltage = si.min_voltage + (spread * 0.25);
			log_warning("setting charge_start_voltage to %.1f\n", si.charge_start_voltage);
		}
		if (!float_equals(si.charge_end_voltage,0.0)) {
			if (!si_isvrange(si.charge_end_voltage)) {
				log_warning("charge_end_voltage (%.1f) out of range(%.1f to %.1f)\n",
					si.charge_end_voltage, SI_VOLTAGE_MIN, SI_VOLTAGE_MAX);
				si.charge_end_voltage = 0.0;
			} else if (si.charge_end_voltage <= si.charge_start_voltage) {
				log_warning("charge_end_voltage <= charge_start_voltage");
				si.charge_end_voltage = 0.0;
			} else if (si.charge_end_voltage < si.min_voltage) {
				log_warning("charge_end_voltage < min_voltage");
				si.charge_end_voltage = 0.0;
			} else if (si.charge_end_voltage > si.max_voltage) {
				log_warning("charge_end_voltage > max_voltage");
				si.charge_end_voltage = 0.0;
			}
		}
		if (float_equals(si.charge_end_voltage,0.0)) {
			si.charge_end_voltage = si.min_voltage + (spread * 0.85);
			log_warning("setting charge_end_voltage to %.1f\n", si.charge_end_voltage);
			continue;
		}

		break;
	}

	return 0;
}

/* Init the charging modules */
function charge_init() {

	var data = si.data;

	charge = new Class("charge", null, 0);

//	config.add("si","charge_data",DATA_TYPE_INT,4,0);

	CHARGE_METHOD_FAST = 0;
	CHARGE_METHOD_CCCV = 1;

	CV_METHOD_TIME = 0;
	CV_METHOD_AMPS = 1;
	CV_METHOD_HYBRID = 1;

	/* Make sure all our variables are defined */
	var flags = CONFIG_FLAG_NOPUB | CONFIG_FLAG_NOSAVE;
	var jstypes = [
		[ "charge_voltage", DATA_TYPE_INT, si.data.battery_voltage, flags ],
		[ "charge_amps_temp_modifier", DATA_TYPE_INT, "1", flags ],
		[ "charge_amps_soc_modifier", DATA_TYPE_INT, "1", flags ],
		[ "charge_state", DATA_TYPE_INT, 0, flags ],
		[ "ExtSrc", DATA_TYPE_STRING, "san", flags ],
		[ "charge_method", DATA_TYPE_INT, CHARGE_METHOD_CCCV.toString(), 0 ],
		[ "cv_method", DATA_TYPE_INT, CV_METHOD_AMPS.toString(), 0 ],
		[ "cv_time", DATA_TYPE_INT, "120", 0 ],
		[ "cv_cutoff", DATA_TYPE_INT, "30", 0 ],
		[ "cv_timeout", DATA_TYPE_BOOL, "true", 0 ],
	];
/*
                { "charge_method", DATA_TYPE_INT, "1", 0,
                        "select", 2, (int []){ 0, 1 }, 2, (char *[]){ "CC/CV","oneshot" }, 0, 1, 0 },
                { "cv_method", DATA_TYPE_INT, &s->cv_method, 0, STRINGIFY(CV_METHOD_AMPS), 0,
                        "select", 2, (int []){ 0, 1 }, 2, (char *[]){ "time","amps" }, 0, 1, 0 },
                { "cv_time", DATA_TYPE_INT, &s->cv_time, 0, "120", 0,
                        "range", 3, (int []){ 0, 1440, 1 }, 1, (char *[]){ "CV Time" }, "minutes", 1, 0 },
                { "cv_cutoff", DATA_TYPE_FLOAT, &s->cv_cutoff, 0, "5", 0,
                        0, 0, 0, 1, (char *[]){ "CV Cutoff" }, "V", 1, 0 },
                { "cv_timeout", DATA_TYPE_BOOL, &s->cv_timeout, 0, "true" },
*/


	for(j=0; j < jstypes.length; j++) {
		if (typeof(si[jstypes[j][0]]) == "undefined")
			config.add(agent.name,jstypes[j][0],jstypes[j][1],jstypes[j][2],jstypes[j][3]);
	}

	if (si_check_config()) {
		log_error("%s\n", si.errmsg);
		charge_stop(s,0);
		return;
	}
	si.charge_voltage = si.charge_end_voltage;
	si.charge_amps = si.charge_min_amps;
	si.charge_mode = 0;
	si.grid_save = "";
	si.gen_save = "";
	if (si.grid_charge_amps <= 0) si.grid_charge_amps = si.charge_max_amps;
	if (si.gen_charge_amps <= 0) si.gen_charge_amps = si.charge_max_amps;
	si.ba = [];
	si.baidx = 0;
	si.bafull = 0;

	// # of CV samples to average
	SI_MAX_BA = 6;

	CHARGE_STATE_NONE = 0;
	CHARGE_STATE_CHARGING = 1;
	CHARGE_STATE_GENSTART = 10;
	CHARGE_STATE_GENWAITON = 11;
	CHARGE_STATE_GENSTOP = 12;
	CHARGE_STATE_GENWAITOFF = 13;
	CHARGE_STATE_GENRESTORE = 14;
	CHARGE_STATE_GRIDSTART = 20;
	CHARGE_STATE_GRIDWAITON = 21;
	CHARGE_STATE_GRIDSTOP = 22;
	CHARGE_STATE_GRIDWAITOFF = 23;
	CHARGE_STATE_GRIDRESTORE = 25;
	si.charge_state = CHARGE_STATE_NONE;

	charge_states = [
		[ CHARGE_STATE_NONE, "none" ],
		[ CHARGE_STATE_CHARGING, "Charging" ],
		[ CHARGE_STATE_GENSTART, "GenStart" ],
		[ CHARGE_STATE_GENWAITON, "GenWaitOn" ],
		[ CHARGE_STATE_GENSTOP, "GenStop" ],
		[ CHARGE_STATE_GENWAITOFF, "GenWaitOff" ],
		[ CHARGE_STATE_GENRESTORE, "GenRestore" ],
		[ CHARGE_STATE_GRIDSTART, "GridStart" ],
		[ CHARGE_STATE_GRIDWAITON, "GridWaitOn" ],
		[ CHARGE_STATE_GRIDSTOP, "GridStop" ],
		[ CHARGE_STATE_GRIDWAITOFF, "GridWaitOff" ],
		[ CHARGE_STATE_GRIDRESTORE, "GridRestore" ],
	];

	charging_initialized = true;
}

function statestr(state) {
	dprintf(1,"state: %d\n", state);
	for(i=0; i < charge_states.length; i++) {
		if (state == charge_states[i][0])
			return charge_states[i][1];
	}
	return "unknown";
}

function charge_max_start() {
	if (si_check_config()) {
		log_error("%s\n", si.errmsg);
		charge_stop(s,0);
		return;
	}
	log_info("*** CHARGING AT MAX ***\n");
	si.charge_voltage = si.max_voltage;
	si.charge_amps = si.charge_max_amps;
	si.charge_at_max = 1;
}

function charge_max_stop() {
	if (si_check_config()) {
		log_error("%s\n", si.errmsg);
		charge_stop(s,0);
		return;
	}
	log_info("*** CHARGING AT END ***\n");
	si.charge_voltage = si.charge_end_voltage;
	si.charge_amps = si.charge_max_amps;
	si.charge_at_max = 0;
}

function charge_stop(rep) {

	if (!si.charge_mode) return;

	if (rep) log_info("*** ENDING CHARGE ***\n");
	si.charge_mode = 0;

	/* Need this AFTER ending charge */
	if (si_check_config()) {
		log_error("%s\n",si.errmsg);
		return;
	}

	dprintf(1,"charge_end_voltage: %.1f, charge_min_amps: %.1f\n", si.charge_end_voltage, si.charge_min_amps);
	si.charge_voltage = si.charge_end_voltage;
	si.charge_amps = si.charge_min_amps;
//	si.force_charge = 0;
}

function charge_start(rep) {

	if (!si.data.Run) return;

	if (si.charge_mode) return;
	if (si_check_config()) {
		log_error("%s\n",si.errmsg);
		charge_stop(s,0);
		return;
	}

	if (si.have_battery_temp && si.data.battery_temp <= 0.0) {
		log_warning("battery_temp <= 0.0, not starting charge\n");
		return;
	}

	if (si.charge_at_max) charge_max_start(s);
	else si.charge_voltage = si.charge_end_voltage;

	si.charge_amps = si.charge_max_amps;
	dprintf(1,"charge_voltage: %.1f, charge_amps: %.1f\n", si.charge_voltage, si.charge_amps);

	if (rep) log_info("*** STARTING CC CHARGE ***\n");
	si.charge_mode = 1;
	si.charge_amps_soc_modifier = 1.0;
	si.charge_amps_temp_modifier = 1.0;
//	si.start_temp = si.data.battery_temp;
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
	log_info("CV Time remaining: %02d:%02d:%02d\n",hours,mins,diff);
}

function charge_start_cv(rep) {

	if (!si.data.Run) return;

//	charge_stop(0);
	if (si_check_config()) {
		log_error("%s\n",si.errmsg);
		charge_stop(0);
		return;
	}
	if (rep) log_info("*** STARTING CV CHARGE ***\n");
	si.charge_mode = 2;
	si.charge_voltage = si.charge_end_voltage;
	si.charge_amps = si.charge_max_amps;
	si.cv_start_time = time();
	si.start_temp = si.data.battery_temp;
	si.baidx = si.bafull = 0;
}

function charge_control(mode,rep) {
	switch(mode) {
	default:
	case 0:
		charge_stop(rep);
		break;
	case 1:
		charge_start(rep);
		break;
	case 2:
		charge_start_cv(rep);
		break;
	}
	return 0;
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
		log_error("charge_voltage > max_voltage!!!\n");
		si.charge_voltage = si.max_voltage;
	}
}

function charge_grid_start() {
	printf("*** STARTING GRID ***\n");
	if (sim.enable) {
		si.grid_save = "Auto";
		si.charge_state = CHARGE_STATE_GRIDWAITON;
		si.grid_op_time = time();
		return false;
	} else if (si.smanet_connected) {
		si.grid_save = smanet.get("GdManStr");
		if (typeof(si.grid_save) == "undefined") {
			log_warning("unable to get current value for GdManStr");
			/* Dont error out, try to start it anyway */
			si.grid_save = "Auto";
		} else if (si.grid_save == "---") {
			si.grid_save = "Auto";
		}
		dprintf(0,"grid_save: %s\n", si.grid_save);
		/* Start the Grid */
		if (!smanet.set("GdManStrx","Start")) {
			si.charge_state = CHARGE_STATE_GRIDWAITON;
			si.grid_op_time = time();
			return true;
		} else {
			si.notify("ERROR: unable to start Grid!");
			si.charge_state = CHARGE_STATE_CHARGING;
		}
	} else {
		si.notify("ERROR: unable to start Grid: smanet not connected");
		si.charge_state = CHARGE_STATE_CHARGING;
	}
	return false;
}

function charge_grid_stop() {
	printf("*** STOPPING GRID ***\n");
	if (!sim.enable && si.smanet_connected) {
		/* Restore GdManStr value */
		if (smanet_set_value(si.smanet,"GdManStr",0,si.grid_save))
			log_error("unable to restore grid state to: %s\n",si.grid_save);
	}
	/* XXX Dont bother waiting */
	si.charge_state = CHARGE_STATE_NONE;
	return false;
}

function charge_gen_start() {
	printf("*** STARTING GEN ***\n");
	if (sim.enable) {
		si.gen_save = "Auto";
		si.charge_state = CHARGE_STATE_GENWAITON;
		si.gen_op_time = time();
	} else if (si.smanet_connected) {
		/* Save current val */
		si.gen_save = smanet.get("GnManStr");
		if (typeof(si.gen_save) == "null") {
			log_warning("unable to get current value for GnManStr");
			/* Dont error out, try to start it anyway */
			si.gen_save = "Auto";
		} else {
			if (si.gen_save == "---") si.gen_save = "Auto";
		}
		dprintf(0,"gen_save: %s\n", si.gen_save);
		/* Start the Gen */
		if (!smanet.set("GnManStrx","Start")) {
			si.charge_state = CHARGE_STATE_GENWAITON;
			si.gen_op_time = time();
			return true;
		} else {
			si.notify("ERROR: unable to start Gen!");
			si.charge_state = CHARGE_STATE_CHARGING;
		}
	} else {
		si.notify("ERROR: unable to start Gen: smanet not connected");
		si.charge_state = CHARGE_STATE_CHARGING;
	}
	return false;
}

function charge_gen_stop() {
	printf("*** STOPPING GEN ***\n");
	if (!sim.enable && si.smanet_connected) {
		/* Restore GnManStr value */
		if (smanet_set_value(si.smanet,"GnManStr",0,si.gen_save))
			log_error("unable to restore gen state to: %s\n",si.gen_save);
	}
	/* XXX Dont bother waiting */
	si.charge_state = CHARGE_STATE_NONE;
	return false;
}

function charge_main()  {

	var checking = true;
	while(checking) {
		dprintf(0,"state: %s\n", statestr(si.charge_state));
		checking = false;
		switch(si.charge_state) {
		case CHARGE_STATE_NONE:
			dprintf(1,"si.data.battery_voltage: %.1f, si.charge_start_voltage: %.1f\n",
				si.data.battery_voltage, si.charge_start_voltage);
			if ((si.data.battery_voltage-0.0001) <= si.charge_start_voltage) {
				/* Start the charge asap */
				charge_start(1);

				/* If we have no AC2 freq */
				dprintf(0,"ac2_frequency: %.1f\n", si.data.ac2_frequency);
				if (si.data.ac2_frequency < 10.0) {
					/* Do we have a gen? */
					if (si.smanet_connected) {
						dprintf(1,"ExtSrc: %s\n", si.ExtSrc);
						if (!strstr(si.ExtSrc,"Gen")) {
							si.notify("ERROR: voltage (%.1f) below EC threshold (%.1f) and no grid/gen!", 
								si.data.battery_voltage, si.charge_start_voltage);
							si.charge_state = CHARGE_STATE_CHARGING;
						}
					} else {
						si.charge_state = CHARGE_STATE_GENSTART;
						checking = true;
					}
				} else if (!si.data.GdOn) {
					si.charge_state = CHARGE_STATE_GRIDSTART;
					checking = true;
				}
			// Failsafe
			} else if (si.data.battery_voltage >= (si.max_voltage-0.0001)) {
				si.charge_state = CHARGE_STATE_CHARGING;
				checking = true;
			}
			break;
		case CHARGE_STATE_GENSTART:
			checking = charge_gen_start();
			break;
		case CHARGE_STATE_GENWAITON:
			printf("*** GENWAIT: GnOn: %d\n", si.data.GnOn);
			dprintf(1,"GnOn: %d\n", si.data.GnOn);
			if (si.data.GnOn) {
				/* Gen is running, connect it */
				si.gen_started = 1;
				si.charge_state = CHARGE_STATE_CHARGING;
				checking = true;
			} else {
				/* Timeout expired? */
				cur = time();
				diff = cur - si.gen_op_time;
				dprintf(0,"diff: %d, gen_start_timeout: %d\n", diff, si.gen_start_timeout);
				if (diff > si.gen_start_timeout) {
					si.notify("ERROR: timeout waiting for Gen to start!");
					si.charge_state = CHARGE_STATE_CHARGING;
					checking = true;
				}
			}
			break;
		case CHARGE_STATE_GENSTOP:
			checking = charge_gen_stop();
			break;
		case CHARGE_STATE_GENRESTORE:
			if (si.smanet_connected) {
				/* Restore GnManStr value */
				printf("*** RESTORING GEN\n");
				if (smanet_set_value(si.smanet,"GnManStr",0,si.gen_save))
					log_error("unable to restore Gen state to: %s\n",si.gen_save);
				if (si.data.GnOn)
					si.charge_state = CHARGE_STATE_GENWAITOFF;
				else
					si.charge_state = CHARGE_STATE_NONE;
			} else {
				si.charge_state = CHARGE_STATE_NONE;
			}
			break;
		case CHARGE_STATE_GENWAITOFF:
			if (sim.enable) si.data.GnOn = false;
			dprintf(1,"GnOn: %d\n", si.data.GnOn);
			if (si.data.GnOn) {
				/* Timeout expired? */
				diff = time() - si.gen_op_time;
				dprintf(1,"diff: %d, gen_stop_timeout: %d\n", diff, si.gen_stop_timeout);
				if (diff > si.gen_stop_timeout) {
					si.notify("ERROR: timeout waiting for Gen to stop!");
					si.charge_state = CHARGE_STATE_GENRESTORE;
				}
			}
			checking = false;
			break;
		case CHARGE_STATE_GRIDSTART:
			checking = charge_grid_start();
			break;
		case CHARGE_STATE_GRIDWAITON:
			if (sim.enable) si.data.GdOn = true;
			dprintf(0,"GdOn: %d\n", si.data.GdOn);
			if (si.data.GdOn) {
				/* Grid is running, connect it */
				si.grid_started = 1;
				si.charge_state = CHARGE_STATE_CHARGING;
				checking = true;
			} else {
				/* Timeout expired? */
				cur = time();
				diff = cur - si.gen_op_time;
				dprintf(1,"diff: %d, gen_start_timeout: %d\n", diff, si.gen_start_timeout);
				if (diff > si.gen_start_timeout) {
					si.notify("ERROR: timeout waiting for Grid to start!");
					si.charge_state = CHARGE_STATE_CHARGING;
					checking = true;
				}
			}
			break;
		case CHARGE_STATE_GRIDSTOP:
			checking = charge_grid_stop();
			break;
		case CHARGE_STATE_CHARGING:
			/* Has the charge completed? */
			if (si.charge_mode == 0) {
				if (si.gen_started)
					si.charge_state = CHARGE_STATE_GENSTOP;
				else if (si.grid_started)
					si.charge_state = CHARGE_STATE_GRIDSTOP;
				else
					si.charge_state = CHARGE_STATE_NONE;
			}	
			break;
		}
	}

	// In case of data errors
	if (si.data.battery_voltage < si.min_voltage) return;

	// If not charging leave now 
	if (!si.charge_mode) return 0;

if (0 == 1) {
	dprintf(1,"battery_voltage: %.1f, charge_start_voltage: %.1f, charge_end_voltage: %.1f\n",
		si.data.battery_voltage, si.charge_start_voltage, si.charge_end_voltage);
	if ((si.data.battery_voltage-0.0001) <= si.charge_start_voltage) {
		/* Start charging */
		charge_start(1);
	}
}

	if (si.charge_mode != 0) {
//		if (si.charge_voltage != si.last_charge_voltage || si.data.battery_current != si.last_battery_amps) {
//			dprintf(1,"Charge Voltage: %.1f, Battery Amps: %.1f\n",si.charge_voltage,si.data.battery_current);
//			si.last_charge_voltage = si.charge_voltage;
//			si.last_battery_amps = si.data.battery_current;
//		}
		if (si.have_battery_temp) {
			/* If battery temp is <= 0, stop charging immediately */
			dprintf(1,"battery_temp: %2.1f\n", si.data.battery_temp);
			if (si.data.battery_temp <= 0.0) {
				charge_stop(1);
				return;
			}
			/* If battery temp <= 5C, reduce charge rate by 1/4 */
			if (si.data.battery_temp <= 5.0) si.charge_amps /= 4.0;

			/* Watch for rise in battery temp, anything above 5 deg C is an error */
			/* We could lower charge amps until temp goes down and then set that as max amps */
			if (pct(si.data.battery_temp,si.start_temp) > 5) {
				log_error("current_temp: %.1f, start_temp: %.1f\n", si.data.battery_temp, si.start_temp);
				charge_stop(1);
				return;
			}
		}

		/* CC */
		if (si.charge_mode == CHARGE_MODE_CC) {
			dprintf(1,"charge_at_max: %d, charge_creep: %d\n", si.charge_at_max, si.charge_creep);
			if ((si.data.battery_voltage+0.0001) >= si.charge_end_voltage) {
				if (si.charge_method == CHARGE_METHOD_CCCV) charge_start_cv(1);
				else charge_stop(1);
			} else if (!si.charge_at_max && si.charge_creep) {
				incvolt();
			}

		/* CV */
		} else if (si.charge_mode == CHARGE_MODE_CV) {
			if (si.cv_method == CV_METHOD_TIME) {
				var current_time,end_time;

				current_time = time();

				/* End saturation mode after X minutes */
 				end_time = si.cv_start_time + MINUTES(si.cv_time);
				cvremain(current_time,end_time);
				if (current_time >= end_time) charge_stop(1);
			} else if (si.cv_method == CV_METHOD_AMPS) {
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
					if (avg < si.cv_cutoff) charge_stop(1);
				}
			}
		}
	}
}

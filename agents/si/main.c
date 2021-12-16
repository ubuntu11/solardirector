/*
Copyright (c) 2021, Stephen P. Shoecraft
All rights reserved.

This source code is licensed under the BSD-style license found in the
LICENSE file in the root directory of this source tree.
*/

#include "si.h"
#include "transports.h"

#define TESTING 1

#define DEBUG_STARTUP 1

#if DEBUG_STARTUP
#define _ST_DEBUG LOG_DEBUG
#else
#define _ST_DEBUG 0
#endif

char *si_agent_version_string = "1.0";

static int si_agent_init(int argc, char **argv, opt_proctab_t *si_opts, si_session_t *s) {
	config_property_t si_props[] = {
		{ "debug", DATA_TYPE_INT, &debug, 0, 0, 0,
			"range", 3, (int []){ 0, 99, 1 }, 1, (char *[]) { "debug level" }, 0, 1, 0 },
		{ "readonly", DATA_TYPE_BOOL, &s->readonly, 0, "1", 0, 0, 0, 0, 0, 0, 0, 1, 0 },
		{ "startup_charge_mode", DATA_TYPE_INT, &s->startup_charge_mode, 0, "0", 0,
			"select", 3, (int []){ 0, 1, 2 }, 1, (char *[]){ "Off","On","CV" }, 0, 1, 0 },
		{ "charge_mode", DATA_TYPE_INT, &s->charge_mode, 0, "0", 0,
			"select", 3, (int []){ 0, 1, 2 }, 3, (char *[]){ "Off","On","CV" }, 0, 1, 0 },
		{ "charge_method", DATA_TYPE_INT, &s->charge_method, 0, "1", 0,
			"select", 2, (int []){ 0, 1 }, 2, (char *[]){ "CC/CV","oneshot" }, 0, 1, 0 },
		{ "cv_method", DATA_TYPE_INT, &s->cv_method, 0, "0", 0,
			"select", 2, (int []){ 0, 1 }, 2, (char *[]){ "time","amps" }, 0, 1, 0 },
		{ "cv_time", DATA_TYPE_INT, &s->cv_time, 0, "90", 0,
			"range", 3, (int []){ 0, 1440, 1 }, 1, (char *[]){ "CV Time" }, "minutes", 1, 0 },
		{ "cv_cutoff", DATA_TYPE_FLOAT, &s->cv_cutoff, 0, "5", 0,
			0, 0, 0, 1, (char *[]){ "CV Cutoff" }, "V", 1, 0 },
		{ "min_voltage", DATA_TYPE_FLOAT, &s->min_voltage, 0, 0, 0,
			"range", 3, (float []){ SI_VOLTAGE_MIN, SI_VOLTAGE_MAX, .1 },
			1, (char *[]){ "Min Battery Voltage" }, "V", 1, 0 },
		{ "max_voltage", DATA_TYPE_FLOAT, &s->max_voltage, 0, 0, 0,
			"range", 3, (float []){ SI_VOLTAGE_MIN, SI_VOLTAGE_MAX, .1 },
			1, (char *[]){ "Max Battery Voltage" }, "V", 1, 0 },
		{ "charge_start_voltage", DATA_TYPE_FLOAT, &s->charge_start_voltage, 0, 0, 0,
//			CONFIG_FLAG_READONLY | CONFIG_FLAG_NOSAVE,
			"range", 3, (float []){ SI_VOLTAGE_MIN, SI_VOLTAGE_MAX, .1 },
			1, (char *[]){ "Start charging at this voltage" }, "V", 1, 0 },
		{ "charge_end_voltage", DATA_TYPE_FLOAT, &s->charge_end_voltage, 0, 0, 0,
//			CONFIG_FLAG_READONLY | CONFIG_FLAG_NOSAVE,
			"range", 3, (float []){ SI_VOLTAGE_MIN, SI_VOLTAGE_MAX, .1 },
			1, (char *[]){ "Stop charging at this voltage" }, "V", 1, 0 },
#if 0
		{ "charge_start_soc", DATA_TYPE_FLOAT, &s->charge_start_voltage, 0, 0, 0,
			"range", 3, (float []){ SI_VOLTAGE_MIN, SI_VOLTAGE_MAX, .1 },
			1, (char *[]){ "Start charging at this voltage" }, "V", 1, 0 },
		{ "charge_end_soc", DATA_TYPE_FLOAT, &s->charge_end_voltage, 0, 0, 0,
			"range", 3, (float []){ SI_VOLTAGE_MIN, SI_VOLTAGE_MAX, .1 },
			1, (char *[]){ "Stop charging at this voltage" }, "V", 1, 0 },
#endif
		{ "charge_min_amps", DATA_TYPE_FLOAT, &s->charge_min_amps, 0, 0, 0,
			"range", 3, (float []){ 0, 5000, .1 }, 1, (char *[]){ "Minimum charge amps" }, "A", 1, 0 },
		{ "charge_amps", DATA_TYPE_FLOAT, &s->std_charge_amps, 0, 0, 0,
			"range", 3, (float []){ 0, 5000, .1 }, 1, (char *[]){ "Charge Amps" }, "A", 1, 0 },

		{ "have_grid", DATA_TYPE_BOOL, &s->have_grid, 0, "-1", 0,
			"range", 3, (int []){ -1, 0, 1 }, 
			3, (char *[]){ "Not set", "No", "Yes" }, 0, 1, 0 },
		{ "charge_from_grid", DATA_TYPE_BOOL, &s->charge_from_grid, 0, "-1", 0,
			"range", 3, (int []){ -1, 0, 1 }, 
			3, (char *[]){ "Not set", "No", "Yes" }, 0, 1, 0 },
		{ "grid_soc_enabled", DATA_TYPE_BOOL, &s->grid_soc_enabled, 0, "-1", 0,
			"range", 3, (int []){ -1, 0, 1 }, 
			3, (char *[]){ "Not set", "No", "Yes" }, 0, 1, 0 },
		{ "grid_soc_stop", DATA_TYPE_FLOAT, &s->grid_soc_stop, 0, "-1", 0,
			"range", 3, (float []){ 0.0, 100.0, .1 },
			1, (char *[]){ "Stop charging at this SoC" }, "%", 1, 1 },
		{ "grid_charge_amps", DATA_TYPE_FLOAT, &s->grid_charge_amps, 0, 0, 0,
			"range", 3, (float []){ 0, 5000, .1 }, 1, (char *[]){ "Charge amps when grid is connected" }, "A", 1, 0 },

		{ "have_gen", DATA_TYPE_BOOL, &s->have_gen, 0, "-1", 0,
			"range", 3, (int []){ -1, 0, 1 }, 
			3, (char *[]){ "Not set", "No", "Yes" }, 0, 1, 0 },
		{ "gen_soc_stop", DATA_TYPE_FLOAT, &s->gen_soc_stop, 0, "-1", 0,
			"range", 3, (float []){ 0.0, 100.0, .1 },
			1, (char *[]){ "Stop charging at this SoC" }, "%", 1, 1 },
		{ "gen_charge_amps", DATA_TYPE_FLOAT, &s->gen_charge_amps, 0, 0, 0,
			"range", 3, (float []){ 0, 5000, .1 }, 1, (char *[]){ "Charge amps when gen is connected" }, "A", 1, 0 },

		{ "charge_at_max", DATA_TYPE_BOOL, &s->charge_at_max, 0, 0, 0,
			0, 0, 0, 1, (char *[]){ "Charge at max_voltage" }, 0, 0 },
		{ "charge_creep", DATA_TYPE_BOOL, &s->charge_creep, 0, 0, 0,
			0, 0, 0, 1, (char *[]){ "Increase voltage to maintain charge amps" }, 0, 0 },
		{ "can_transport", DATA_TYPE_STRING, s->can_transport, sizeof(s->can_transport)-1, 0, 0,
			0, 0, 0, 0, 0, 0, 1, 0 },
		{ "can_target", DATA_TYPE_STRING, s->can_target, sizeof(s->can_target)-1, 0, 0,
			0, 0, 0, 0, 0, 0, 1, 0 },
		{ "can_topts", DATA_TYPE_STRING, s->can_topts, sizeof(s->can_topts)-1, 0, 0,
			0, 0, 0, 0, 0, 0, 1, 0 },
		{ "can_fallback", DATA_TYPE_BOOL, &s->can_fallback, 0, "N", 0,
			0, 0, 0, 1, (char *[]){ "Fallback to NULL driver if CAN init fails" }, 0, 0 },
		{ "smanet_transport", DATA_TYPE_STRING, &s->smanet_transport, sizeof(s->smanet_transport)-1, 0, 0,
			0, 0, 0, 0, 0, 0, 1, 0 },
		{ "smanet_target", DATA_TYPE_STRING, &s->smanet_target, sizeof(s->smanet_target)-1, 0, 0,
			0, 0, 0, 0, 0, 0, 1, 0 },
		{ "smanet_topts", DATA_TYPE_STRING, &s->smanet_topts, sizeof(s->smanet_topts)-1, 0, 0,
			0, 0, 0, 0, 0, 0, 1, 0 },
		{ "smanet_channels_path", DATA_TYPE_STRING, &s->smanet_channels_path, sizeof(s->smanet_channels_path)-1, 0, 0,
			0, 0, 0, 0, 0, 0, 1, 0 },
		{ "sim", DATA_TYPE_BOOL, &s->sim, 0, "N", 0,
			0, 0, 0, 1, (char *[]){ "Run simulation" }, 0, 0 },
		{ "sim_step", DATA_TYPE_FLOAT, &s->sim_step, 0, "0.1", 0,
			"range", 3, (float []){ 0, 5000, .1 }, 1, (char *[]){ "SIM Voltage Step" }, 0, 1, 1 },
		{ "discharge_amps", DATA_TYPE_FLOAT, &s->discharge_amps, 0, "1200", 0,
			"range", 3, (float []){ 0, 5000, .1 }, 1, (char *[]){ "Discharge Amps" }, "A", 1, 0 },
		{ "interval", DATA_TYPE_INT, &s->interval, 0, "10", 0,
			"range", 3, (int []){ 0, 10000, 1 }, 1, (char *[]) { "Interval" }, "S", 1, 0 },
		{ "capacity", DATA_TYPE_FLOAT, &s->soc, 0, 0, CONFIG_FLAG_READONLY | CONFIG_FLAG_NOSAVE,
			"range", 3, (float []){ 0, 100, .1 }, 1, (char *[]){ "State of Charge" }, 0, 1, 1 },
		{ "user_soc", DATA_TYPE_FLOAT, &s->user_soc, 0, "-1", 0,
			"range", 3, (float []){ 0, 100, .1 }, 1, (char *[]){ "State of Charge" }, 0, 1, 1 },
#if 0
		{ "grid_start_timeout", DATA_TYPE_INT, &s->grid_start_timeout, 0, "90", 0,
			"range", 3, (int []){ 0, 1440, 1 }, 1, (char *[]){ "Time to wait for grid to start" }, "S", 1, 0 },
		{ "grid_stop_timeout", DATA_TYPE_INT, &s->grid_stop_timeout, 0, "90", 0,
			"range", 3, (int []){ 0, 1440, 1 }, 1, (char *[]){ "Time to wait for grid to stop" }, "S", 1, 0 },
		{ "gen_start_timeout", DATA_TYPE_INT, &s->gen_start_timeout, 0, "90", 0,
			"range", 3, (int []){ 0, 1440, 1 }, 1, (char *[]){ "Time to wait for gen to start" }, "S", 1, 0 },
		{ "gen_stop_timeout", DATA_TYPE_INT, &s->gen_stop_timeout, 0, "90", 0,
			"range", 3, (int []){ 0, 1440, 1 }, 1, (char *[]){ "Time to wait for gen to stop" }, "S", 1, 0 },
#endif
		{ "notify", DATA_TYPE_STRING, &s->notify_path, sizeof(s->notify_path)-1, 0, 0,
			0, 0, 0, 1, (char *[]){ "Notify program" }, 0, 1, 0 },
		{ "have_battery_temp", DATA_TYPE_BOOL, &s->have_battery_temp, 0, "N", 0,
			0, 0, 0, 1, (char *[]){ "Is battery temp available" }, 0, 0 },
#include "si_info_props.c"
		{ 0 }
	};
	config_function_t si_funcs[] = {
		{0}
	};

	s->ap = agent_init(argc,argv,si_opts,&si_driver,s,si_props,si_funcs);
	dprintf(1,"ap: %p\n",s->ap);
	if (!s->ap) return 1;
	return 0;
}

#ifdef SI_CB
static int si_cb(void *ctx) {
	si_session_t *s = ctx;

	/* Get the relay/func bits */
	dprintf(1,"getting bits...\n");
	if (si_get_bits(s)) return 1;

	if (s->info.GdOn != s->grid_connected) log_info("Grid %s\n",(s->grid_connected ? "connected" : "disconnected"));
	if (s->info.GnOn != s->gen_connected) log_info("Generator %s\n",(s->gen_connected ? "connected" : "disconnected"));
	if ((s->info.GdOn != s->grid_connected) || (s->info.GnOn != s->gen_connected)) si_write_va(s);
	s->grid_connected = s->info.GdOn;
	s->gen_connected = s->info.GnOn;

	return 0;
}
#endif

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
	dprintf(1,"s->can_transport: %s, s->can_target: %s, s->can_topts: %s\n", s->can_transport, s->can_target, s->can_topts);

	if (si_can_init(s)) {
		log_error(s->errmsg);
		return 1;
	}

	dprintf(1,"smatpinfo: %s\n", smatpinfo);
	if (strlen(smatpinfo)) {
		*s->smanet_transport = *s->smanet_target = *s->smanet_topts = 0;
		strncat(s->smanet_transport,strele(0,",",smatpinfo),sizeof(s->smanet_transport)-1);
		strncat(s->smanet_target,strele(1,",",smatpinfo),sizeof(s->smanet_target)-1);
		strncat(s->smanet_topts,strele(2,",",smatpinfo),sizeof(s->smanet_topts)-1);
        }
	si_smanet_init(s);

	/* Init charge params */
	charge_init(s);

	/* Interval cannot be more than 30s */
	if (s->interval > 30) {
		log_warning("interval reduced to 30s (was: %d)\n",s->interval);
		s->interval = 30;
	}

	/* Read and write intervals are the same */
	s->ap->read_interval = s->ap->write_interval = s->interval;

	/* We are starting up */
	s->startup = 1;

#ifdef SI_CB
	/* Set callback */
	agent_set_callback(s->ap,si_cb,s);
#endif

	/* Init JS */
	si_jsinit(s);
	if (s->smanet) smanet_jsinit(s->ap->js, s->smanet);

	time(&end);
	diff = end - start;
	dprintf(1,"--> startup time: %d\n", diff);

	/* Go */
	agent_run(s->ap);
	return 0;
}


/*
Copyright (c) 2021, Stephen P. Shoecraft
All rights reserved.

This source code is licensed under the BSD-style license found in the
LICENSE file in the root directory of this source tree.
*/

#include "si.h"
#include "transports.h"

extern solard_driver_t si_driver;

#define dlevel 1

static int si_get_value(void *ctx, list args, char *errmsg) {
//	si_session_t *s = ctx;
	dprintf(dlevel,"args count: %d\n", list_count(args));
	return 0;
}

static int si_set_value(void *ctx, list args, char *errmsg) {
	si_session_t *s = ctx;
	char **argv, *name;
	config_property_t *p;

	dprintf(1,"args: %p\n", args);
	dprintf(1,"args count: %d\n", list_count(args));
	list_reset(args);
	while((argv = list_get_next(args)) != 0) {
		dprintf(1,"argv: %p\n", argv);
		dprintf(1,"argv[0]: %p\n", argv[0]);
		dprintf(1,"argv[0]: %s\n", argv[0]);
		p = config_find_property(s->ap->cp, argv[0]);
		dprintf(dlevel,"p: %p\n", p);
		if (!p) {
			sprintf(errmsg,"property %s not found",argv[0]);
			return 1;
		}
		p->len = conv_type(p->type,p->dest,p->dsize,DATA_TYPE_STRING,argv[1],strlen(argv[1]));
	}
	return 0;
}

static int si_cf_init_can(void *ctx, list args, char *errmsg) {
	si_session_t *s = ctx;

	if (si_can_init(s)) {
		strcpy(errmsg,s->errmsg);
		return 1;
	}
	return 0;
}

static int si_cf_init_smanet(void *ctx, list args, char *errmsg) {
	si_session_t *s = ctx;

	if (si_smanet_init(s)) {
		strcpy(errmsg,s->errmsg);
		return 1;
	}
	return 0;
}

int si_agent_init(int argc, char **argv, opt_proctab_t *si_opts, si_session_t *s) {
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
		{ "grid_connected", DATA_TYPE_BOOL, &s->grid_connected, 0, 0, CONFIG_FLAG_NOSAVE },
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
		{ "gen_connected", DATA_TYPE_BOOL, &s->gen_connected, 0, 0, CONFIG_FLAG_NOSAVE },
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
		{ "smanet_data", DATA_TYPE_BOOL, &s->smanet_data, 0, "N", 0,
			0, 0, 0, 1, (char *[]){ "supplement data with SMANET" }, 0, 0 },
		{ "sim", DATA_TYPE_BOOL, &s->sim, 0, "N", 0,
			0, 0, 0, 1, (char *[]){ "Run simulation" }, 0, 0 },
		{ "sim_step", DATA_TYPE_FLOAT, &s->sim_step, 0, "0.1", 0,
			"range", 3, (float []){ 0, 5000, .1 }, 1, (char *[]){ "SIM Voltage Step" }, 0, 1, 1 },
		{ "discharge_amps", DATA_TYPE_FLOAT, &s->discharge_amps, 0, "1200", 0,
			"range", 3, (float []){ 0, 5000, .1 }, 1, (char *[]){ "Discharge Amps" }, "A", 1, 0 },
		{ "interval", DATA_TYPE_INT, &s->interval, 0, "30", 0,
			"range", 3, (int []){ 0, 10000, 1 }, 1, (char *[]) { "Interval" }, "S", 1, 0 },
		{ "soc", DATA_TYPE_FLOAT, &s->soc, 0, 0, CONFIG_FLAG_NOSAVE,
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
		{ "input_current_source", DATA_TYPE_STRING, &s->input_current_source, sizeof(s->input_current_source)-1, 0, 0,
			0, 0, 0, 1, (char *[]){ "Source of input current data" }, 0, 1, 0 },
		{ "output_current_source", DATA_TYPE_STRING, &s->output_current_source, sizeof(s->output_current_source)-1, 0, 0,
			0, 0, 0, 1, (char *[]){ "Source of output current data" }, 0, 1, 0 },
		{ 0 }
	};
	config_function_t si_funcs[] = {
		{ "get", si_get_value, s, 1 },
		{ "set", si_set_value, s, 2 },
		{ "init_can", si_cf_init_can, s, 3 },
		{ "init_smanet", si_cf_init_smanet, s, 3 },
		{0}
	};

	s->ap = agent_init(argc,argv,si_opts,&si_driver,s,si_props,si_funcs);
	dprintf(1,"ap: %p\n",s->ap);
	if (!s->ap) return 1;
	return 0;
}

#if 0
enum SI_PARAM_SOURCE {
	SI_PARAM_SOURCE_AGENT,
	SI_PARAM_SOURCE_SMANET,
};

/* Combined SI/SMANET params */
struct si_param {
	char name[32];
	enum SI_PARAM_SOURCE source;
	enum DATA_TYPE type;
	void *ptr;
	int len;
	union {
		unsigned char bval;
		short wval;
		int ival;
		long lval;
		float fval;
		double dval;
//		char sval[128];
		char *sval;
	};
};
typedef struct si_param si_param_t;

struct solard_config_params {
        char *name;			/* Parameter name */
        enum DATA_TYPE type;		/* Data type */
	void *dest;			/* Ptr to storage */
	int len;			/* Length of storage */
        char *scope;			/* Scope (select/range/etc) */
        int nvalues;			/* # of scope values */
        void *values;			/* Scope values */
        int nlabels;			/* # of labels */
        char **labels;			/* Labels */
        char *units;			/* Units */
        float scale;			/* Scale */
        char *format;			/* Format for display */
	char *def;			/* Default value (STRING) */
};
typedef struct solard_config_params solard_config_params_t;

int si_config_init(si_session_t *s) {
#if 0
	solard_config_params_t si_params[] = {
		{ "charge_mode", DATA_TYPE_INT, &s->charge_mode, 0,
			"select", 3, (int []){ 0, 1, 2 }, 1, (char *[]){ "Off","On","CV" }, 0, 1, 0, 0 },
		{ "charge_method", DATA_TYPE_INT, &s->charge_method, 0,
			"select", 3, (int []){ 0, 1, 2 }, 1, (char *[]){ "none","CC/CV","oneshot" }, 0, 1, 0, 0 },
		{ "startup_charge_mode", DATA_TYPE_INT, &s->startup_charge_mode, 0,
			"select", 3, (int []){ 0, 1, 2 }, 1, (char *[]){ "Off","On","CV" }, 0, 1, 0, 0 },
		{ "cv_method", DATA_TYPE_INT, &s->cv_method, 0,
			"select", 2, (int []){ 0, 1 }, 1, (char *[]){ "time","amps" }, 0, 1, 0, "0" },
		{ "cv_time", DATA_TYPE_INT, &s->cv_time, 0,
			"range", 3, (int []){ 0, 1440, 1 },
			1, (char *[]){ "CV Time" }, "M", 1, "%d", "90" },
#if 0
		{ s->ap->section_name, "cv_time", 0, DATA_TYPE_INT, &s->cv_time, 0, "90" },
		{ s->ap->section_name, "cv_cutoff", 0, DATA_TYPE_INT, &s->cv_cutoff, 0, "7" },
		{ s->ap->section_name, "gen_start_timeout", 0, DATA_TYPE_INT, &s->gen_start_timeout, 0, "30" },
		{ s->ap->section_name, "notify", 0, DATA_TYPE_STRING, &s->notify_path, sizeof(s->notify_path)-1, "" },
		{ s->ap->section_name, "report_interval", 0, DATA_TYPE_INT, &s->cv_cutoff, 0, "15" },
#endif
		{ "max_voltage", DATA_TYPE_FLOAT, &s->max_voltage, 0,
			"range", 3, (float []){ SI_VOLTAGE_MIN, SI_VOLTAGE_MAX, .1 },
			1, (char *[]){ "Max Battery Voltage" }, "V", 1, "%2.1f", 0 },
		{ "min_voltage", DATA_TYPE_FLOAT, &s->min_voltage, 0,
			"range", 3, (float []){ SI_VOLTAGE_MIN, SI_VOLTAGE_MAX, .1 },
			1, (char *[]){ "Min Battery Voltage" }, "V", 1, "%2.1f", 0 },
		{ "charge_start_voltage", DATA_TYPE_FLOAT, &s->charge_start_voltage, 0,
			"range", 3, (float []){ SI_VOLTAGE_MIN, SI_VOLTAGE_MAX, .1 },
			1, (char *[]){ "Charge start voltage" }, "V", 1, "%2.1f", 0 },
		{ "charge_end_voltage", DATA_TYPE_FLOAT, &s->charge_end_voltage, 0,
			"range", 3, (float []){ SI_VOLTAGE_MIN, SI_VOLTAGE_MAX, .1 },
			1, (char *[]){ "Charge end voltage" }, "V", 1, "%2.1f", 0 },
		{ "charge_amps", DATA_TYPE_FLOAT, &s->charge_amps, 0, 
			"range", 3, (float []){ 0, 5000, .1 }, 1, (char *[]){ "Charge Amps" }, "A", 1, "%.1f" },
		{ "discharge_amps", DATA_TYPE_FLOAT, &s->discharge_amps, 0,
			"range", 3, (float []){ 0, 5000, .1 }, 1, (char *[]){ "Discharge Amps" }, "A", 1, "%.1f" },
		{ "charge_min_amps", DATA_TYPE_FLOAT, &s->charge_min_amps, 0
			"range", 3, (float []){ 0, 5000, .1 }, 1, (char *[]){ "Minimum charge amps" }, "A", 1, "%.1f" },
		{ "charge_at_max", DATA_TYPE_BOOL, &s->charge_at_max, 0,
			0, 0, 0, 1, (char *[]){ "Charge at max_voltage" }, 0, 0 },
		{ "charge_creep", DATA_TYPE_BOOL, &s->charge_creep, 0,
			 0, 0, 0, 1, (char *[]){ "Increase voltage to maintain charge amps" }, 0, 0 },
		{ "sim_step", DATA_TYPE_FLOAT, &s->sim_step, 0,
			"range", 3, (float []){ 0, 5000, .1 }, 1, (char *[]){ "SIM Voltage Step" }, "V", 1, "%.1f", "0.1" },
		{ "readonly", DATA_TYPE_BOOL, &s->readonly, 0,
			0, 0, 0, 1, (char *[]){ "Do not update" }, 0, 0, "N" },
		{ "interval", DATA_TYPE_INT, &s->interval, 0,
			"range", 3, (int []){ 0, 10000, 1 }, 0, 0, 0, 1, 0, "10" },
		{ "can_transport", DATA_TYPE_STRING, s->can_transport, sizeof(s->can_transport)-1,
			0, 0, 0, 0, 0, 0, 1, 0 },
		{ "can_target", DATA_TYPE_STRING, s->can_target, sizeof(s->can_target)-1,
			0, 0, 0, 0, 0, 0, 1, 0 },
		{ "can_topts", DATA_TYPE_STRING, s->can_topts, sizeof(s->can_topts)-1,
			0, 0, 0, 0, 0, 1, 0, 0 },
		{ "smanet_transport", DATA_TYPE_STRING, &s->smanet_transport, sizeof(s->smanet_transport)-1,
			0, 0, 0, 0, 0, 0, 1, 0 },
		{ "smanet_target", DATA_TYPE_STRING, &s->smanet_target, sizeof(s->smanet_target)-1,
			0, 0, 0, 0, 0, 0, 1, 0 },
		{ "smanet_topts", DATA_TYPE_STRING, &s->smanet_topts, sizeof(s->smanet_topts)-1,
			0, 0, 0, 0, 0, 0, 1, 0 },
		{ "smanet_channels_path", DATA_TYPE_STRING, &s->smanet_channels_path, sizeof(s->smanet_channels_path)-1,
			0, 0, 0, 0, 0, 0, 1, 0 },
		{ "soc", DATA_TYPE_FLOAT, &s->user_soc, 0, 
			"range", 3, (float []){ 0, 100, .1 }, 1, (char *[]){ "State of charge" }, "%", 1, "%.1f", "-1" },
		{ "grid_charge_amps", DATA_TYPE_FLOAT, &s->grid_charge_amps, 0, 
			"range", 3, (float []){ 0, 1000, .1 }, 1, (char *[]){ "State of charge" }, "%", 1, "%.1f", "25" },
		{ "gen_charge_amps", DATA_TYPE_FLOAT, &s->gen_charge_amps, 0, 
			"range", 3, (float []){ 0, 1000, .1 }, 1, (char *[]){ "State of charge" }, "%", 1, "%.1f", "1000" },
		{ "charge_min_amps", DATA_TYPE_FLOAT, &s->charge_min_amps, 0, 
			"range", 3, (float []){ 0, 100, .1 }, 1, (char *[]){ "State of charge" }, "%", 1, "%.1f", "-1" },
		{ s->ap->section_name, "charge_creep", "Increase charge voltage in order to maintain charge amps", DATA_TYPE_BOOL, &s->charge_creep, 0, "no" },
#if 0
		{ section_name, "name", 0, DATA_TYPE_STRING,&s->name,sizeof(s->name)-1, s->ap->instance_name },
		{ section_name, "system_voltage", 0, DATA_TYPE_FLOAT, &s->system_voltage, 0, "48" },
		{ section_name, "max_voltage", 0, DATA_TYPE_FLOAT, &s->max_voltage, 0, 0 },
		{ section_name, "min_voltage", 0, DATA_TYPE_FLOAT, &s->min_voltage, 0, 0 },
		{ section_name, "charge_start_voltage", 0, DATA_TYPE_FLOAT, &s->charge_start_voltage, 0, 0 },
		{ section_name, "charge_end_voltage", 0, DATA_TYPE_FLOAT, &s->charge_end_voltage, 0, 0 },
		{ section_name, "charge_at_max", 0, DATA_TYPE_BOOL, &s->charge_at_max, 0, 0 },
		{ section_name, "charge_amps", 0, DATA_TYPE_FLOAT, &s->charge_amps, 0, 0 },
		{ section_name, "discharge_amps", 0, DATA_TYPE_FLOAT, &s->discharge_amps, 0, 0 },
		{ section_name, "soc", 0, DATA_TYPE_FLOAT, &s->soc, 0, "-1" },
#endif
		CFG_PROCTAB_END
	};
#endif

	return 0;
}

//#define NCHG (sizeof(charge_params)/sizeof(json_descriptor_t))


int si_read_config(si_session_t *s) {
//	getconf(s);
	return 0;
}

int si_write_config(si_session_t *s) {
	return 0;
}

json_descriptor_t *_getd(si_session_t *s,char *name) {
	json_descriptor_t *dp;
	register int i,j;

	dprintf(1,"label: %s\n", name);

	for(i=0; i < NALL; i++) {
		dprintf(1,"section: %s\n", allparms[i].name);
		dp = allparms[i].parms;
		for(j=0; j < allparms[i].count; j++) {
			dprintf(1,"dp->name: %s\n", dp[j].name);
			if (strcmp(dp[j].name,name)==0)
				return &dp[j];
		}
	}

	/* See if it's in our desc list */
	dprintf(1,"looking in desc list...\n");
	list_reset(s->desc);
	while((dp = list_get_next(s->desc)) != 0) {
		dprintf(1,"dp->name: %s\n", dp->name);
		if (strcmp(dp->name,name)==0)
			return dp;
	}

	return 0;
}
#endif

static void _addchans(si_session_t *s) {
	smanet_session_t *ss = s->smanet;
	smanet_channel_t *c;
	config_property_t newp;
	float step;

	list_reset(ss->channels);
	while((c = list_get_next(ss->channels)) != 0) {
		dprintf(1,"adding chan: %s\n", c->name);
		memset(&newp,0,sizeof(newp));
		newp.name = c->name;
		step = 1;
		switch(c->format & 0xf) {
		case CH_BYTE:
			newp.type = DATA_TYPE_BYTE;
			break;
		case CH_SHORT:
			newp.type = DATA_TYPE_SHORT;
			break;
		case CH_LONG:
			newp.type = DATA_TYPE_LONG;
			break;
		case CH_FLOAT:
			newp.type = DATA_TYPE_FLOAT;
			step = .1;
			break;
		case CH_DOUBLE:
			newp.type = DATA_TYPE_DOUBLE;
			step = .1;
			break;
		default:
			dprintf(1,"unknown type: %d\n", c->format & 0xf);
			break;
		}
		if (c->mask & CH_PARA && c->mask & CH_ANALOG) {
			float *values;

			newp.scope = "range";
			values = malloc(3*sizeof(float));
			values[0] = c->gain;
			values[1] = c->offset;
			values[2] = step;
			newp.nvalues = 3;
			newp.values = values;
			dprintf(1,"adding range: 0: %f, 1: %f, 2: %f\n", values[0], values[1], values[2]);
		} else if (c->mask & CH_DIGITAL) {
			char **labels;

			newp.nlabels = 2;
			labels = malloc(newp.nlabels*sizeof(char *));
			labels[0] = c->txtlo;
			labels[1] = c->txthi;
			newp.labels = labels;
		} else if (c->mask & CH_STATUS && list_count(c->strings)) {
			char **labels,*p;
			int i;

			newp.scope = "select";
			newp.nlabels = list_count(c->strings);
			labels = malloc(newp.nlabels*sizeof(char *));
			i = 0;
			list_reset(c->strings);
			while((p = list_get_next(c->strings)) != 0) labels[i++] = p;
			newp.labels = labels;
		}
		newp.units = c->unit;
		newp.scale = 1.0;
		config_add_props(s->ap->cp, "smanet", &newp, CONFIG_FLAG_VOLATILE);
	}
}

int si_config_add_info(si_session_t *s, json_object_t *o) {

	/* Add SMANET channels to config */
	dprintf(1,"smanet: %p\n", s->smanet);
	if (s->smanet) {
		dprintf(1,"smanet_channels_path: %s\n", s->smanet_channels_path);
		if (!strlen(s->smanet_channels_path)) sprintf(s->smanet_channels_path,"%s/%s.dat",SOLARD_LIBDIR,s->smanet->type);
		fixpath(s->smanet_channels_path, sizeof(s->smanet_channels_path)-1);
		dprintf(1,"smanet_channels_path: %s\n", s->smanet_channels_path);
		if (smanet_load_channels(s->smanet, s->smanet_channels_path) == 0) _addchans(s);
	}

	/* Add config info */
	config_add_info(s->ap->cp, o);

	return 0;
}

#if 0
static json_proctab_t *_getinv(si_session_t *s, char *name) {
	int can_init;
	json_proctab_t params[] = {
		{ "charge",DATA_TYPE_INT,&s->charge_mode,0,0 },
		{ "max_voltage",DATA_TYPE_FLOAT,&s->max_voltage,0,0 },
		{ "min_voltage",DATA_TYPE_FLOAT,&s->min_voltage,0,0 },
		{ "charge_start_voltage",DATA_TYPE_FLOAT,&s->charge_start_voltage,0,0 },
		{ "charge_end_voltage",DATA_TYPE_FLOAT,&s->charge_end_voltage,0,0 },
		{ "charge_amps",DATA_TYPE_FLOAT,&s->charge_amps,0,0 },
		{ "discharge_amps",DATA_TYPE_FLOAT,&s->discharge_amps,0,0 },
		{ "charge_min_amps", DATA_TYPE_FLOAT, &s->charge_min_amps,0,0 },
		{ "charge_at_max", DATA_TYPE_BOOL, &s->charge_at_max, 0,0 },
		{ "charge_creep", DATA_TYPE_BOOL, &s->charge_creep, 0,0 },
		{ "sim_step", DATA_TYPE_FLOAT, &s->sim_step, 0,0 },
		{ "interval", DATA_TYPE_INT, &s->interval, 0,0 },
		{ "can_transport", DATA_TYPE_STRING, &s->can_transport, sizeof(s->can_transport)-1,0 },
		{ "can_target", DATA_TYPE_STRING, &s->can_target, sizeof(s->can_target)-1,0 },
		{ "can_topts", DATA_TYPE_STRING, &s->can_topts, sizeof(s->can_topts)-1,0 },
		{ "smanet_transport", DATA_TYPE_STRING, &s->smanet_transport, sizeof(s->smanet_transport)-1,0 },
		{ "smanet_target", DATA_TYPE_STRING, &s->smanet_target, sizeof(s->smanet_target)-1,0 },
		{ "smanet_topts", DATA_TYPE_STRING, &s->smanet_topts, sizeof(s->smanet_topts)-1,0 },
		{ "smanet_channels_path", DATA_TYPE_STRING, &s->smanet_channels_path, sizeof(s->smanet_channels_path)-1,0 },
		{ "soc", DATA_TYPE_FLOAT, &s->user_soc,0,0 },
		{ "readonly", DATA_TYPE_BOOL, &s->readonly,0,0 },
		{ "can_init", DATA_TYPE_BOOL, &can_init,0,0 },
		JSON_PROCTAB_END
	};
	json_proctab_t *invp;

	dprintf(1,"name: %s\n", name);

	for(invp=params; invp->field; invp++) {
		dprintf(1,"invp->field: %s\n", invp->field);
		if (strcmp(invp->field,name)==0) {
			dprintf(1,"found!\n");
			s->idata = *invp;
			return &s->idata;
		}
	}
	dprintf(1,"NOT found\n");
	return 0;
}

si_param_t *_getp(si_session_t *s, char *name, int invonly) {
	json_proctab_t *invp;
	si_param_t *pinfo;

	dprintf(1,"name: %s, invonly: %d\n", name, invonly);

	if (!s->pdata) {
		s->pdata = malloc(sizeof(*pinfo));
		if (!s->pdata) return 0;
	}
	pinfo = s->pdata;
	memset(pinfo,0,sizeof(*pinfo));

	dprintf(0,"mem usage: %d\n", mem_used()-s->ap->mem_start);
	/* Is it from the inverter struct? */
	invp = _getinv(s,name);
	if (invp) {
		uint8_t *bptr;
		short *wptr;
		int *iptr;
		long *lptr;
		float *fptr;
		double *dptr;

		dprintf(1,"inv: name: %s, type: %d, ptr: %p, len: %d\n", invp->field, invp->type, invp->ptr, invp->len);
		dprintf(1,"invp->type: %d\n", invp->type);

		strncat(pinfo->name,invp->field,sizeof(pinfo->name)-1);
		pinfo->type = invp->type;
		pinfo->source = 1;
		switch(invp->type) {
		case DATA_TYPE_BOOL:
			iptr = invp->ptr;
			pinfo->bval = *iptr;
			break;
		case DATA_TYPE_BYTE:
			bptr = invp->ptr;
			pinfo->bval = *bptr;
			break;
		case DATA_TYPE_SHORT:
			wptr = invp->ptr;
			pinfo->wval = *wptr;
			break;
		case DATA_TYPE_INT:
			iptr = invp->ptr;
			pinfo->lval = *iptr;
			break;
		case DATA_TYPE_LONG:
			lptr = invp->ptr;
			pinfo->lval = *lptr;
			break;
		case DATA_TYPE_FLOAT:
			fptr = invp->ptr;
			pinfo->fval = *fptr;
			break;
		case DATA_TYPE_DOUBLE:
			dptr = invp->ptr;
			pinfo->dval = *dptr;
			break;
		case DATA_TYPE_STRING:
			pinfo->sval = invp->ptr;
			break;
		default: break;
		}
		dprintf(1,"found\n");
		dprintf(0,"mem usage: %d\n", mem_used()-s->ap->mem_start);
		return pinfo;
	}
	if (invonly) goto _getp_done;

	if (s->smanet) {
		json_descriptor_t *dp;
		double value;
		char *text;

		dprintf(1,"NOT found, checking smanet\n");

		dp = _getd(s,name);
		if (!dp) {
			dprintf(1,"_getd returned null!\n");
			goto _getp_done;
		}
		if (smanet_get_value(s->smanet, name, &value, &text)) goto _getp_done;
		dprintf(1,"val: %f, sval: %s\n", value, text);
		strncat(pinfo->name,name,sizeof(pinfo->name)-1);
		pinfo->source = 2;
		if (text) {
			pinfo->sval = text;
			pinfo->type = DATA_TYPE_STRING;
		} else {
			conv_type(dp->type,&pinfo->bval,0,DATA_TYPE_DOUBLE,&value,0);
			pinfo->type = dp->type;
		}
		dprintf(0,"mem usage: %d\n", mem_used()-s->ap->mem_start);
		return pinfo;
	}

_getp_done:
	dprintf(1,"NOT found\n");
	return 0;
}

static int si_get_config(si_session_t *s, si_param_t *pp, json_descriptor_t *dp) {
//	char topic[200];
	char temp[72];

	dprintf(1,"dp: name: %s, unit: %s, scale: %f\n", dp->name, dp->units, dp->scale);

	temp[0] = 0;
	dprintf(1,"pp->type: %d (%s)\n", pp->type, typestr(pp->type));
	switch(pp->type) {
	case DATA_TYPE_BOOL:
		dprintf(1,"bval: %d\n", pp->bval);
		sprintf(temp,"%s",pp->bval ? "true" : "false");
		break;
	case DATA_TYPE_BYTE:
		{
			unsigned char bval = pp->bval;
			dprintf(1,"bval: %d\n", bval);
			dprintf(1,"scale: %f\n", dp->scale);
			if (dp->scale != 0.0) bval /= dp->scale;
			dprintf(1,"format: %s\n", dp->format);
			if (dp->format) sprintf(temp,dp->format,bval);
			else sprintf(temp,"%d",bval);
		}
		break;
	case DATA_TYPE_SHORT:
		{
			short wval = pp->wval;
			dprintf(1,"wval: %d\n", wval);
			dprintf(1,"scale: %f\n", dp->scale);
			if (dp->scale != 0.0) wval /= dp->scale;
			dprintf(1,"format: %s\n", dp->format);
			if (dp->format) sprintf(temp,dp->format,wval);
			else sprintf(temp,"%d",wval);
		}
		break;
	case DATA_TYPE_INT:
		{
			int ival = pp->ival;
			dprintf(1,"ival: %d\n", ival);
			dprintf(1,"scale: %f\n", dp->scale);
			if (dp->scale != 0.0) ival /= dp->scale;
			dprintf(1,"format: %s\n", dp->format);
			if (dp->format) sprintf(temp,dp->format,ival);
			else sprintf(temp,"%d",ival);
		}
		break;
	case DATA_TYPE_LONG:
		{
			long lval = pp->lval;
			dprintf(1,"lval: %d\n", lval);
			dprintf(1,"scale: %f\n", dp->scale);
			if (dp->scale != 0.0) lval /= dp->scale;
			dprintf(1,"format: %s\n", dp->format);
			if (dp->format) sprintf(temp,dp->format,lval);
			else sprintf(temp,"%ld",lval);
		}
		break;
	case DATA_TYPE_FLOAT:
		{
			float fval = pp->fval;
			dprintf(1,"fval: %f\n", fval);
			dprintf(1,"scale: %f\n", dp->scale);
			if (dp->scale != 0.0) fval /= dp->scale;
			if (dp->format) sprintf(temp,dp->format,fval);
			else sprintf(temp,"%f",fval);
		}
		break;
	case DATA_TYPE_DOUBLE:
		{
			double dval = pp->dval;
			dprintf(1,"dval: %lf\n", dval);
			dprintf(1,"scale: %f\n", dp->scale);
			if (dp->scale != 0.0) dval /= dp->scale;
			if (dp->format) sprintf(temp,dp->format,dval);
			else sprintf(temp,"%f",dval);
		}
		break;
	case DATA_TYPE_STRING:
		dprintf(1,"string: %s\n", pp->sval);
		temp[0] = 0;
		strncat(temp,pp->sval,sizeof(temp)-1);
		trim(temp);
		break;
	default:
		sprintf(temp,"unhandled switch for: %d",dp->type);
		return 1;
		break;
	}
//	agent_mktopic(topic,sizeof(topic)-1,ap,s->name,SOLARD_FUNC_CONFIG);
//	sprintf(topic,"%s/%s/%s/%s/%s",SOLARD_TOPIC_ROOT,s->ap->name,SOLARD_FUNC_CONFIG,pp->name);
//	dprintf(1,"topic: %s, temp: %s\n", topic, temp);
//	return mqtt_pub(s->ap->m,topic,temp,1,1);
	return 0;
}

#if 0
int si_config_smanet(void *h) {
	si_session_t *s = h;
	char transport[SOLARD_TRANSPORT_LEN];
	char target[SOLARD_TARGET_LEN];
	char topts[SOLARD_TOPTS_LEN];
	cfg_proctab_t smanet_tab[] = {
		{ s->ap->section_name, "smanet_transport", 0, DATA_TYPE_STRING, &transport, sizeof(transport)-1,"" },
		{ s->ap->section_name, "smanet_target", 0, DATA_TYPE_STRING, &target, sizeof(target)-1,"" },
		{ s->ap->section_name, "smanet_topts", 0, DATA_TYPE_STRING, &topts, sizeof(topts)-1,"" },
		{ s->ap->section_name, "smanet_channels_path", 0, DATA_TYPE_STRING, &s->channels_path, sizeof(s->channels_path)-1,"" },
		CFG_PROCTAB_END
	};

	cfg_get_tab(s->ap->cfg,smanet_tab);
	if (debug) cfg_disp_tab(smanet_tab,"smanet",0);
	if (strlen(transport) && strlen(target)) {
		solard_driver_t *tp;
		void *tp_handle;
		int i;

		tp = 0;
		for(i=0; si_transports[i]; i++) {
			tp = si_transports[i];
			dprintf(1,"type: %d\n", tp->type);
			if (tp->type != SOLARD_DRIVER_TRANSPORT) continue;
			dprintf(1,"tp->name: %s\n", tp->name);
			if (strcmp(tp->name,transport) == 0) {
				tp_handle = tp->new(s->ap, target, topts);
				if (tp_handle) {
					s->smanet = smanet_init(tp,tp_handle);
				} else {
					log_write(LOG_ERROR,"error creating new %s transport instance", transport);
					return 1;
				}
			}
		}
		if (!tp) {
			log_write(LOG_ERROR,"unable to load smanet transport: %s",transport);
			return 1;
		}
	} else {
		log_write(LOG_INFO,"smanet transport or target null, skipping smanet init");
		return 1;
	}
	if (!s->smanet) return 1;

	/* Load channels */
	if (!strlen(s->channels_path)) sprintf(s->channels_path,"%s/%s.dat",SOLARD_LIBDIR,s->smanet->type);
	smanet_set_chanpath(s->smanet,s->channels_path);
	if (access(s->channels_path,0) == 0) smanet_load_channels(s->smanet);

	return 0;
}
#endif

static int _si_set_config(si_session_t *s, si_param_t *pp, json_descriptor_t *dp, char *value, char *errmsg) {

	dprintf(1,"dp: name: %s, unit: %s, scale: %f\n", dp->name, dp->units, dp->scale);
	dprintf(1,"used: %ld\n", mem_used() - s->ap->mem_start);

	/* We can use any field in the union as a source or dest */
	if (pp->source == SI_PARAM_SOURCE_INV) {
		json_proctab_t *invp;
		int oldval;

		invp = _getinv(s,pp->name);
		dprintf(1,"field: %s\n", invp->field);
		if (strcmp(invp->field,"can_init")==0) {
			return si_can_init(s);
		}

		/* Save old vals */
//		if (strcmp(invp->field,"run")==0) oldval = s->run_state;
		if (strcmp(invp->field,"charge")==0) oldval = s->charge_mode;
		else if (strcmp(invp->field,"charge_at_max")==0) oldval = s->charge_at_max;
		else oldval = -1;

		/* Update the inv struct directly */
		conv_type(invp->type,invp->ptr,invp->len,DATA_TYPE_STRING,value,strlen(value)+1);
		log_info("%s set to %s\n", invp->field, (char *)invp->ptr);
		dprintf(1,"used: %ld\n", mem_used() - s->ap->mem_start);

		/* Update our local copy of charge amps too */
		if (strcmp(invp->field,"charge_amps")==0) s->charge_amps = *((float *)invp->ptr);

#if 0
		if (strcmp(invp->field,"run")==0) {
			int r;

			printf("oldval: %d, state: %d\n", oldval, s->run_state);
			if (!oldval && s->run_state) {
				r = si_startstop(s,1);
				if (!r) log_write(LOG_INFO,"Started Sunny Island\n");
				else {
					log_error("start failed\n");
					s->run_state = 0;
				}
			}
			if (oldval && !s->run_state) {
				r = si_startstop(s,0);
				if (!r) {
					log_write(LOG_INFO,"Stopped Sunny Island\n");
					charge_stop(s,1);
				} else {
					log_error("stop failed\n");
					s->run_state = 0;
				}
			}
		} else
#endif
		if (strcmp(invp->field,"charge")==0) {
		dprintf(1,"used: %ld\n", mem_used() - s->ap->mem_start);
			charge_control(s,s->charge_mode,1);
		dprintf(1,"used: %ld\n", mem_used() - s->ap->mem_start);
#if 0
			switch(s->charge_mode) {
			case 0:
				charge_stop(s,1);
				break;
			case 1:
				charge_start(s,1);
				break;
			case 2:
				charge_start_cv(s,1);
				break;
			}
#endif
		} else if (strcmp(invp->field,"charge_at_max")==0) {
			printf("at_max: %d, oldval: %d\n", s->charge_at_max, oldval);
			if (s->charge_at_max && !oldval) charge_max_start(s);
			if (!s->charge_at_max && oldval) charge_max_stop(s);
		}


		/* ALSO update this value in our config file */
		dprintf(1,"used: %ld\n", mem_used() - s->ap->mem_start);
//		conv_type(DATA_TYPE_STRING,temp,sizeof(temp)-1,DATA_TYPE_STRING,value,0);
		cfg_set_item(s->ap->cfg,s->ap->section_name,invp->field,0,value);
		dprintf(1,"used: %ld\n", mem_used() - s->ap->mem_start);
		cfg_write(s->ap->cfg);
		dprintf(1,"used: %ld\n", mem_used() - s->ap->mem_start);
	} else { 
		double dval;
		char *text;

		text = 0;
		dprintf(1,"pp->type: %s\n", typestr(pp->type));
		if (pp->type == DATA_TYPE_STRING) text = value;
		else conv_type(DATA_TYPE_DOUBLE,&dval,0,DATA_TYPE_STRING,value,strlen(value));
		if (smanet_set_value(s->smanet,dp->name,dval,text)) {
			strcpy(errmsg,"error setting smanet value");
			return 1;
		}
	}
	/* Re-get the param to update internal vars and publish */
//	return si_get_config(s,_getp(s,dp->name,0),dp);
//	return (pub ? si_get_config(s,_getp(s,dp->name,0),dp) : 0);
	return 0;
}

int si_doconfig(void *ctx, char *action, char *label, char *value, char *errmsg) {
	si_session_t *s = ctx;
	si_param_t *pp;
	json_descriptor_t *dp;
	int r;

	dprintf(1,"used: %ld\n", mem_used() - s->ap->mem_start);
	r = 1;
	dprintf(1,"label: %s, value: %s\n", label, value);
	if (!label) {
		strcpy(errmsg,"invalid request");
		goto si_doconfig_error;
	}
	pp = _getp(s,label,0);
	if (!pp) {
		sprintf(errmsg,"%s: not found",label);
		goto si_doconfig_error;
	}
	dp = _getd(s,label);
	if (!dp) {
		sprintf(errmsg,"%s: not found",label);
		goto si_doconfig_error;
	}

	dprintf(1,"used: %ld\n", mem_used() - s->ap->mem_start);
	dprintf(1,"dp: name: %s, unit: %s, scale: %f\n", dp->name, dp->units, dp->scale);
	dprintf(1,"action: %s\n", action);
	if (strcmp(action,"Get")==0) {
		r = si_get_config(s,pp,dp);
	} else if (strcmp(action,"Set")==0) {
		if (!value) {
			strcpy(errmsg,"invalid request (Set requested but no value passed)");
			goto si_doconfig_error;
		}
		r = _si_set_config(s,pp,dp,value,errmsg);
	} else {
		strcpy(errmsg,"invalid request");
		goto si_doconfig_error;
	}

si_doconfig_error:
	dprintf(1,"used: %ld\n", mem_used() - s->ap->mem_start);
	dprintf(1,"returning: %d\n",r);
	return r;
}

static void getconf(si_session_t *s) {
	cfg_proctab_t mytab[] = {
		{ s->ap->section_name, "readonly", 0, DATA_TYPE_LOGICAL, &s->readonly, 0, "N" },
		{ s->ap->section_name, "soc", 0, DATA_TYPE_FLOAT, &s->user_soc, 0, "-1" },
		{ s->ap->section_name, "startup_charge_mode", 0, DATA_TYPE_INT, &s->startup_charge_mode, 0, "1" },
		{ s->ap->section_name, "grid_charge_amps", 0, DATA_TYPE_FLOAT, &s->grid_charge_amps, 0, "2" },
		{ s->ap->section_name, "charge_min_amps", 0, DATA_TYPE_FLOAT, &s->charge_min_amps, 0, "0.1" },
		{ s->ap->section_name, "charge_creep", "Increase charge voltage in order to maintain charge amps", DATA_TYPE_BOOL, &s->charge_creep, 0, "no" },
		{ s->ap->section_name, "sim_step", 0, DATA_TYPE_FLOAT, &s->sim_step, 0, "0.1" },
		{ s->ap->section_name, "interval", 0, DATA_TYPE_INT, &s->interval, 0, "10" },
		{ s->ap->section_name, "charge_method", 0, DATA_TYPE_INT, &s->charge_method, 0, "1" },
		{ s->ap->section_name, "cv_method", 0, DATA_TYPE_INT, &s->cv_method, 0, "1" },
		{ s->ap->section_name, "cv_time", 0, DATA_TYPE_INT, &s->cv_time, 0, "90" },
		{ s->ap->section_name, "cv_cutoff", 0, DATA_TYPE_INT, &s->cv_cutoff, 0, "7" },
		{ s->ap->section_name, "gen_start_timeout", 0, DATA_TYPE_INT, &s->gen_start_timeout, 0, "30" },
		{ s->ap->section_name, "notify", 0, DATA_TYPE_STRING, &s->notify_path, sizeof(s->notify_path)-1, "" },
		{ s->ap->section_name, "can_transport", 0, DATA_TYPE_STRING, &s->can_transport, sizeof(s->can_transport)-1, "" },
		{ s->ap->section_name, "can_target", 0, DATA_TYPE_STRING, &s->can_target, sizeof(s->can_target)-1, "" },
		{ s->ap->section_name, "can_topts", 0, DATA_TYPE_STRING, &s->can_topts, sizeof(s->can_topts)-1, "" },
		{ s->ap->section_name, "smanet_transport", 0, DATA_TYPE_STRING, &s->smanet_transport, sizeof(s->smanet_transport)-1, "" },
		{ s->ap->section_name, "smanet_target", 0, DATA_TYPE_STRING, &s->smanet_target, sizeof(s->smanet_target)-1, "" },
		{ s->ap->section_name, "smanet_topts", 0, DATA_TYPE_STRING, &s->smanet_topts, sizeof(s->smanet_topts)-1, "" },
		{ s->ap->section_name, "smanet_channels_path", 0, DATA_TYPE_STRING, &s->smanet_channels_path, sizeof(s->smanet_channels_path)-1, "" },
		CFG_PROCTAB_END
	};

	dprintf(1,"s: %p\n", s);
	dprintf(1,"s->ap: %p\n", s->ap);
	dprintf(1,"s->ap->cfg: %p\n", s->ap->cfg);
	cfg_get_tab(s->ap->cfg,mytab);
	if (debug) cfg_disp_tab(mytab,"si",1);
	if (!strlen(s->notify_path)) sprintf(s->notify_path,"%s/notify",SOLARD_BINDIR);

	dprintf(1,"done\n");
	return;

}
#endif

#if 0
static int si_config_getmsg(si_session_t *s, solard_message_t *msg) {
	int status;

	dprintf(1,"used: %ld\n", mem_used() - s->ap->mem_start);
	status = agent_config_process(msg,si_doconfig,s,s->errmsg);
	if (status) goto si_config_error;

	/* If config updated, write/publish it */
//	if (s->ap->cfg->updated) si_config_write(s);

	status = 0;
	strcpy(s->errmsg,"success");

si_config_error:
	dprintf(1,"mem usage: %d\n", mem_used()-s->ap->mem_start);
	dprintf(1,"msg->replyto: %s", msg->replyto);
	if (msg->replyto) agent_reply(s->ap, msg->replyto, status, s->errmsg);
	dprintf(1,"used: %ld\n", mem_used() - s->ap->mem_start);
	return status;
}
#endif

void si_config_add_si_data(si_session_t *s) {
	/* Only used by JS funcs */
	config_property_t si_data_props[] = {
		{ "ac1_voltage_l1", DATA_TYPE_FLOAT, &s->data.ac1_voltage_l1, 0, 0, CONFIG_FLAG_NOSAVE },
		{ "ac1_voltage_l2", DATA_TYPE_FLOAT, &s->data.ac1_voltage_l2, 0, 0, CONFIG_FLAG_NOSAVE },
		{ "ac1_voltage", DATA_TYPE_FLOAT, &s->data.ac1_voltage, 0, 0, CONFIG_FLAG_NOSAVE },
		{ "ac1_frequency", DATA_TYPE_FLOAT, &s->data.ac1_frequency, 0, 0, CONFIG_FLAG_NOSAVE },
		{ "ac1_current", DATA_TYPE_FLOAT, &s->data.ac1_current, 0, 0, CONFIG_FLAG_NOSAVE },
		{ "ac1_power", DATA_TYPE_FLOAT, &s->data.ac1_power, 0, 0, CONFIG_FLAG_NOSAVE },
		{ "battery_voltage", DATA_TYPE_FLOAT, &s->data.battery_voltage, 0, 0, CONFIG_FLAG_NOSAVE },
		{ "battery_current", DATA_TYPE_FLOAT, &s->data.battery_current, 0, 0, CONFIG_FLAG_NOSAVE },
		{ "battery_power", DATA_TYPE_FLOAT, &s->data.battery_power, 0, 0, CONFIG_FLAG_NOSAVE },
		{ "battery_temp", DATA_TYPE_FLOAT, &s->data.battery_temp, 0, 0, CONFIG_FLAG_NOSAVE },
		{ "battery_soc", DATA_TYPE_FLOAT, &s->data.battery_soc, 0, 0, CONFIG_FLAG_NOSAVE },
		{ "battery_soh", DATA_TYPE_FLOAT, &s->data.battery_soh, 0, 0, CONFIG_FLAG_NOSAVE },
		{ "battery_cvsp", DATA_TYPE_FLOAT, &s->data.battery_cvsp, 0, 0, CONFIG_FLAG_NOSAVE },
		{ "relay1", DATA_TYPE_BOOL, &s->data.relay1, 0, 0, CONFIG_FLAG_NOSAVE },
		{ "relay2", DATA_TYPE_BOOL, &s->data.relay2, 0, 0, CONFIG_FLAG_NOSAVE },
		{ "s1_relay1", DATA_TYPE_BOOL, &s->data.s1_relay1, 0, 0, CONFIG_FLAG_NOSAVE },
		{ "s1_relay2", DATA_TYPE_BOOL, &s->data.s1_relay2, 0, 0, CONFIG_FLAG_NOSAVE },
		{ "s2_relay1", DATA_TYPE_BOOL, &s->data.s2_relay1, 0, 0, CONFIG_FLAG_NOSAVE },
		{ "s2_relay2", DATA_TYPE_BOOL, &s->data.s2_relay2, 0, 0, CONFIG_FLAG_NOSAVE },
		{ "s3_relay1", DATA_TYPE_BOOL, &s->data.s3_relay1, 0, 0, CONFIG_FLAG_NOSAVE },
		{ "s3_relay2", DATA_TYPE_BOOL, &s->data.s3_relay2, 0, 0, CONFIG_FLAG_NOSAVE },
		{ "GnRn", DATA_TYPE_BOOL, &s->data.GnRn, 0, 0, CONFIG_FLAG_NOSAVE },
		{ "s1_GnRn", DATA_TYPE_BOOL, &s->data.s1_GnRn, 0, 0, CONFIG_FLAG_NOSAVE },
		{ "s2_GnRn", DATA_TYPE_BOOL, &s->data.s2_GnRn, 0, 0, CONFIG_FLAG_NOSAVE },
		{ "s3_GnRn", DATA_TYPE_BOOL, &s->data.s3_GnRn, 0, 0, CONFIG_FLAG_NOSAVE },
		{ "AutoGn", DATA_TYPE_BOOL, &s->data.AutoGn, 0, 0, CONFIG_FLAG_NOSAVE },
		{ "AutoLodExt", DATA_TYPE_BOOL, &s->data.AutoLodExt, 0, 0, CONFIG_FLAG_NOSAVE },
		{ "AutoLodSoc", DATA_TYPE_BOOL, &s->data.AutoLodSoc, 0, 0, CONFIG_FLAG_NOSAVE },
		{ "Tm1", DATA_TYPE_BOOL, &s->data.Tm1, 0, 0, CONFIG_FLAG_NOSAVE },
		{ "Tm2", DATA_TYPE_BOOL, &s->data.Tm2, 0, 0, CONFIG_FLAG_NOSAVE },
		{ "ExtPwrDer", DATA_TYPE_BOOL, &s->data.ExtPwrDer, 0, 0, CONFIG_FLAG_NOSAVE },
		{ "ExtVfOk", DATA_TYPE_BOOL, &s->data.ExtVfOk, 0, 0, CONFIG_FLAG_NOSAVE },
		{ "GdOn", DATA_TYPE_BOOL, &s->data.GdOn, 0, 0, CONFIG_FLAG_NOSAVE },
		{ "GnOn", DATA_TYPE_BOOL, &s->data.GnOn, 0, 0, CONFIG_FLAG_NOSAVE },
		{ "Error", DATA_TYPE_BOOL, &s->data.Error, 0, 0, CONFIG_FLAG_NOSAVE },
		{ "Run", DATA_TYPE_BOOL, &s->data.Run, 0, 0, CONFIG_FLAG_NOSAVE },
		{ "BatFan", DATA_TYPE_BOOL, &s->data.BatFan, 0, 0, CONFIG_FLAG_NOSAVE },
		{ "AcdCir", DATA_TYPE_BOOL, &s->data.AcdCir, 0, 0, CONFIG_FLAG_NOSAVE },
		{ "MccBatFan", DATA_TYPE_BOOL, &s->data.MccBatFan, 0, 0, CONFIG_FLAG_NOSAVE },
		{ "MccAutoLod", DATA_TYPE_BOOL, &s->data.MccAutoLod, 0, 0, CONFIG_FLAG_NOSAVE },
		{ "Chp", DATA_TYPE_BOOL, &s->data.Chp, 0, 0, CONFIG_FLAG_NOSAVE },
		{ "ChpAdd", DATA_TYPE_BOOL, &s->data.ChpAdd, 0, 0, CONFIG_FLAG_NOSAVE },
		{ "SiComRemote", DATA_TYPE_BOOL, &s->data.SiComRemote, 0, 0, CONFIG_FLAG_NOSAVE },
		{ "OverLoad", DATA_TYPE_BOOL, &s->data.OverLoad, 0, 0, CONFIG_FLAG_NOSAVE },
		{ "ExtSrcConn", DATA_TYPE_BOOL, &s->data.ExtSrcConn, 0, 0, CONFIG_FLAG_NOSAVE },
		{ "Silent", DATA_TYPE_BOOL, &s->data.Silent, 0, 0, CONFIG_FLAG_NOSAVE },
		{ "Current", DATA_TYPE_BOOL, &s->data.Current, 0, 0, CONFIG_FLAG_NOSAVE },
		{ "FeedSelfC", DATA_TYPE_BOOL, &s->data.FeedSelfC, 0, 0, CONFIG_FLAG_NOSAVE },
		{ "Esave", DATA_TYPE_BOOL, &s->data.Esave, 0, 0, CONFIG_FLAG_NOSAVE },
		{ "TotLodPwr", DATA_TYPE_FLOAT, &s->data.TotLodPwr, 0, 0, CONFIG_FLAG_NOSAVE },
		{ "charging_proc", DATA_TYPE_BYTE, &s->data.charging_proc, 0, 0, CONFIG_FLAG_NOSAVE },
		{ "state", DATA_TYPE_BYTE, &s->data.state, 0, 0, CONFIG_FLAG_NOSAVE },
		{ "errmsg", DATA_TYPE_SHORT, &s->data.errmsg, 0, 0, CONFIG_FLAG_NOSAVE },
		{ "ac2_voltage_l1", DATA_TYPE_FLOAT, &s->data.ac2_voltage_l1, 0, 0, CONFIG_FLAG_NOSAVE },
		{ "ac2_voltage_l2", DATA_TYPE_FLOAT, &s->data.ac2_voltage_l2, 0, 0, CONFIG_FLAG_NOSAVE },
		{ "ac2_voltage", DATA_TYPE_FLOAT, &s->data.ac2_voltage, 0, 0, CONFIG_FLAG_NOSAVE },
		{ "ac2_frequency", DATA_TYPE_FLOAT, &s->data.ac2_frequency, 0, 0, CONFIG_FLAG_NOSAVE },
		{ "ac2_current", DATA_TYPE_FLOAT, &s->data.ac2_current, 0, 0, CONFIG_FLAG_NOSAVE },
		{ "ac2_power", DATA_TYPE_FLOAT, &s->data.ac2_power, 0, 0, CONFIG_FLAG_NOSAVE },
		{ "PVPwrAt", DATA_TYPE_FLOAT, &s->data.PVPwrAt, 0, 0, CONFIG_FLAG_NOSAVE },
		{ "GdCsmpPwrAt", DATA_TYPE_FLOAT, &s->data.GdCsmpPwrAt, 0, 0, CONFIG_FLAG_NOSAVE },
		{ "GdFeedPwr", DATA_TYPE_FLOAT, &s->data.GdFeedPwr, 0, 0, CONFIG_FLAG_NOSAVE },
		{0}
	};

	/* Add info_props to config */
	config_add_props(s->ap->cp, "si_data", si_data_props, CONFIG_FLAG_NOSAVE);
}

int si_config(void *h, int req, ...) {
	si_session_t *s = h;
	va_list va;
	void **vp;
	int r;

	r = 1;
	va_start(va,req);
	dprintf(1,"req: %d\n", req);
	switch(req) {
	case SOLARD_CONFIG_INIT:
		dprintf(1,"**** CONFIG INIT *******\n");
		/* 1st arg is AP */
		s->ap = va_arg(va,solard_agent_t *);

		/* Add si_data to config */
		si_config_add_si_data(s);

		/* Init charge params */
		if (!s->readonly) charge_init(s);

		/* Interval cannot be more than 30s */
		if (s->interval > 30) {
			log_warning("interval reduced to 30s (was: %d)\n",s->interval);
			s->interval = 30;
		}

		/* Read and write intervals are the same */
		s->ap->read_interval = s->ap->write_interval = s->interval;

		/* Set startup flag */
		s->startup = 1;

#ifdef JS
		/* Init JS */
		si_jsinit(s);
//		if (s->smanet) smanet_jsinit(s->ap->js, s->smanet);
		smanet_jsinit(s->ap->js);
#endif
		break;

#if 0
		/* Read our specific config */
		getconf(s);
		si_config_smanet(s);
		if (s->smanet) {
			if (smanet_get_value(s->smanet,"ExtSrc",0,&p) == 0) {
				strncpy(s->ExtSrc,p,sizeof(s->ExtSrc)-1);
				dprintf(1,"s->ExtSrc: %s\n", s->ExtSrc);
			}
		}

		/* Readonly? */
		dprintf(1,"readonly: %d\n", s->readonly);
		if (s->readonly)
			si_driver.write  = 0;
		else
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
#endif
	case SOLARD_CONFIG_GET_INFO:
		{
			json_value_t **vp = va_arg(va,json_value_t **);
			dprintf(1,"vp: %p\n", vp);
			if (vp) {
				*vp = si_get_info(s);
				r = 0;
			}
		}
		break;
	case SOLARD_CONFIG_GET_DRIVER:
		dprintf(1,"GET_DRIVER called!\n");
		vp = va_arg(va,void **);
		if (vp) {
			*vp = s->can;
			r = 0;
		}
		break;
	case SOLARD_CONFIG_GET_HANDLE:
		dprintf(1,"GET_HANDLE called!\n");
		vp = va_arg(va,void **);
		if (vp) {
			*vp = s->can_handle;
			r = 0;
		}
		break;
	}
	dprintf(1,"returning: %d\n", r);
	va_end(va);
	return r;
}

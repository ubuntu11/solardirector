
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
	si_session_t *s = ctx;
	char *name;
	config_property_t *p;

	dprintf(dlevel,"args count: %d\n", list_count(args));
	list_reset(args);
	while((name = list_get_next(args)) != 0) {
		dprintf(dlevel,"name: %s\n", name);
		p = config_find_property(s->ap->cp, name);
		dprintf(dlevel,"p: %p\n", p);
		if (!p) {
			sprintf(errmsg,"property %s not found",name);
			return 1;
		}
		dprintf(dlevel,"flags: %04x\n", p->flags);
		if (p->flags & SI_CONFIG_FLAG_SMANET) {
			char *text;
			double d;

			dprintf(dlevel,"smanet: %p, connected: %d\n", s->smanet, s->smanet_connected);
			/* This shouldnt happen */
			if (!s->smanet || !s->smanet_connected) return 1;
			if (smanet_get_value(s->smanet, p->name, &d, &text)) return 1;
			dprintf(dlevel,"p->type: %d(%s)\n", p->type, typestr(p->type));
			if (p->dest && p->flags & CONFIG_FLAG_ALLOC) free(p->dest);
			if (text) {
				p->dsize = strlen(text)+1;
				p->dest = malloc(p->dsize);
				if (!p->dest) {
					log_syserror("si_get_value: malloc(%d)",p->dsize);
					strcpy(errmsg,"memory allocation error");
					return 1;
				}
				p->len = conv_type(p->type,p->dest,p->dsize,DATA_TYPE_STRING,text,strlen(text));
			} else {
				p->dsize = sizeof(d);
				p->dest = malloc(p->dsize);
				if (!p->dest) {
					log_syserror("si_get_value: malloc(%d)",p->dsize);
					strcpy(errmsg,"memory allocation error");
					return 1;
				}
				p->len = conv_type(p->type,p->dest,p->dsize,DATA_TYPE_DOUBLE,&d,0);
			}
			solard_set_bit(p->flags,CONFIG_FLAG_ALLOC);
		}
	}
	return 0;
}

static int si_set_value(void *ctx, list args, char *errmsg) {
	si_session_t *s = ctx;
	char **argv, *name, *value;
	config_property_t *p;

	dprintf(dlevel,"args: %p\n", args);
	dprintf(dlevel,"args count: %d\n", list_count(args));
	list_reset(args);
	while((argv = list_get_next(args)) != 0) {
		name = argv[0];
		value = argv[1];
		dprintf(dlevel,"name: %s, value: %s\n", name, value);
		p = config_find_property(s->ap->cp, name);
		dprintf(dlevel,"p: %p\n", p);
		if (!p) {
			sprintf(errmsg,"property %s not found",name);
			return 1;
		}
		if (p->flags & SI_CONFIG_FLAG_SMANET) {
			if (!s->smanet || !s->smanet_connected) return 1;
			dprintf(dlevel,"p->type: %d(%s)\n", p->type, typestr(p->type));
			if (p->dest && p->flags & CONFIG_FLAG_ALLOC) free(p->dest);
			if (p->type == DATA_TYPE_STRING) {
				p->dsize = strlen(value)+1;
				p->dest = malloc(p->dsize);
				if (!p->dest) {
					log_syserror("si_set_value: malloc(%d)",p->dsize);
					strcpy(errmsg,"memory allocation error");
					return 1;
				}
				strcpy(p->dest,value);
				/* Set the value */
				if (smanet_set_value(s->smanet, p->name, 0, value)) return 1;
			} else {
				double d;

				p->dsize = typesize(p->type);
				p->dest = malloc(p->dsize);
				if (!p->dest) {
					log_syserror("si_set_value: malloc(%d)",p->dsize);
					strcpy(errmsg,"memory allocation error");
					return 1;
				}
				conv_type(DATA_TYPE_DOUBLE,&d,sizeof(d),DATA_TYPE_STRING,value,strlen(value));
				if (smanet_set_value(s->smanet, p->name, d, 0)) return 1;
			}
			solard_set_bit(p->flags,CONFIG_FLAG_ALLOC);
		}
		p->len = conv_type(p->type,p->dest,p->dsize,DATA_TYPE_STRING,value,strlen(value));
		p->dirty = 1;
	}
	config_write(s->ap->cp);
	return 0;
}

static int si_cf_can_init(void *ctx, list args, char *errmsg) {
	si_session_t *s = ctx;
	char **argv;

	/* We take 3 args: transport, target, topts */
	list_reset(args);
	argv = list_get_next(args);
	strncpy(s->can_transport,argv[0],sizeof(s->can_transport)-1);
	strncpy(s->can_target,argv[1],sizeof(s->can_target)-1);
	strncpy(s->can_topts,argv[2],sizeof(s->can_topts)-1);
	dprintf(dlevel,"args: transport: %s, target: %s, topts: %s\n", s->can_transport, s->can_target, s->can_topts);
	if (si_can_init(s)) {
		strcpy(errmsg,s->errmsg);
		return 1;
	}
	return 0;
}

static int si_cf_smanet_init(void *ctx, list args, char *errmsg) {
	si_session_t *s = ctx;
	char **argv;

	/* We take 3 args: transport, target, topts */
	list_reset(args);
	argv = list_get_next(args);
	strncpy(s->smanet_transport,argv[0],sizeof(s->smanet_transport)-1);
	strncpy(s->smanet_target,argv[1],sizeof(s->smanet_target)-1);
	strncpy(s->smanet_topts,argv[2],sizeof(s->smanet_topts)-1);
	dprintf(dlevel,"args: transport: %s, target: %s, topts: %s\n", s->smanet_transport, s->smanet_target, s->smanet_topts);
	if (si_smanet_init(s)) {
		strcpy(errmsg,s->errmsg);
		return 1;
	}
	return 0;
}

static void _getsource(si_session_t *s, si_current_source_t *spec) {
	char line[128],*p,*v;
	int i;

	i = 0;
	p = strele(i++,",",spec->text);
	v = line;
	v += sprintf(v,"spec: text: %s, ", spec->text);
	if (strcasecmp(p,"can") == 0) {
		/* can,id,offset,size,type,mult */
		spec->source = CURRENT_SOURCE_CAN;
		v += sprintf(v,"source: %d, ", spec->source);
		p = strele(i++,",",spec->text);
		conv_type(DATA_TYPE_U16,&spec->can.id,0,DATA_TYPE_STRING,p,strlen(p));
		p = strele(i++,",",spec->text);
		conv_type(DATA_TYPE_U8,&spec->can.offset,0,DATA_TYPE_STRING,p,strlen(p));
		p = strele(i++,",",spec->text);
		conv_type(DATA_TYPE_U8,&spec->can.size,0,DATA_TYPE_STRING,p,strlen(p));
		v += sprintf(v,"id: %03x, offset: %d, size: %d, ", spec->can.id, spec->can.offset, spec->can.size);
	} else if (strcasecmp(p,"smanet") == 0) {
		spec->source = CURRENT_SOURCE_SMANET;
		p = strele(i++,",",spec->text);
		strncpy(spec->name,p,sizeof(spec->name)-1);
		v += sprintf(v,"source: %d, name: %s, ", spec->source, spec->name);
	} else {
		log_warning("invalid input_current_source: %s, ignored.\n", s->input.text);
	}
	/* type, mult */
	p = strele(i++,",",spec->text);
	if (*p == 'W' || *p == 'w')
		spec->type = CURRENT_TYPE_WATTS;
	else
		spec->type = CURRENT_TYPE_AMPS;
	p = strele(i++,",",spec->text);
	if (strlen(p)) conv_type(DATA_TYPE_FLOAT,&spec->mult,0,DATA_TYPE_STRING,p,strlen(p));
	else spec->mult = 1.0;
	v += sprintf(v,"type: %d, mult: %f", spec->type, spec->mult);
	dprintf(dlevel,"%s\n", line);
}

int si_agent_init(int argc, char **argv, opt_proctab_t *si_opts, si_session_t *s) {
	config_property_t si_props[] = {
		{ "bms_mode", DATA_TYPE_BOOL, &s->bms_mode, 0, "no", 0, 0, 0, 0, 0, 0, 0, 1, 0 },
		{ "readonly", DATA_TYPE_BOOL, &s->readonly, 0, "yes", 0, 0, 0, 0, 0, 0, 0, 1, 0 },
		{ "startup_charge_mode", DATA_TYPE_INT, &s->startup_charge_mode, 0, "0", 0,
			"select", 3, (int []){ 0, 1, 2 }, 1, (char *[]){ "Off","On","CV" }, 0, 1, 0 },
		{ "charge_mode", DATA_TYPE_INT, &s->charge_mode, 0, "0", 0,
			"select", 3, (int []){ 0, 1, 2 }, 3, (char *[]){ "Off","On","CV" }, 0, 1, 0 },
		{ "charge_method", DATA_TYPE_INT, &s->charge_method, 0, "1", 0,
			"select", 2, (int []){ 0, 1 }, 2, (char *[]){ "CC/CV","oneshot" }, 0, 1, 0 },
		{ "cv_method", DATA_TYPE_INT, &s->cv_method, 0, STRINGIFY(CV_METHOD_AMPS), 0,
			"select", 2, (int []){ 0, 1 }, 2, (char *[]){ "time","amps" }, 0, 1, 0 },
		{ "cv_time", DATA_TYPE_INT, &s->cv_time, 0, "120", 0,
			"range", 3, (int []){ 0, 1440, 1 }, 1, (char *[]){ "CV Time" }, "minutes", 1, 0 },
		{ "cv_cutoff", DATA_TYPE_FLOAT, &s->cv_cutoff, 0, "5", 0,
			0, 0, 0, 1, (char *[]){ "CV Cutoff" }, "V", 1, 0 },
		{ "cv_timeout", DATA_TYPE_BOOL, &s->cv_timeout, 0, "true" },
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
			"range", 3, (float []){ 0, 5000, .1 }, 1, (char *[]){ "Minimum charge current" }, "A", 1, 0 },
		{ "charge_max_amps", DATA_TYPE_FLOAT, &s->charge_max_amps, 0, 0, 0,
			"range", 3, (float []){ 0, 5000, .1 }, 1, (char *[]){ "Maximum charge current" }, "A", 1, 0 },
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
		{ "can_connected", DATA_TYPE_BOOL, &s->can_connected, 0, "N", CONFIG_FLAG_NOSAVE },
		{ "smanet_transport", DATA_TYPE_STRING, &s->smanet_transport, sizeof(s->smanet_transport)-1, 0, 0,
			0, 0, 0, 0, 0, 0, 1, 0 },
		{ "smanet_target", DATA_TYPE_STRING, &s->smanet_target, sizeof(s->smanet_target)-1, 0, 0,
			0, 0, 0, 0, 0, 0, 1, 0 },
		{ "smanet_topts", DATA_TYPE_STRING, &s->smanet_topts, sizeof(s->smanet_topts)-1, 0, 0,
			0, 0, 0, 0, 0, 0, 1, 0 },
		{ "smanet_channels_path", DATA_TYPE_STRING, &s->smanet_channels_path, sizeof(s->smanet_channels_path)-1, 0, 0,
			0, 0, 0, 0, 0, 0, 1, 0 },
		{ "smanet_connected", DATA_TYPE_BOOL, &s->smanet_connected, 0, "N", CONFIG_FLAG_NOSAVE },
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
		{ "input_current_source", DATA_TYPE_STRING, &s->input.text, sizeof(s->input.text)-1, 0, 0,
			0, 0, 0, 1, (char *[]){ "Source of input current data" }, 0, 1, 0 },
		{ "output_current_source", DATA_TYPE_STRING, &s->output.text, sizeof(s->output.text)-1, 0, 0,
			0, 0, 0, 1, (char *[]){ "Source of output current data" }, 0, 1, 0 },
		{ "state", DATA_TYPE_SHORT, &s->state, 0, 0, 0 },
		{ "charge_voltage", DATA_TYPE_FLOAT, &s->charge_voltage, 0, 0, 0 },
		{ "charge_amps_temp_modifier", DATA_TYPE_FLOAT, &s->charge_amps_temp_modifier, 0, 0, 0 },
		{ "charge_amps_soc_modifier", DATA_TYPE_FLOAT, &s->charge_amps_soc_modifier, 0, 0, 0 },
		{ "charge_amps", DATA_TYPE_FLOAT, &s->charge_amps, 0, 0, CONFIG_FLAG_NOSAVE },
		{ "cv_start_time", DATA_TYPE_INT, &s->cv_start_time, 0, 0, 0 },
		{ "grid_charge_start_voltage", DATA_TYPE_FLOAT, &s->grid_charge_start_voltage, 0, 0, 0 },
		{ "grid_charge_start_soc", DATA_TYPE_FLOAT, &s->grid_charge_start_soc, 0, 0, 0 },
		{ "grid_charge_stop_voltage", DATA_TYPE_FLOAT, &s->grid_charge_stop_voltage, 0, 0, 0 },
		{ "grid_charge_stop_soc", DATA_TYPE_FLOAT, &s->grid_charge_stop_soc, 0, 0, 0 },
		{ "gen_started", DATA_TYPE_INT, &s->gen_started, 0, 0, 0 },
		{ "gen_op_time", DATA_TYPE_INT, &s->gen_op_time, 0, 0, 0 },
		{ "soh", DATA_TYPE_FLOAT, &s->soh, 0, 0, 0 },
		{ "tvolt", DATA_TYPE_FLOAT, &s->tvolt, 0, 0, 0 },
		{ "sim_amps", DATA_TYPE_FLOAT, &s->sim_amps, 0, 0, 0 },
#if 0
		{ "last_battery_voltage", DATA_TYPE_FLOAT, &s->last_battery_voltage, 0, 0, 0 },
		{ "last_soc", DATA_TYPE_FLOAT, &s->last_soc, 0, 0, 0 },
		{ "last_charge_voltage", DATA_TYPE_FLOAT, &s->last_charge_voltage, 0, 0, 0 },
		{ "last_battery_amps", DATA_TYPE_FLOAT, &s->last_battery_amps, 0, 0, 0 },
		{ "ec_state", DATA_TYPE_INT, &s->ec_state, 0, 0, 0 },
#endif
		{ "startup", DATA_TYPE_INT, &s->startup, 0, 0, 0 },
		{ "tozero", DATA_TYPE_INT, &s->tozero, 0, 0, 0 },
		{ "errmsg", DATA_TYPE_STRING, s->errmsg, sizeof(s->errmsg)-1, 0, 0 },
		{ "force_charge", DATA_TYPE_BOOL, &s->force_charge, 0, 0, 0 },
		{ "smanet_added", DATA_TYPE_BOOL, &s->smanet_added, 0, 0, 0 },
		{ 0 }
	};
	config_function_t si_funcs[] = {
		{ "get", si_get_value, s, 1 },
		{ "set", si_set_value, s, 2 },
		{ "can_init", si_cf_can_init, s, 3 },
		{ "smanet_init", si_cf_smanet_init, s, 3 },
		{0}
	};

	s->ap = agent_init(argc,argv,si_opts,&si_driver,s,si_props,si_funcs);
	dprintf(dlevel,"ap: %p\n",s->ap);
	if (!s->ap) return 1;

	if (strlen(s->input.text)) _getsource(s,&s->input);
	if (strlen(s->output.text)) _getsource(s,&s->output);
	return 0;
}

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
	dprintf(dlevel,"req: %d\n", req);
	switch(req) {
	case SOLARD_CONFIG_INIT:
		dprintf(dlevel,"**** CONFIG INIT *******\n");
		/* 1st arg is AP */
		s->ap = va_arg(va,solard_agent_t *);

		/* Add si_data to config */
		si_config_add_si_data(s);

#if 0
		/* Set min/max for volts if not set */
		if (!si_isvrange(s->min_voltage)) {
			log_warning("setting min_voltage to %.1f\n", 41.0);
			s->min_voltage = 41.0;
		}
		if (!si_isvrange(s->max_voltage)) {
			log_warning("setting max_voltage to %.1f\n", 58.1);
			s->max_voltage = 58.1;
		}
#endif

		/* Interval cannot be more than 30s */
		if (s->interval > 30) {
			log_warning("interval reduced to 30s (was: %d)\n",s->interval);
			s->interval = 30;
		}

		/* Read and write intervals are the same */
		s->ap->read_interval = s->ap->write_interval = s->interval;

		/* Set startup flag */
		s->startup = 1;

		/* XXX smanet needs to be created here (before jsinit) */
		s->smanet = smanet_init(0,0,0);

#ifdef JS
		/* Init JS */
		si_jsinit(s);
		smanet_jsinit(s->ap->js);
#endif
		r = 0;
		break;
	case SOLARD_CONFIG_GET_INFO:
		{
			json_value_t **vp = va_arg(va,json_value_t **);
			dprintf(dlevel,"vp: %p\n", vp);
			if (vp) {
				*vp = si_get_info(s);
				r = 0;
			}
		}
		break;
	case SOLARD_CONFIG_GET_DRIVER:
		dprintf(dlevel,"GET_DRIVER called!\n");
		vp = va_arg(va,void **);
		if (vp) {
			*vp = s->can;
			r = 0;
		}
		break;
	case SOLARD_CONFIG_GET_HANDLE:
		dprintf(dlevel,"GET_HANDLE called!\n");
		vp = va_arg(va,void **);
		if (vp) {
			*vp = s->can_handle;
			r = 0;
		}
		break;
	}
	dprintf(dlevel,"returning: %d\n", r);
	va_end(va);
	return r;
}

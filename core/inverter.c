
#if 0
/*
Copyright (c) 2021, Stephen P. Shoecraft
All rights reserved.

This source code is licensed under the BSD-style license found in the
LICENSE file in the root directory of this source tree.
*/

#include "inverter.h"

static struct inverter_states {
	int mask;
	char *label;
} states[] = {
	{ SOLARD_INVERTER_STATE_RUNNING, "Running" },
	{ SOLARD_INVERTER_STATE_CHARGING, "Charging" },
	{ SOLARD_INVERTER_STATE_GRID, "GridConnected" },
	{ SOLARD_INVERTER_STATE_GEN, "GeneratorRunning" },
};
#define NSTATES (sizeof(states)/sizeof(struct inverter_states))

/* PV to DC inverter (aka "charge controler") */
#if 0
class: Inverter
subclass: dc-dc
capabilities:	input_voltage,
		input_current,
		output_voltage,
		output_current,
		charge_start_voltage,
		charge_end_voltage,
		yeild,

subclass: dc-ac
capabilities:	input_voltage,
		input_current,
		output_voltage,
		output_current,
		output_frequecy,
		yeild,

class: Inverter
subclass: ac-dc-ac
capabilities:	input_voltage,
		input_current,
		input_frequecy,
		output_voltage,
		output_current,
		output_frequecy,
		Grid,
		Generator,
		run_state,
		charge_state,
		charge_control,
		input_control
		output_control

class: Storage
subclass: Battery
capabilities:	capacity,
		voltage,
		current,
		temperature,
		cell_voltages,
		cell_resistances,
		charge_state,
		discharge_state,
		balance_state,
		charge_control,
		discharge_control,
		balance_control,
expect:
	{ 	"capacity": "XX%",
		"voltage": 1234.00,
		"current": 10.0,
		"temps": [ 25 ],
		"cellvolt": [ 1.0, 2.0 ],
		"cellres": [ 1.0, 2.0 ],
		"states": [ "Charging", "Discharging", "Balancing" ],
		"Status": 0,
		"Errmsg": "No error"
	}
#endif

static void _get_state(char *name, void *dest, int len, json_value_t *v) {
	solard_inverter_t *inv = dest;
	list lp;
	char *p;
	int i;

	dprintf(4,"value: %s\n", v->value.string.chars);
	conv_type(DATA_TYPE_LIST,&lp,0,DATA_TYPE_STRING,v->value.string.chars,v->value.string.length);

	inv->state = 0;
	list_reset(lp);
	while((p = list_get_next(lp)) != 0) {
		dprintf(6,"p: %s\n", p);
		for(i=0; i < NSTATES; i++) {
			if (strcmp(p,states[i].label) == 0) {
				inv->state |= states[i].mask;
				break;
			}
		}
	}
	dprintf(4,"state: %x\n", inv->state);
	list_destroy(lp);
}

static void _set_state(char *name, void *dest, int len, json_value_t *v) {
	solard_inverter_t *inv = dest;
	char temp[128],*p;
	int i,j;

	dprintf(4,"state: %x\n", inv->state);

	/* States */
	temp[0] = 0;
	p = temp;
	for(i=j=0; i < NSTATES; i++) {
		if (solard_check_state(inv,states[i].mask)) {
			if (j) p += sprintf(p,",");
			p += sprintf(p,states[i].label);
			j++;
		}
	}
	dprintf(4,"temp: %s\n", temp);
	json_add_string(v, "states", temp);
}

/* XXX these dont need to be reported via MQTT */
#if 0
		{ "max_voltage",DATA_TYPE_FLOAT,&inv->max_voltage,0,0 }, \
		{ "min_voltage",DATA_TYPE_FLOAT,&inv->min_voltage,0,0 }, \
		{ "charge_start_voltage",DATA_TYPE_FLOAT,&inv->charge_start_voltage,0,0 }, \
		{ "charge_end_voltage",DATA_TYPE_FLOAT,&inv->charge_end_voltage,0,0 }, \
		{ "charge_at_max",DATA_TYPE_BOOL,&inv->charge_at_max,0,0 }, \
		{ "charge_amps",DATA_TYPE_FLOAT,&inv->charge_amps,0,0 }, \
		{ "discharge_amps",DATA_TYPE_FLOAT,&inv->discharge_amps,0,0 },
		{ "soh",DATA_TYPE_FLOAT,&inv->soh,0,0 },
#endif
#define INVERTER_TAB(ACTION) \
		{ "name",DATA_TYPE_STRING,&inv->name,sizeof(inv->name)-1,0 }, \
		{ "type",DATA_TYPE_INT,&inv->type,0,0 }, \
		{ "battery_voltage",DATA_TYPE_FLOAT,&inv->battery_voltage,0,0 }, \
		{ "battery_amps",DATA_TYPE_FLOAT,&inv->battery_amps,0,0 }, \
		{ "battery_watts",DATA_TYPE_FLOAT,&inv->battery_watts,0,0 }, \
		{ "battery_temp",DATA_TYPE_FLOAT,&inv->battery_temp,0,0 }, \
		{ "capacity",DATA_TYPE_FLOAT,&inv->soc,0,0 }, \
		{ "grid_voltage_l1",DATA_TYPE_FLOAT,&inv->grid_voltage.l1,0,0 }, \
		{ "grid_voltage_l2",DATA_TYPE_FLOAT,&inv->grid_voltage.l2,0,0 }, \
		{ "grid_voltage_l3",DATA_TYPE_FLOAT,&inv->grid_voltage.l3,0,0 }, \
		{ "grid_voltage_total",DATA_TYPE_FLOAT,&inv->grid_voltage.total,0,0 }, \
		{ "grid_frequency",DATA_TYPE_FLOAT,&inv->grid_frequency,0,0 }, \
		{ "grid_amps_l1",DATA_TYPE_FLOAT,&inv->grid_amps.l1,0,0 }, \
		{ "grid_amps_l2",DATA_TYPE_FLOAT,&inv->grid_amps.l2,0,0 }, \
		{ "grid_amps_l3",DATA_TYPE_FLOAT,&inv->grid_amps.l3,0,0 }, \
		{ "grid_amps_total",DATA_TYPE_FLOAT,&inv->grid_amps.total,0,0 }, \
		{ "grid_watts_l1",DATA_TYPE_FLOAT,&inv->grid_watts.l1,0,0 }, \
		{ "grid_watts_l2",DATA_TYPE_FLOAT,&inv->grid_watts.l2,0,0 }, \
		{ "grid_watts_l3",DATA_TYPE_FLOAT,&inv->grid_watts.l3,0,0 }, \
		{ "grid_watts_total",DATA_TYPE_FLOAT,&inv->grid_watts.total,0,0 }, \
		{ "load_voltage_l1",DATA_TYPE_FLOAT,&inv->load_voltage.l1,0,0 }, \
		{ "load_voltage_l2",DATA_TYPE_FLOAT,&inv->load_voltage.l2,0,0 }, \
		{ "load_voltage_l3",DATA_TYPE_FLOAT,&inv->load_voltage.l3,0,0 }, \
		{ "load_voltage_total",DATA_TYPE_FLOAT,&inv->load_voltage.total,0,0 }, \
		{ "load_frequency",DATA_TYPE_FLOAT,&inv->load_frequency,0,0 }, \
		{ "load_watts",DATA_TYPE_FLOAT,&inv->load_watts,0,0 }, \
		{ "site_voltage_l1",DATA_TYPE_FLOAT,&inv->site_voltage.l1,0,0 }, \
		{ "site_voltage_l2",DATA_TYPE_FLOAT,&inv->site_voltage.l2,0,0 }, \
		{ "site_voltage_l3",DATA_TYPE_FLOAT,&inv->site_voltage.l3,0,0 }, \
		{ "site_voltage_total",DATA_TYPE_FLOAT,&inv->site_voltage.total,0,0 }, \
		{ "site_frequency",DATA_TYPE_FLOAT,&inv->site_frequency,0,0 }, \
		{ "site_amps_l1",DATA_TYPE_FLOAT,&inv->site_amps.l1,0,0 }, \
		{ "site_amps_l2",DATA_TYPE_FLOAT,&inv->site_amps.l2,0,0 }, \
		{ "site_amps_l3",DATA_TYPE_FLOAT,&inv->site_amps.l3,0,0 }, \
		{ "site_watts_l1",DATA_TYPE_FLOAT,&inv->site_watts.l1,0,0 }, \
		{ "site_watts_l2",DATA_TYPE_FLOAT,&inv->site_watts.l2,0,0 }, \
		{ "site_watts_l3",DATA_TYPE_FLOAT,&inv->site_watts.l3,0,0 }, \
		{ "site_watts_total",DATA_TYPE_FLOAT,&inv->site_watts.total,0,0 }, \
		{ "errcode",DATA_TYPE_INT,&inv->errcode,0,0 }, \
		{ "errmsg",DATA_TYPE_STRING,&inv->errmsg,sizeof(inv->errmsg)-1,0 }, \
		{ "state",0,inv,0,ACTION }, \
		JSON_PROCTAB_END

static void _dump_state(char *name, void *dest, int flen, json_value_t *v) {
	solard_inverter_t *inv = dest;
	char format[16];
	int *dlevel = (int *) v;

	sprintf(format,"%%%ds: %%d\n",flen);
	if (debug >= *dlevel) printf(format,name,inv->state);
}

void inverter_dump(solard_inverter_t *inv, int dlevel) {
	json_proctab_t tab[] = { INVERTER_TAB(_dump_state) }, *p;
	char format[16],temp[1024];
	int flen;

	flen = 0;
	for(p=tab; p->field; p++) {
		if (strlen(p->field) > flen)
			flen = strlen(p->field);
	}
	flen++;
	sprintf(format,"%%%ds: %%s\n",flen);
	dprintf(dlevel,"inverter:\n");
	for(p=tab; p->field; p++) {
		if (p->cb) p->cb(p->field,p->ptr,flen,(void *)&dlevel);
		else {
			conv_type(DATA_TYPE_STRING,&temp,sizeof(temp)-1,p->type,p->ptr,p->len);
			if (debug >= dlevel) printf(format,p->field,temp);
		}
	}
}

int inverter_from_json(solard_inverter_t *inv, char *str) {
	json_proctab_t inverter_tab[] = { INVERTER_TAB(_get_state) };
	json_value_t *v;

	v = json_parse(str);
	memset(inv,0,sizeof(*inv));
	json_to_tab(inverter_tab,v);
//	inverter_dump(inv,3);
	json_destroy(v);
	return 0;
};

json_value_t *inverter_to_json(solard_inverter_t *inv) {
	json_proctab_t inverter_tab[] = { INVERTER_TAB(_set_state) };

	return json_from_tab(inverter_tab);
}

#define isvrange(v) ((v >= 1.0) && (v  <= 1000.0))

int inverter_check_parms(solard_inverter_t *inv) {
	int r;
	char *msg;

	r = 1;
	/* min and max must be set */
	dprintf(1,"min_voltage: %.1f, max_voltage: %.1f\n", inv->min_voltage, inv->max_voltage);
	if (!isvrange(inv->min_voltage)) {
		msg = "min_voltage not set or out of range\n";
		goto inverter_check_parms_error;
	}
	if (!isvrange(inv->max_voltage)) {
		msg = "max_voltage not set or out of range\n";
		goto inverter_check_parms_error;
	}
	/* min must be < max */
	if (inv->min_voltage >= inv->max_voltage) {
		msg = "min_voltage > max_voltage\n";
		goto inverter_check_parms_error;
	}
	/* max must be > min */
	if (inv->max_voltage <= inv->min_voltage) {
		msg = "min_voltage > max_voltage\n";
		goto inverter_check_parms_error;
	}
	/* charge_start_voltage must be >= min */
	dprintf(1,"charge_start_voltage: %.1f, charge_end_voltage: %.1f\n", inv->charge_start_voltage, inv->charge_end_voltage);
	if (inv->charge_start_voltage < inv->min_voltage) {
		msg = "charge_start_voltage < min_voltage\n";
		goto inverter_check_parms_error;
	}
	/* charge_start_voltage must be <= max */
	if (inv->charge_start_voltage > inv->max_voltage) {
		msg = "charge_start_voltage > max_voltage\n";
		goto inverter_check_parms_error;
	}
	/* charge_end_voltage must be >= min */
	if (inv->charge_end_voltage < inv->min_voltage) {
		msg = "charge_end_voltage < min_voltage\n";
		goto inverter_check_parms_error;
	}
	/* charge_end_voltage must be <= max */
	if (inv->charge_end_voltage > inv->max_voltage) {
		msg = "charge_end_voltage > max_voltage\n";
		goto inverter_check_parms_error;
	}
	/* charge_start_voltage must be < charge_end_voltage */
	if (inv->charge_start_voltage > inv->charge_end_voltage) {
		goto inverter_check_parms_error;
	}
	r = 0;
	msg = "";

inverter_check_parms_error:
	strcpy(inv->errmsg,msg);
	return r;
}

#ifdef JS
#include "js_solard_inverter.h"
enum INVERTER_PROPERTY_ID {
	SOLARD_INVERTER_PROPIDS
};

static JSBool inverter_getprop(JSContext *cx, JSObject *obj, jsval id, jsval *rval) {
	int prop_id;
	solard_inverter_t *inv;

	inv = JS_GetPrivate(cx,obj);
	dprintf(4,"inv: %p\n", inv);
	if (!inv) {
		JS_ReportError(cx, "private is null!");
		return JS_FALSE;
	}

	dprintf(4,"is_int: %d\n", JSVAL_IS_INT(id));
	if(JSVAL_IS_INT(id)) {
		prop_id = JSVAL_TO_INT(id);
		dprintf(4,"prop_id: %d\n", prop_id);
		switch(prop_id) {
		SOLARD_INVERTER_GETPROP
		default:
			JS_ReportError(cx, "not a property");
			*rval = JSVAL_NULL;
		}
	}
	return JS_TRUE;
}

static JSClass inverter_class = {
	"inverter",
	JSCLASS_HAS_PRIVATE,
	JS_PropertyStub,
	JS_PropertyStub,
	inverter_getprop,
	JS_PropertyStub,
	JS_EnumerateStub,
	JS_ResolveStub,
	JS_ConvertStub,
	JS_FinalizeStub,
	JSCLASS_NO_OPTIONAL_MEMBERS
};

JSObject *JSInverter(JSContext *cx, solard_inverter_t *bp) {
	JSPropertySpec inverter_props[] = { 
		{0}
	};
	JSFunctionSpec inverter_funcs[] = {
		{ 0 }
	};
	JSObject *obj;

	dprintf(5,"defining %s object\n",inverter_class.name);
#if 0
	obj = JS_InitClass(cx, JS_GetGlobalObject(cx), 0, &inverter_class, 0, 0, inverter_props, inverter_funcs, 0, 0);
	if (!obj) {
		JS_ReportError(cx,"unable to initialize inverter class");
		return 0;
	}
#endif
	if ((obj = JS_DefineObject(cx, JS_GetGlobalObject(cx), inverter_class.name, &inverter_class, NULL, JSPROP_ENUMERATE)) == 0) return 0;
	if (!JS_DefineProperties(cx, obj, inverter_props)) return 0;
	if (!JS_DefineFunctions(cx, obj, inverter_funcs)) return 0;

	/* Pre-create the JSVALs */
	JS_SetPrivate(cx,obj,bp);
	dprintf(5,"done!\n");
	return obj;
}
#endif
#endif

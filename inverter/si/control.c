
/*
Copyright (c) 2021, Stephen P. Shoecraft
All rights reserved.

This source code is licensed under the BSD-style license found in the
LICENSE file in the root directory of this source tree.
*/

#include "si.h"

int si_control_add_info(si_session_t *s, json_value_t *v) {
	return 0;
}

static int getval(int *val, int dt, json_value_t *ov) {
	switch(ov->type) {
	case JSONString:
		conv_type(dt,val,0,DATA_TYPE_STRING,ov->value.string.chars,ov->value.string.length);
		break;
	case JSONNumber:
		conv_type(dt,val,0,DATA_TYPE_DOUBLE,&ov->value.number,0);
		break;
	case JSONBoolean:
		conv_type(dt,val,0,DATA_TYPE_INT,&ov->value.boolean,0);
		break;
	default:
		dprintf(1,"invalid type\n");
		return 1;
		break;
	}
	dprintf(1,"returning: %d\n", *val);
	return 0;
}

/* XXX TODO: add range validation? */
static int si_getcontrol(si_session_t *s, json_object_t *o) {
	solard_inverter_t *inv = s->ap->role_data;
	json_value_t *ov;
	char *name;
	int i,r,val;

	r = 1;
	dprintf(1,"count: %d\n", o->count);
	for(i=0; i < o->count; i++) {
		name = o->names[i];
		ov = o->values[i];
		dprintf(1,"ov name: %s, type: %d(%s)\n", name, ov->type, json_typestr(ov->type));
		if (strcasecmp(name,"Charge")==0) {
			if (getval(&val, DATA_TYPE_INT, ov)) return 1;
			dprintf(1,"val: %d\n", val);
			switch(val) {
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
			r = 0;
		} else if (strcasecmp(name,"SIM")==0) {
			if (getval(&val, DATA_TYPE_INT, ov)) return 1;
			dprintf(1,"val: %d\n", val);
			if (val) {
				if (!s->sim) {
					lprintf(0,"**** STARTING SIM ****\n");
					s->sim = 1;
					s->tvolt = inv->battery_voltage;
				}
			} else {
				if (s->sim) {
					lprintf(0,"**** STOPPING SIM ****\n");
					s->sim = 0;
				}
			}
			r = 0;
		} else if (strcasecmp(name,"charge_at_max")==0) {
			if (getval(&val, DATA_TYPE_BOOL, ov)) return 1;
			printf("val: %d, at_max: %d\n", val, inv->charge_at_max);
			if (val && !inv->charge_at_max) charge_max_start(s);
			if (!val && inv->charge_at_max) charge_max_stop(s);
			r = 0;
		} else if (strcasecmp(name,"Run")==0) {
			if (getval(&val, DATA_TYPE_BOOL, ov)) return 1;
			printf("val: %d, state: %d\n", val, s->bits.Run);
			if (val && !s->bits.Run) {
				r = si_startstop(s,1);
				if (!r) log_write(LOG_INFO,"Started Sunny Island\n");
			}
			if (!val && s->bits.Run) {
				r = si_startstop(s,0);
				if (!r) {
					log_write(LOG_INFO,"Stopped Sunny Island\n");
					charge_stop(s,1);
				}
			}
		}
	}
	dprintf(1,"returning: %d\n", r);
	return r;
}

int si_control(void *handle,char *op, char *id, json_value_t *actions) {
	si_session_t *s = handle;
	int status;
	char *message;

	status = 1;

	dprintf(1,"type: %d\n", actions->type);
	if (actions->type == JSONObject) {
		si_getcontrol(s, actions->value.object);
		dprintf(1,"one object...\n");
	} else if (actions->type == JSONArray) {
		json_array_t *a;
		int i;

		dprintf(1,"array of objects...\n");
		a = actions->value.array;
		dprintf(1,"count: %d\n", a->count);
		for(i=0; i < a->count; i++) {
			si_getcontrol(s, a->items[i]->value.object);
		}
	} else {
		message = "invalid request";
		goto si_control_error;
	}
	dprintf(1,"back\n");

	status = 0;
	message = "success";

si_control_error:
	agent_send_status(s->ap, s->ap->name, "Control", op, id, status, message);
	dprintf(1,"returning: %d\n", status);
	return status;
}

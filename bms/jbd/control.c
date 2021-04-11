
/*
Copyright (c) 2021, Stephen P. Shoecraft
All rights reserved.

This source code is licensed under the BSD-style license found in the
LICENSE file in the root directory of this source tree.
*/

#include "jbd.h"

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
static int jbd_getcontrol(jbd_session_t *s, json_object_t *o, int *c, int *d, int *b) {
	int i;
	json_value_t *ov;
	int val;

	dprintf(1,"count: %d\n", o->count);
	for(i=0; i < o->count; i++) {
		ov = o->values[i];
		dprintf(1,"ov name: %s, type: %d(%s)\n", o->names[i], ov->type, json_typestr(ov->type));
		dprintf(1,"name: %s\n", o->names[i]);
		if (strcmp(o->names[i],"Charging")==0) {
			if (getval(&val, DATA_TYPE_BOOL, ov)) return 1;
			*c = val;
		} else if (strcmp(o->names[i],"Discharging")==0) {
			if (getval(&val, DATA_TYPE_BOOL, ov)) return 1;
			*d = val;
		} else if (strcmp(o->names[i],"Balancing")==0) {
			if (getval(&val, DATA_TYPE_INT, ov)) return 1;
			*b = val;
		}
	}
	return 0;
}

int jbd_control(void *handle,char *op, char *id, json_value_t *actions) {
	jbd_session_t *s = handle;
	int opt,charge,discharge,balance,status,bytes;
	uint8_t data[2];
	char *message;

	status = 1;

	charge = discharge = balance = -1;
	dprintf(1,"type: %d\n", actions->type);
	if (actions->type == JSONObject) {
		jbd_getcontrol(s, actions->value.object, &charge, &discharge, &balance);
		dprintf(1,"one object...\n");
	} else if (actions->type == JSONArray) {
		json_array_t *a;
		int i;

		dprintf(1,"array of objects...\n");
		a = actions->value.array;
		dprintf(1,"count: %d\n", a->count);
		for(i=0; i < a->count; i++) {
			jbd_getcontrol(s, a->items[i]->value.object, &charge, &discharge, &balance);
		}
	} else {
		message = "invalid request";
		goto jbd_control_error;
	}
	dprintf(1,"back\n");

	/* If modifying charge/discharge, get the current fetstate before enabling eeprom */
        if (charge >= 0 || discharge >= 0) {
		if (jbd_get_fetstate(s)) {
			message = "I/O error";
			goto jbd_control_error;
		}
	}

	/* Now that we've collected all the actions, apply them */
	if (jbd_eeprom_start(s) < 0) return 1;
        dprintf(2,"charge: %d, discharge: %d\n", charge, discharge);
        if (charge >= 0 || discharge >= 0) {
                opt = 0;
		dprintf(2,"fetstate: %x\n", s->fetstate);
		/* Each bit set TURNS IT OFF */
		if ((s->fetstate & JBD_MOS_CHARGE)==0) opt = JBD_MOS_CHARGE;
		if ((s->fetstate & JBD_MOS_DISCHARGE)==0) opt = JBD_MOS_DISCHARGE;
		dprintf(2,"opt: %x\n", opt);
		if (charge == 0) opt |= JBD_MOS_CHARGE;
		else if (charge == 1) opt &= ~JBD_MOS_CHARGE;
		if (discharge == 0) opt |= JBD_MOS_DISCHARGE;
		else if (discharge == 1) opt &= ~JBD_MOS_DISCHARGE;
		dprintf(2,"opt: %x\n", opt);
		jbd_putshort(data,opt);
		bytes = jbd_rw(s, JBD_CMD_WRITE, JBD_REG_MOSFET, data, sizeof(data));
		dprintf(1,"bytes: %d\n", bytes);
		if (bytes < 0) {
			message = "I/O error";
			goto jbd_control_error;
		}
        }
	dprintf(2,"balance: %d\n", balance);
	if (balance >= 0) {
		list lp;
		char *p;

		/* Get current value */
		bytes = jbd_rw(s, JBD_CMD_READ, JBD_REG_FUNCMASK, data, sizeof(data));
		dprintf(1,"bytes: %d\n", bytes);
		if (bytes < 0) {
			message = "I/O error";
			goto jbd_control_error;
		}
		opt = jbd_getshort(data);
		dprintf(2,"opt: %x\n", opt);
		switch(balance) {
		case 0:
			opt &= ~(JBD_FUNC_BALANCE_EN | JBD_FUNC_CHG_BALANCE);
			break;
		case 1:
			opt &= ~(JBD_FUNC_BALANCE_EN | JBD_FUNC_CHG_BALANCE);
			opt |= JBD_FUNC_BALANCE_EN;
			break;
		case 2:
			opt |= (JBD_FUNC_BALANCE_EN | JBD_FUNC_CHG_BALANCE);
			break;
		}
		dprintf(2,"opt: %x\n", opt);
		jbd_putshort(data,opt);
		bytes = jbd_rw(s, JBD_CMD_WRITE, JBD_REG_FUNCMASK, data, sizeof(data));
		dprintf(1,"bytes: %d\n", bytes);
		if (bytes < 0) {
			message = "I/O error";
			goto jbd_control_error;
		}
		s->balancing = balance;
		/* For balance, we need to pub the config for it */
		lp = list_create();
		p = "BatteryConfig";
		list_add(lp,p,0);
		jbd_config(s,"Get","jbd_control",lp);
		list_destroy(lp);
	}

	status = 0;
	message = "success";

jbd_control_error:
	jbd_eeprom_end(s);
	agent_send_status(s->conf, s->conf->name, "Control", op, id, status, message);
	dprintf(1,"returning: %d\n", status);
	return status;
}


/*
Copyright (c) 2021, Stephen P. Shoecraft
All rights reserved.

This source code is licensed under the BSD-style license found in the
LICENSE file in the root directory of this source tree.
*/

#include "battery.h"

/******************************
*
*** Battery Storage Device
*
******************************/

static void _get_arr(char *name,void *dest, int len,json_value_t *v) {
	solard_battery_t *bp = dest;
	json_array_t *a;
	int i,type;

	type = json_value_get_type(v);
	dprintf(6,"type: %d(%s)\n", type, json_typestr(type));
	if (type != JSON_TYPE_ARRAY) return;
 
	a = json_value_get_array(v);
	if (!a->count) return;
	/* must be numbers */
	if (json_value_get_type(a->items[0]) != JSON_TYPE_NUMBER) return;
	if (strcmp(name,"temps")==0) {
		for(i=0; i < a->count; i++) bp->temps[i] = json_value_get_number(a->items[i]);
		bp->ntemps = a->count;
	} else if (strcmp(name,"cellvolt")==0) {
		for(i=0; i < a->count; i++) bp->cells[i].voltage = json_value_get_number(a->items[i]);
		bp->ncells = a->count;
	} else if (strcmp(name,"cellres")==0) {
		for(i=0; i < a->count; i++) bp->cells[i].resistance = json_value_get_number(a->items[i]);
	}
	return;
}

static void _set_arr(char *name,void *dest, int len,json_value_t *v) {
	solard_battery_t *bp = dest;
	json_array_t *a;
	int i;

	a = json_create_array();
	if (!a) return;
	dprintf(1,"len: %d\n", len);
	if (strcmp(name,"temps")==0) {
		for(i=0; i < len; i++) json_array_add_number(a,bp->temps[i]);
	} else if (strcmp(name,"cellvolt")==0) {
		for(i=0; i < len; i++) json_array_add_number(a,bp->cells[i].voltage);
	} else if (strcmp(name,"cellres")==0) {
		for(i=0; i < len; i++) json_array_add_number(a,bp->cells[i].resistance);
	}
	json_object_set_array(json_value_get_object(v),name,a);
	return;
}

static void _flat_arr(char *name,void *dest, int len,json_value_t *v) {
	solard_battery_t *bp = dest;
	json_object_t *o;
	char label[32];
	int i;

	o = json_value_get_object(v);
	dprintf(1,"len: %d\n", len);
	if (strcmp(name,"temps")==0) {
		for(i=0; i < len; i++) {
			sprintf(label,"temp_%02d",i);
			json_object_set_number(o,label,bp->temps[i]);
		}
	} else if (strcmp(name,"cellvolt")==0) {
		for(i=0; i < len; i++) {
			sprintf(label,"cell_%02d",i);
			json_object_set_number(o,label,bp->cells[i].voltage);
		}
	} else if (strcmp(name,"cellres")==0) {
		for(i=0; i < len; i++) {
			sprintf(label,"res_%02d",i);
			json_object_set_number(o,label,bp->cells[i].resistance);
		}
	}
	return;
}

static struct battery_states {
	int mask;
	char *label;
} states[] = {
	{ BATTERY_STATE_CHARGING, "Charging" },
	{ BATTERY_STATE_BALANCING, "Balancing" },
};
#define NSTATES (sizeof(states)/sizeof(struct battery_states))

static void _get_state(char *name, void *dest, int len, json_value_t *v) {
	solard_battery_t *bp = dest;
	list lp;
	char *p;
	int i;

	p = json_value_get_string(v);
	dprintf(4,"value: %s\n", p);
	conv_type(DATA_TYPE_STRING_LIST,&lp,0,DATA_TYPE_STRING,p,strlen(p));

	bp->state = 0;
	list_reset(lp);
	while((p = list_get_next(lp)) != 0) {
		dprintf(6,"p: %s\n", p);
		for(i=0; i < NSTATES; i++) {
			if (strcmp(p,states[i].label) == 0) {
				bp->state |= states[i].mask;
				break;
			}
		}
	}
	dprintf(4,"state: %x\n", bp->state);
	list_destroy(lp);
}

static void _set_state(char *name, void *dest, int len, json_value_t *v) {
	solard_battery_t *bp = dest;
	char temp[128],*p;
	int i,j;

	dprintf(1,"state: %x\n", bp->state);

	/* States */
	temp[0] = 0;
	p = temp;
	for(i=j=0; i < NSTATES; i++) {
		if (solard_check_state(bp,states[i].mask)) {
			if (j) p += sprintf(p,",");
			p += sprintf(p,states[i].label);
			j++;
		}
	}
	dprintf(1,"temp: %s\n", temp);
	json_object_set_string(json_value_get_object(v), name, temp);
}

#define BATTERY_TAB(NTEMP,NBAT,ACTION,STATE) \
		{ "name",DATA_TYPE_STRING,&bp->name,sizeof(bp->name)-1,0 }, \
		{ "capacity",DATA_TYPE_FLOAT,&bp->capacity,0,0 }, \
		{ "voltage",DATA_TYPE_FLOAT,&bp->voltage,0,0 }, \
		{ "current",DATA_TYPE_FLOAT,&bp->current,0,0 }, \
		{ "ntemps",DATA_TYPE_INT,&bp->ntemps,0,0 }, \
		{ "temps",0,bp,NTEMP,ACTION }, \
		{ "ncells",DATA_TYPE_INT,&bp->ncells,0,0 }, \
		{ "cellvolt",0,bp,NBAT,ACTION }, \
		{ "cellres",0,bp,NBAT,ACTION }, \
		{ "cell_min",DATA_TYPE_FLOAT,&bp->cell_min,0,0 }, \
		{ "cell_max",DATA_TYPE_FLOAT,&bp->cell_max,0,0 }, \
		{ "cell_diff",DATA_TYPE_FLOAT,&bp->cell_diff,0,0 }, \
		{ "cell_avg",DATA_TYPE_FLOAT,&bp->cell_avg,0,0 }, \
		{ "cell_total",DATA_TYPE_FLOAT,&bp->cell_total,0,0 }, \
		{ "errcode",DATA_TYPE_INT,&bp->errcode,0,0 }, \
		{ "errmsg",DATA_TYPE_STRING,&bp->errmsg,sizeof(bp->errmsg)-1,0 }, \
		{ "state",0,bp,0,STATE }, \
		JSON_PROCTAB_END

static void _dump_arr(char *name,void *dest, int flen,json_value_t *v) {
	solard_battery_t *bp = dest;
	char format[16];
	int i,*dlevel = (int *) v;

#ifdef DEBUG
	if (debug >= *dlevel) {
#endif
		dprintf(1,"flen: %d\n", flen);
		if (strcmp(name,"temps")==0) {
			sprintf(format,"%%%ds: %%.1f\n",flen);
			for(i=0; i < bp->ntemps; i++) printf(format,name,bp->temps[i]);
		} else if (strcmp(name,"cellvolt")==0) {
			sprintf(format,"%%%ds: %%.3f\n",flen);
			for(i=0; i < bp->ncells; i++) printf(format,name,bp->cells[i].voltage);
		} else if (strcmp(name,"cellres")==0) {
			sprintf(format,"%%%ds: %%.3f\n",flen);
			for(i=0; i < bp->ncells; i++) printf(format,name,bp->cells[i].resistance);
		}
#ifdef DEBUG
	}
#endif
	return;
}

static void _dump_state(char *name, void *dest, int flen, json_value_t *v) {
	solard_battery_t *bp = dest;
	char format[16];
	int *dlevel = (int *) v;

	sprintf(format,"%%%ds: %%d\n",flen);
#ifdef DEBUG
	if (debug >= *dlevel) printf(format,name,bp->state);
#else
	printf(format,name,bp->state);
#endif
}

void battery_dump(solard_battery_t *bp, int dlevel) {
	json_proctab_t tab[] = { BATTERY_TAB(bp->ntemps,bp->ncells,_dump_arr,_dump_state) }, *p;
	char format[16],temp[1024];
	int flen;

	flen = 0;
	for(p=tab; p->field; p++) {
		if (strlen(p->field) > flen)
			flen = strlen(p->field);
	}
	flen++;
	sprintf(format,"%%%ds: %%s\n",flen);
	dprintf(dlevel,"battery:\n");
	for(p=tab; p->field; p++) {
		if (p->cb) p->cb(p->field,p->ptr,flen,(void *)&dlevel);
		else {
			conv_type(DATA_TYPE_STRING,&temp,sizeof(temp)-1,p->type,p->ptr,p->len);
#ifdef DEBUG
			if (debug >= dlevel) printf(format,p->field,temp);
#else
			printf(format,p->field,temp);
#endif
		}
	}
}

json_value_t *battery_to_json(solard_battery_t *bp) {
	json_proctab_t battery_tab[] = { BATTERY_TAB(bp->ntemps,bp->ncells,_set_arr,_set_state) };

	return json_from_tab(battery_tab);
}

json_value_t *battery_to_flat_json(solard_battery_t *bp) {
	json_proctab_t battery_tab[] = { BATTERY_TAB(bp->ntemps,bp->ncells,_flat_arr,_set_state) };

	return json_from_tab(battery_tab);
}

int battery_from_json(solard_battery_t *bp, char *str) {
	json_proctab_t battery_tab[] = { BATTERY_TAB(BATTERY_MAX_TEMPS,BATTERY_MAX_CELLS,_get_arr,_get_state) };
	json_value_t *v;

	v = json_parse(str);
	memset(bp,0,sizeof(*bp));
	json_to_tab(battery_tab,v);
//	battery_dump(bp,3);
	json_destroy_value(v);
	return 0;
};

#ifdef JS
enum BATTERY_PROPERTY_ID {
	BATTERY_PROPERTY_ID_NONE,
	BATTERY_PROPERTY_ID_NAME,
	BATTERY_PROPERTY_ID_CAPACITY,
	BATTERY_PROPERTY_ID_VOLTAGE,
	BATTERY_PROPERTY_ID_CURRENT,
	BATTERY_PROPERTY_ID_NTEMPS,
	BATTERY_PROPERTY_ID_TEMPS,
	BATTERY_PROPERTY_ID_NCELLS,
	BATTERY_PROPERTY_ID_CELLVOLT,
	BATTERY_PROPERTY_ID_CELLRES,
	BATTERY_PROPERTY_ID_CELL_MIN,
	BATTERY_PROPERTY_ID_CELL_MAX,
	BATTERY_PROPERTY_ID_CELL_DIFF,
	BATTERY_PROPERTY_ID_CELL_AVG,
	BATTERY_PROPERTY_ID_CELL_TOTAL,
	BATTERY_PROPERTY_ID_BALANCEBITS,
	BATTERY_PROPERTY_ID_ERRCODE,
	BATTERY_PROPERTY_ID_ERRMSG,
	BATTERY_PROPERTY_ID_STATE,
	BATTERY_PROPERTY_ID_LAST_UPDATE,
};

static JSBool battery_getprop(JSContext *cx, JSObject *obj, jsval id, jsval *rval) {
	int prop_id;
	solard_battery_t *bp;
	register int i;

	bp = JS_GetPrivate(cx,obj);
	dprintf(4,"bp: %p\n", bp);
	if (!bp) {
		JS_ReportError(cx, "private is null!");
		return JS_FALSE;
	}

	dprintf(4,"is_int: %d\n", JSVAL_IS_INT(id));
	if(JSVAL_IS_INT(id)) {
		prop_id = JSVAL_TO_INT(id);
		dprintf(4,"prop_id: %d\n", prop_id);
		switch(prop_id) {
		case BATTERY_PROPERTY_ID_NAME:
			*rval = STRING_TO_JSVAL(JS_NewStringCopyZ(cx,bp->name));
			*rval = type_to_jsval(cx, DATA_TYPE_STRING, bp->name, strlen(bp->name));
			break;
		case BATTERY_PROPERTY_ID_CAPACITY:
			JS_NewDoubleValue(cx, bp->capacity, rval);
			break;
		case BATTERY_PROPERTY_ID_VOLTAGE:
			JS_NewDoubleValue(cx, bp->voltage, rval);
			break;
		case BATTERY_PROPERTY_ID_CURRENT:
			JS_NewDoubleValue(cx, bp->current, rval);
			break;
		case BATTERY_PROPERTY_ID_NTEMPS:
			dprintf(4,"ntemps: %d\n", bp->ntemps);
			*rval = INT_TO_JSVAL(bp->ntemps);
			break;
		case BATTERY_PROPERTY_ID_TEMPS:
		       {
				JSObject *rows;
				jsval val;

//				dprintf(4,"ntemps: %d\n", bp->ntemps);
				rows = JS_NewArrayObject(cx, bp->ntemps, NULL);
				for(i=0; i < bp->ntemps; i++) {
//					dprintf(4,"temps[%d]: %f\n", i, bp->temps[i]);
					JS_NewDoubleValue(cx, bp->temps[i], &val);
					JS_SetElement(cx, rows, i, &val);
				}
				*rval = OBJECT_TO_JSVAL(rows);
			}
			break;
		case BATTERY_PROPERTY_ID_NCELLS:
			dprintf(4,"ncells: %d\n", bp->ncells);
			*rval = INT_TO_JSVAL(bp->ncells);
			break;
		case BATTERY_PROPERTY_ID_CELLVOLT:
		       {
				JSObject *rows;
				jsval val;

				rows = JS_NewArrayObject(cx, bp->ncells, NULL);
				for(i=0; i < bp->ncells; i++) {
					JS_NewDoubleValue(cx, bp->cells[i].voltage, &val);
					JS_SetElement(cx, rows, i, &val);
				}
				*rval = OBJECT_TO_JSVAL(rows);
			}
			break;
		case BATTERY_PROPERTY_ID_CELLRES:
		       {
				JSObject *rows;
				jsval val;

				rows = JS_NewArrayObject(cx, bp->ncells, NULL);
				for(i=0; i < bp->ncells; i++) {
					JS_NewDoubleValue(cx, bp->cells[i].resistance, &val);
					JS_SetElement(cx, rows, i, &val);
				}
				*rval = OBJECT_TO_JSVAL(rows);
			}
			break;
		case BATTERY_PROPERTY_ID_CELL_MIN:
			JS_NewDoubleValue(cx, bp->cell_min, rval);
			break;
		case BATTERY_PROPERTY_ID_CELL_MAX:
			JS_NewDoubleValue(cx, bp->cell_max, rval);
			break;
		case BATTERY_PROPERTY_ID_CELL_DIFF:
			JS_NewDoubleValue(cx, bp->cell_diff, rval);
			break;
		case BATTERY_PROPERTY_ID_CELL_AVG:
			JS_NewDoubleValue(cx, bp->cell_avg, rval);
			break;
		case BATTERY_PROPERTY_ID_CELL_TOTAL:
			JS_NewDoubleValue(cx, bp->cell_total, rval);
			break;
		case BATTERY_PROPERTY_ID_BALANCEBITS:
			*rval = INT_TO_JSVAL(bp->balancebits);
			break;
		case BATTERY_PROPERTY_ID_ERRCODE:
			*rval = INT_TO_JSVAL(bp->errcode);
			break;
		case BATTERY_PROPERTY_ID_ERRMSG:
			*rval = STRING_TO_JSVAL(JS_NewStringCopyZ(cx,bp->errmsg));
			break;
		case BATTERY_PROPERTY_ID_STATE:
			*rval = INT_TO_JSVAL(bp->state);
			break;
		case BATTERY_PROPERTY_ID_LAST_UPDATE:
			*rval = INT_TO_JSVAL(bp->last_update);
			break;
		default:
			JS_ReportError(cx, "not a property");
			*rval = JSVAL_NULL;
		}
	}
	return JS_TRUE;
}

static JSClass battery_class = {
	"battery",
	JSCLASS_HAS_PRIVATE,
	JS_PropertyStub,
	JS_PropertyStub,
	battery_getprop,
	JS_PropertyStub,
	JS_EnumerateStub,
	JS_ResolveStub,
	JS_ConvertStub,
	JS_FinalizeStub,
	JSCLASS_NO_OPTIONAL_MEMBERS
};

JSObject *JSBattery(JSContext *cx, solard_battery_t *bp) {
	JSPropertySpec battery_props[] = { 
		{ "name",		BATTERY_PROPERTY_ID_NAME,	JSPROP_ENUMERATE | JSPROP_READONLY },
		{ "capacity",		BATTERY_PROPERTY_ID_CAPACITY,	JSPROP_ENUMERATE | JSPROP_READONLY },
		{ "voltage",		BATTERY_PROPERTY_ID_VOLTAGE,	JSPROP_ENUMERATE | JSPROP_READONLY },
		{ "current",		BATTERY_PROPERTY_ID_CURRENT,	JSPROP_ENUMERATE | JSPROP_READONLY },
		{ "ntemps",		BATTERY_PROPERTY_ID_NTEMPS,	JSPROP_ENUMERATE | JSPROP_READONLY },
		{ "temps",		BATTERY_PROPERTY_ID_TEMPS,	JSPROP_ENUMERATE | JSPROP_READONLY },
		{ "ncells",		BATTERY_PROPERTY_ID_NCELLS,	JSPROP_ENUMERATE | JSPROP_READONLY },
		{ "cellvolt",		BATTERY_PROPERTY_ID_CELLVOLT,	JSPROP_ENUMERATE | JSPROP_READONLY },
		{ "cellres",		BATTERY_PROPERTY_ID_CELLRES,	JSPROP_ENUMERATE | JSPROP_READONLY },
		{ "cell_min",		BATTERY_PROPERTY_ID_CELL_MIN,	JSPROP_ENUMERATE | JSPROP_READONLY },
		{ "cell_max",		BATTERY_PROPERTY_ID_CELL_MAX,	JSPROP_ENUMERATE | JSPROP_READONLY },
		{ "cell_diff",		BATTERY_PROPERTY_ID_CELL_DIFF,	JSPROP_ENUMERATE | JSPROP_READONLY },
		{ "cell_avg",		BATTERY_PROPERTY_ID_CELL_AVG,	JSPROP_ENUMERATE | JSPROP_READONLY },
		{ "cell_total",		BATTERY_PROPERTY_ID_CELL_TOTAL,	JSPROP_ENUMERATE | JSPROP_READONLY },
		{ "balancebits",	BATTERY_PROPERTY_ID_BALANCEBITS,	JSPROP_ENUMERATE | JSPROP_READONLY },
		{ "errcode",		BATTERY_PROPERTY_ID_ERRCODE,	JSPROP_ENUMERATE | JSPROP_READONLY },
		{ "errmsg",		BATTERY_PROPERTY_ID_ERRMSG,	JSPROP_ENUMERATE | JSPROP_READONLY },
		{ "state",		BATTERY_PROPERTY_ID_STATE,	JSPROP_ENUMERATE | JSPROP_READONLY },
		{ "last_update",	BATTERY_PROPERTY_ID_LAST_UPDATE,	JSPROP_ENUMERATE | JSPROP_READONLY },
		{0}
	};
	JSFunctionSpec battery_funcs[] = {
		{ 0 }
	};
	JSObject *obj;

	dprintf(5,"defining %s object\n",battery_class.name);
	obj = JS_InitClass(cx, JS_GetGlobalObject(cx), 0, &battery_class, 0, 0, battery_props, battery_funcs, 0, 0);
	if (!obj) {
		JS_ReportError(cx,"unable to initialize battery class");
		return 0;
	}

	/* Pre-create the JSVALs */
	JS_SetPrivate(cx,obj,bp);
	dprintf(5,"done!\n");
	return obj;
}
#endif

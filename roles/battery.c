
/*
Copyright (c) 2021, Stephen P. Shoecraft
All rights reserved.

This source code is licensed under the BSD-style license found in the
LICENSE file in the root directory of this source tree.
*/

#include "battery.h"

struct battery_session {
	char name[SOLARD_NAME_LEN];
	solard_agent_t *ap;
	solard_driver_t *driver;
	void *handle;
	solard_battery_t info;
	int flatten;
};
typedef struct battery_session battery_session_t;

static int battery_get_config(battery_session_t *s) {
	solard_battery_t *bp = &s->info;
	char *section_name = s->ap->section_name;
	struct cfg_proctab packtab[] = {
		{ section_name, "name", 0, DATA_TYPE_STRING,&bp->name,sizeof(bp->name)-1, s->ap->instance_name },
		{ section_name, "capacity", 0, DATA_TYPE_FLOAT,&bp->capacity, 0, 0 },
		{ section_name, "flatten", 0, DATA_TYPE_BOOL, &s->flatten, 0, 0 },
		CFG_PROCTAB_END
	};

	cfg_get_tab(s->ap->cfg,packtab);
#ifdef DEBUG
	if (debug >= 3) cfg_disp_tab(packtab,s->info.name,1);
#endif
	return 0;
}

static void *battery_new(void *confp, void *driver, void *driver_handle) {
	solard_agent_t *conf = confp;
	battery_session_t *s;

	dprintf(5,"confp: %p, driver: %p, driver_handle: %p\n", confp, driver, driver_handle);
	s = calloc(1,sizeof(*s));
	if (!s) {
		log_write(LOG_SYSERR,"battery_new: calloc");
		return 0;
	}
	s->ap = conf;
	s->driver = driver;
	s->handle = driver_handle;

	/* Get our specific config */
	battery_get_config(s);

	/* Update the role_data in agent conf */
	conf->role_data = &s->info;

	/* Save a copy of the name */
	strncpy(s->name,s->info.name,sizeof(s->name)-1);

	return s;
}

static int battery_open(void *handle) {
	return 0;
}

static int battery_close(void *handle) {
	return 0;
}

static void _get_arr(char *name,void *dest, int len,json_value_t *v) {
	solard_battery_t *bp = dest;
	json_array_t *a;
	int i;

	dprintf(6,"type: %d(%s)\n", v->type, json_typestr(v->type));
	if (v->type != JSONArray) return;
 
	a = v->value.array;
	if (!a->count) return;
	/* must be numbers */
	if (a->items[0]->type != JSONNumber) return;
	if (strcmp(name,"temps")==0) {
		for(i=0; i < a->count; i++) bp->temps[i] = a->items[i]->value.number;
		bp->ntemps = a->count;
	} else if (strcmp(name,"cellvolt")==0) {
		for(i=0; i < a->count; i++) bp->cellvolt[i] = a->items[i]->value.number;
		bp->ncells = a->count;
#if BATTERY_CELLRES
	} else if (strcmp(name,"cellres")==0) {
		for(i=0; i < a->count; i++) bp->cellres[i] = a->items[i]->value.number;
#endif
	}
	return;
}

static void _set_arr(char *name,void *dest, int len,json_value_t *v) {
	solard_battery_t *bp = dest;
	json_value_t *a;
	int i;

	a = json_create_array();
	if (!a) return;
	dprintf(1,"len: %d\n", len);
	if (strcmp(name,"temps")==0) {
		for(i=0; i < len; i++) json_array_add_number(a,bp->temps[i]);
	} else if (strcmp(name,"cellvolt")==0) {
		for(i=0; i < len; i++) json_array_add_number(a,bp->cellvolt[i]);
#if BATTERY_CELLRES
	} else if (strcmp(name,"cellres")==0) {
		for(i=0; i < len; i++) json_array_add_number(a,bp->cellres[i]);
#endif
	}
	json_add_value(v,name,a);
	return;
}

static void _flat_arr(char *name,void *dest, int len,json_value_t *v) {
	solard_battery_t *bp = dest;
	char label[32];
	int i;

	dprintf(1,"len: %d\n", len);
	if (strcmp(name,"temps")==0) {
		for(i=0; i < len; i++) {
			sprintf(label,"temp_%02d",i);
			json_add_number(v,label,bp->temps[i]);
		}
	} else if (strcmp(name,"cellvolt")==0) {
		for(i=0; i < len; i++) {
			sprintf(label,"cell_%02d",i);
			json_add_number(v,label,bp->cellvolt[i]);
		}
#if BATTERY_CELLRES
	} else if (strcmp(name,"cellres")==0) {
		for(i=0; i < len; i++) {
			sprintf(label,"res_%02d",i);
			json_add_number(v,label,bp->cellres[i]);
		}
#endif
	}
	return;
}

#ifdef INFLUXDB
static void _ic_arr(char *name,void *dest, int len,json_value_t *v) {
	solard_battery_t *bp = dest;
	char label[32];
	int i;

	dprintf(1,"len: %d\n", len);
	if (strcmp(name,"temps")==0) {
		for(i=0; i < len; i++) {
			sprintf(label,"temp_%02d",i);
			ic_double(label,bp->temps[i]);
		}
	} else if (strcmp(name,"cellvolt")==0) {
		for(i=0; i < len; i++) {
			sprintf(label,"cell_%02d",i);
			ic_double(label,bp->cellvolt[i]);
		}
#if BATTERY_CELLRES
	} else if (strcmp(name,"cellres")==0) {
		for(i=0; i < len; i++) {
			sprintf(label,"res_%02d",i);
			ic_double(label,bp->cellres[i]);
		}
#endif
	}

	return;
}
#endif

static struct battery_states {
	int mask;
	char *label;
} states[] = {
	{ BATTERY_STATE_CHARGING, "Charging" },
	{ BATTERY_STATE_DISCHARGING, "Discharging" },
	{ BATTERY_STATE_BALANCING, "Balancing" },
};
#define NSTATES (sizeof(states)/sizeof(struct battery_states))

static void _get_state(char *name, void *dest, int len, json_value_t *v) {
	solard_battery_t *bp = dest;
	list lp;
	char *p;
	int i;

	dprintf(4,"value: %s\n", v->value.string.chars);
	conv_type(DATA_TYPE_LIST,&lp,0,DATA_TYPE_STRING,v->value.string.chars,v->value.string.length);

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
	json_add_string(v, name, temp);
}

#if BATTERY_CELLRES
		{ "cellres",0,bp,NBAT,ACTION }, 
#endif
#define BATTERY_TAB(NTEMP,NBAT,ACTION,STATE) \
		{ "name",DATA_TYPE_STRING,&bp->name,sizeof(bp->name)-1,0 }, \
		{ "capacity",DATA_TYPE_FLOAT,&bp->capacity,0,0 }, \
		{ "voltage",DATA_TYPE_FLOAT,&bp->voltage,0,0 }, \
		{ "current",DATA_TYPE_FLOAT,&bp->current,0,0 }, \
		{ "ntemps",DATA_TYPE_INT,&bp->ntemps,0,0 }, \
		{ "temps",0,bp,NTEMP,ACTION }, \
		{ "ncells",DATA_TYPE_INT,&bp->ncells,0,0 }, \
		{ "cellvolt",0,bp,NBAT,ACTION }, \
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
			for(i=0; i < bp->ncells; i++) printf(format,name,bp->cellvolt[i]);
#if BATTERY_CELLRES
		} else if (strcmp(name,"cellres")==0) {
			sprintf(format,"%%%ds: %%.3f\n",flen);
			for(i=0; i < bp->ncells; i++) printf(format,name,bp->cellres[i]);
#endif
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

#ifdef INFLUXDB
int battery_to_influxdb(battery_session_t *s) {
	solard_battery_t *bp = &s->info;
	json_proctab_t battery_tab[] = { BATTERY_TAB(bp->ntemps,bp->ncells,_ic_arr,_set_state) }, *p;
        int *ip;
        float *fp;
        double *dp;

	ic_measure("Battery");
        for(p=battery_tab; p->field; p++) {
                if (p->cb) p->cb(p->field,p->ptr,p->len,0);
                else {
                        switch(p->type) {
                        case DATA_TYPE_STRING:
				ic_string(p->field,p->ptr);
                                break;
                        case DATA_TYPE_INT:
                                ip = p->ptr;
				ic_long(p->field,*ip);
                                break;
                        case DATA_TYPE_FLOAT:
                                fp = p->ptr;
				ic_double(p->field,*fp);
                                break;
                        case DATA_TYPE_DOUBLE:
                                dp = p->ptr;
				ic_double(p->field,*dp);
                                break;
                        default:
                                dprintf(1,"battery_to_influxdb: unhandled type: %d\n", p->type);
                                break;
                        }
                }
	}
	ic_measureend();
	ic_push();
	return 0;
}
#endif

int battery_from_json(solard_battery_t *bp, char *str) {
	json_proctab_t battery_tab[] = { BATTERY_TAB(BATTERY_MAX_TEMPS,BATTERY_MAX_CELLS,_get_arr,_get_state) };
	json_value_t *v;

	v = json_parse(str);
	memset(bp,0,sizeof(*bp));
	json_to_tab(battery_tab,v);
//	battery_dump(bp,3);
	json_destroy(v);
	return 0;
};

int battery_send_mqtt(battery_session_t *s) {
	solard_battery_t *pp = &s->info;
	char temp[256],*p;
	json_value_t *v;

//	strcpy(pp->id,s->id);
//	strcpy(pp->name,s->name);
	if (s->flatten) v = battery_to_flat_json(pp);
	else v = battery_to_json(pp);
	if (!v) return 1;
	p = json_dumps(v,0);
	sprintf(temp,"%s/%s/%s/%s",SOLARD_TOPIC_ROOT,SOLARD_ROLE_BATTERY,s->name,SOLARD_FUNC_DATA);
	dprintf(2,"sending mqtt data...\n");
	if (mqtt_pub(s->ap->m, temp, p, 1, 0)) {
		log_write(LOG_ERROR,"unable to publish to mqtt, reconnecting");
		mqtt_disconnect(s->ap->m,5);
		mqtt_connect(s->ap->m,20);
	}
	free(p);
	json_destroy(v);
	return 0;
}

static int battery_read(void *handle,void *buf,int buflen) {
	battery_session_t *s = handle;
	solard_battery_t *pp = &s->info;
	int r;

	dprintf(4,"%s: opening...\n", s->name);
	if (s->driver->open(s->handle)) {
		dprintf(1,"%s: open error\n",s->name);
		return 1;
	}
	dprintf(4,"%s: reading...\n", s->name);
	r = (s->driver->read ? s->driver->read(s->handle,pp,sizeof(*pp)) : 1);
	dprintf(4,"%s: closing\n", s->name);
	s->driver->close(s->handle);
	if (!r) {
		float total;
		int i;

//		solard_set_state(s,SOLARD_PACK_STATE_UPDATED);
		/* Set min/max/diff/avg */
		pp->cell_max = 0.0;
//		pp->cell_min = s->conf->cell_crit_high;
		pp->cell_min = 9999999.99;
		total = 0.0;
		for(i=0; i < pp->ncells; i++) {
			if (pp->cellvolt[i] < pp->cell_min)
				pp->cell_min = pp->cellvolt[i];
			if (pp->cellvolt[i] > pp->cell_max)
				pp->cell_max = pp->cellvolt[i];
			total += pp->cellvolt[i];
		}
		pp->cell_diff = pp->cell_max - pp->cell_min;
		pp->cell_avg = total / pp->ncells;
		if (!(int)pp->voltage) pp->voltage = total;
		pp->cell_total = total;
		battery_send_mqtt(s);
//		battery_to_influxdb(s);
	}
	dprintf(4,"%s: returning: %d\n", s->name, r);
	return r;
}

static int battery_write(void *handle, void *buf, int buflen) {
	battery_session_t *s = handle;
	int r;

	dprintf(4,"%s: opening...\n", s->name);
	if (s->driver->open(s->handle)) {
		dprintf(1,"%s: open error\n",s->name);
		return 1;
	}
	dprintf(4,"%s: writing...\n", s->name);
	r = (s->driver->write ? s->driver->write(s->handle,&s->info,sizeof(s->info)) : 1);
	dprintf(4,"%s: closing\n", s->name);
	s->driver->close(s->handle);
	dprintf(4,"%s: returning: %d\n", s->name, r);
	return 1;
}

#if 0
static json_value_t *battery_info(void *handle) {
	battery_session_t *s = handle;
	json_value_t *info;

	dprintf(4,"%s: opening...\n", s->name);
	while(1) {
		if (!s->driver->open(s->handle)) {
			break;
		} else {
			dprintf(1,"%s: open error\n",s->name);
		}
		sleep(10);
	}
	dprintf(4,"%s: getting info...\n", s->name);
	info = (s->driver->info ? s->driver->info(s->handle) : json_parse("{}"));
	dprintf(4,"%s: closing\n", s->name);
	s->driver->close(s->handle);
	return info;
}
#endif

static int battery_config(void *h, int req, ...) {
	battery_session_t *s = h;
	va_list ap;
	void **vp;
	int r;

	r = 1;
	va_start(ap,req);
	dprintf(1,"req: %d\n", req);
	switch(req) {
	case SOLARD_CONFIG_GET_DRIVER:
		vp = va_arg(ap,void **);
		if (vp) {
			*vp = s->driver;
			r = 0;
		}
		break;
	case SOLARD_CONFIG_GET_HANDLE:
		vp = va_arg(ap,void **);
		if (vp) {
			*vp = s->handle;
			r = 0;
		}
		break;
	default:
		if (s) r = s->driver->config(s->handle,req,va_arg(ap,void *));
	}
	dprintf(1,"returning: %d\n", r);
	return r;
}

solard_driver_t battery_driver = {
	SOLARD_DRIVER_ROLE,
	"Battery",
	battery_new,
	battery_open,
	battery_close,
	battery_read,
	battery_write,
	battery_config,
};


#include "battery.h"

struct battery_session {
	char name[BATTERY_NAME_LEN+1];
	solard_agent_t *ap;
	module_t *driver;
	void *handle;
	solard_battery_t info;
};
typedef struct battery_session battery_session_t;

void battery_dump(solard_battery_t *bp, int dlevel) {
	int i;

	dprintf(dlevel,"id: %s\n", bp->id);
	dprintf(dlevel,"name: %s\n", bp->name);
	dprintf(dlevel,"capacity: %.1f\n", bp->capacity);
	dprintf(dlevel,"voltage %.1f\n", bp->voltage);
	dprintf(dlevel,"current: %.1f\n", bp->current);
	dprintf(dlevel,"ntemps: %d\n", bp->ntemps);
	for(i=0; i < bp->ntemps; i++) dprintf(dlevel,"temp[%d]: %.1f\n", i, bp->temps[i]);
	dprintf(dlevel,"ncells: %d\n", bp->ncells);
	for(i=0; i < bp->ncells; i++) dprintf(dlevel,"cellvolt[%d]: %.3f\n", i, bp->cellvolt[i]);
	for(i=0; i < bp->ncells; i++) dprintf(dlevel,"cellres[%d]: %.3f\n", i, bp->cellres[i]);
	dprintf(dlevel,"cell_min: %.3f\n", bp->cell_min);
	dprintf(dlevel,"cell_max: %.3f\n", bp->cell_max);
	dprintf(dlevel,"cell_diff: %.3f\n", bp->cell_diff);
	dprintf(dlevel,"cell_avg: %.3f\n", bp->cell_avg);
	dprintf(dlevel,"cell_total: %.3f\n", bp->cell_total);
	dprintf(dlevel,"errcode: %d\n", bp->errcode);
	dprintf(dlevel,"errmsg: %s\n", bp->errmsg);
	dprintf(dlevel,"state: %04x\n", bp->state);
}

int battery_get_config(battery_session_t *s, char *section_name) {
	struct cfg_proctab packtab[] = {
		{ section_name, "capacity", "Pack Capacity in AH", DATA_TYPE_FLOAT,&s->info.capacity, 0, 0 },
		CFG_PROCTAB_END
	};

	cfg_get_tab(s->ap->cfg,packtab);
	if (debug >= 3) cfg_disp_tab(packtab,s->name,1);

	return 0;
}

int battery_init(void *conf, char *section_name) {
	return 0;
}

void *battery_new(void *handle, void *driver, void *driver_handle) {
	solard_agent_t *conf = handle;
	battery_session_t *s;

	s = calloc(1,sizeof(*s));
	if (!s) {
		log_write(LOG_SYSERR,"battery_new: calloc");
		return 0;
	}
	s->ap = conf;
	s->driver = driver;
	s->handle = driver_handle;

	/* Save a copy of the name */
	strncat(s->name,s->ap->name,sizeof(s->name)-1);

	/* Get battery-specific config */
	battery_get_config(s,conf->section_name);

	return s;
}

int battery_open(void *handle) {
	return 0;
}

int battery_close(void *handle) {
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
	} else if (strcmp(name,"cellres")==0) {
		for(i=0; i < a->count; i++) bp->cellres[i] = a->items[i]->value.number;
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
	} else if (strcmp(name,"cellres")==0) {
		for(i=0; i < len; i++) json_array_add_number(a,bp->cellres[i]);
	}
	json_add_value(v,name,a);
	return;
}

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

#define BATTERY_TAB(NTEMP,NBAT,ACTION,STATE) \
		{ "id",&bp->id,DATA_TYPE_STRING,sizeof(bp->id)-1,0 }, \
		{ "name",&bp->name,DATA_TYPE_STRING,sizeof(bp->name)-1,0 }, \
		{ "capacity",&bp->capacity,DATA_TYPE_FLOAT,0,0 }, \
		{ "voltage",&bp->voltage,DATA_TYPE_FLOAT,0,0 }, \
		{ "current",&bp->current,DATA_TYPE_FLOAT,0,0 }, \
		{ "ntemps",&bp->ntemps,DATA_TYPE_INT,0,0 }, \
		{ "temps",bp,0,NTEMP,ACTION }, \
		{ "ncells",&bp->ncells,DATA_TYPE_INT,0,0 }, \
		{ "cellvolt",bp,0,NBAT,ACTION }, \
		{ "cellres",bp,0,NBAT,ACTION }, \
		{ "cell_min",&bp->cell_min,DATA_TYPE_FLOAT,0,0 }, \
		{ "cell_max",&bp->cell_max,DATA_TYPE_FLOAT,0,0 }, \
		{ "cell_diff",&bp->cell_diff,DATA_TYPE_FLOAT,0,0 }, \
		{ "cell_avg",&bp->cell_avg,DATA_TYPE_FLOAT,0,0 }, \
		{ "cell_total",&bp->cell_total,DATA_TYPE_FLOAT,0,0 }, \
		{ "errcode",&bp->errcode,DATA_TYPE_INT,0,0 }, \
		{ "errmsg",&bp->errmsg,DATA_TYPE_STRING,sizeof(bp->errmsg)-1,0 }, \
		{ "state",bp,0,0,STATE }, \
		JSON_PROCTAB_END

json_value_t *battery_to_json(solard_battery_t *bp) {
	json_proctab_t battery_tab[] = { BATTERY_TAB(bp->ntemps,bp->ncells,_set_arr,_set_state) };

	return json_from_tab(battery_tab);
}

int battery_from_json(solard_battery_t *bp, char *str) {
	json_proctab_t battery_tab[] = { BATTERY_TAB(BATTERY_MAX_TEMPS,BATTERY_MAX_CELLS,_get_arr,_get_state) };
	json_value_t *v;

	v = json_parse(str);
	memset(bp,0,sizeof(*bp));
	json_to_tab(battery_tab,v);
	battery_dump(bp,3);
	json_destroy(v);
	return 0;
};

int battery_send_mqtt(battery_session_t *s) {
	solard_battery_t *pp = &s->info;
	char temp[256],*p;
	json_value_t *v;

//	strcpy(pp->id,s->id);
	strcpy(pp->name,s->name);
	v = battery_to_json(pp);
	if (!v) return 1;
	p = json_dumps(v,0);
	sprintf(temp,"/SolarD/Battery/%s/Data",s->name);
	dprintf(2,"sending mqtt data...\n");
	mqtt_pub(s->ap->m, temp, p, 0);
	free(p);
	json_destroy(v);
	return 0;
}

char *battery_info(void *handle) {
	battery_session_t *s = handle;
	char *info;

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
	info = (s->driver->info ? s->driver->info(s->handle) : "{}");
	dprintf(4,"%s: closing\n", s->name);
	s->driver->close(s->handle);
	return info;
}

int battery_read(void *handle,void *buf,int buflen) {
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
	}
	dprintf(4,"%s: returning: %d\n", s->name, r);
	return r;
}

int battery_write(void *handle, void *buf, int buflen) {
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

int battery_control(void *handle, char *action, char *id, json_value_t *actions) {
	battery_session_t *s = handle;
	int r;

	dprintf(1,"actions->type: %d\n", actions->type);

	dprintf(1,"s->driver->control: %p\n", s->driver->control);
	if (!s->driver->control) return 1;

	dprintf(4,"%s: opening...\n", s->name);
	if (s->driver->open(s->handle)) {
		dprintf(1,"%s: open error\n",s->name);
		return 0;
	}
	r = s->driver->control(s->handle,action,id,actions);
	dprintf(4,"%s: closing\n", s->name);
	s->driver->close(s->handle);
	return r;
}

//solard_agent_t *ap, char *action, list lp) {
int battery_config(void *handle, char *action, char *caller, list lp) {
	battery_session_t *s = handle;
	int r;

	dprintf(1,"action: %s, caller: %s\n", action, caller);

	dprintf(1,"s->driver->config: %p\n", s->driver->config);
	if (!s->driver->config) return 1;

	dprintf(4,"%s: opening...\n", s->name);
	if (s->driver->open(s->handle)) {
		dprintf(1,"%s: open error\n",s->name);
		return 0;
	}
	r = s->driver->config(s->handle,action,caller,lp);
	dprintf(4,"%s: closing\n", s->name);
	s->driver->close(s->handle);
	return r;
}

int battery_free(void *handle) {
	return 0;
}

int battery_shutdown(void *handle) {
	return 0;
}

void *battery_get_handle(battery_session_t *s) {
	return s->handle;
}

void *battery_get_info(battery_session_t *s) {
	return &s->info;
}

solard_module_t battery = {
	SOLARD_MODTYPE_BATTERY,
	"Battery",
	battery_init,
	battery_new,
	battery_info,
	battery_open,
	battery_read,
	battery_write,
	battery_close,
	battery_free,
	battery_shutdown,
	battery_control,
	battery_config,
	(solard_module_get_handle_t)battery_get_handle,
};

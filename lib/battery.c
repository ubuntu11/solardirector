
#include "battery.h"

struct battery_session {
	char name[BATTERY_NAME_LEN+1];
	solard_agent_t *ap;
	module_t *driver;
	void *handle;
	solard_battery_t info;
};
typedef struct battery_session battery_session_t;

#if 0
solard_battery_t *battery_init(int argc,char **argv,opt_proctab_t *opts, module_t *mp) {
	return 0;
}
#endif

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

//void *battery_new(void *handle, ...) {
void *battery_new(void *handle, void *driver, void *driver_handle) {
	solard_agent_t *conf = handle;
	battery_session_t *s;
//	va_list va;
//	char *sname;

	s = calloc(1,sizeof(*s));
	if (!s) {
		log_write(LOG_SYSERR,"battery_new: calloc");
		return 0;
	}
	s->ap = conf;
	s->driver = driver;
	s->handle = driver_handle;

#if 0
	va_start(va,handle);
	s->driver = va_arg(va,solard_module_t *);
	s->handle = va_arg(va,void *);
	sname = va_arg(va,char *);
	va_end(va);
#endif

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
int battery_send_mqtt(battery_session_t *s) {
	solard_battery_t *pp = &s->info;
	register int i,j;
	char temp[256],*p;
	unsigned long mask;
	json_object_t *o;
	struct battery_states {
		int mask;
		char *label;
	} states[] = {
		{ BATTERY_STATE_CHARGING, "Charging" },
		{ BATTERY_STATE_DISCHARGING, "Discharging" },
		{ BATTERY_STATE_BALANCING, "Balancing" },
	};
#define NSTATES (sizeof(states)/sizeof(struct battery_states))

	/* Create JSON data */
	o = json_create_object();
//	if (get_timestamp(temp,sizeof(temp),1) == 0) json_add_string(o, "timestamp", temp);
	json_add_string(o, "name", s->name);
	if (strlen(s->ap->uuid)) json_add_string(o, "uuid", s->ap->uuid);
	json_add_number(o, "state", pp->state);
	json_add_number(o, "errcode", pp->errcode);
	json_add_string(o, "errmsg", pp->errmsg);
	json_add_number(o, "capacity", pp->capacity);
	json_add_number(o, "voltage", pp->voltage);
	json_add_number(o, "current", pp->current);
	if (pp->ntemps) {
		json_value_t *a;

		a = json_create_array();
		dprintf(4,"ntemps: %d\n", pp->ntemps);
		for(i=0; i < pp->ntemps; i++) json_array_add_number(a,pp->temps[i]);
		json_add_array(o,"temps",a);
	}
	if (pp->ncells) {
		json_value_t *a;

		a = json_create_array();
		dprintf(4,"cells: %d\n", pp->ncells);
		for(i=0; i < pp->ncells; i++) json_array_add_number(a,pp->cellvolt[i]);
		json_add_array(o,"cellvolt",a);
	}
	json_add_number(o, "cell_min", pp->cell_min);
	json_add_number(o, "cell_max", pp->cell_max);
	json_add_number(o, "cell_diff", pp->cell_diff);
	json_add_number(o, "cell_avg", pp->cell_avg);
	json_add_number(o, "cell_total", pp->cell_total);

	/* States */
	temp[0] = 0;
	p = temp;
	dprintf(1,"state: %x\n", pp->state);
	for(i=j=0; i < NSTATES; i++) {
		if (solard_check_state(pp,states[i].mask)) {
			if (j) p += sprintf(p,",");
			p += sprintf(p,states[i].label);
			j++;
		}
	}
	dprintf(2,"temp: %s\n",temp);
	json_add_string(o, "states", temp);

	mask = 1;
	for(i=0; i < pp->ncells; i++) {
		temp[i] = ((pp->balancebits & mask) != 0 ? '1' : '0');
		mask <<= 1;
	}
	temp[i] = 0;
	json_add_string(o, "balancebits", temp);

	/* TODO: protection info ... already covered with 'error/errmsg?' */
#if 0
        struct {
                unsigned sover: 1;              /* Single overvoltage protection */
                unsigned sunder: 1;             /* Single undervoltage protection */
                unsigned gover: 1;              /* Whole group overvoltage protection */
                unsigned gunder: 1;             /* Whole group undervoltage protection */
                unsigned chitemp: 1;            /* Charge over temperature protection */
                unsigned clowtemp: 1;           /* Charge low temperature protection */
                unsigned dhitemp: 1;            /* Discharge over temperature protection */
                unsigned dlowtemp: 1;           /* Discharge low temperature protection */
                unsigned cover: 1;              /* Charge overcurrent protection */
                unsigned cunder: 1;             /* Discharge overcurrent protection */
                unsigned shorted: 1;            /* Short circuit protection */
                unsigned ic: 1;                 /* Front detection IC error */
                unsigned mos: 1;                /* Software lock MOS */
        } protect;
#endif
	p = json_dumps(o,0);
	sprintf(temp,"/SolarD/Battery/%s/Data",s->name);
	dprintf(2,"sending mqtt data...\n");
#if 1
	if (mqtt_pub(s->ap->m, temp, p, 0)) {
		dprintf(1,"mqtt send error!\n");
		return 1;
	}
#else
	mqtt_dosend(s->ap->m, temp, p);
#endif
	free(p);
	json_destroy(o);
	return 0;
}

char *battery_info(void *handle) {
	battery_session_t *s = handle;
	char *info;

	dprintf(4,"%s: opening...\n", s->name);
	if (s->driver->open(s->handle)) {
		dprintf(1,"%s: open error\n",s->name);
		return 0;
	}
	dprintf(4,"%s: getting info...\n", s->name);
	info = (s->driver->info ? s->driver->info(s->handle) : "{}");
	dprintf(4,"%s: closing\n", s->name);
	s->driver->close(s->handle);
	return info;
}

int battery_read(void *handle,...) {
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
		battery_send_mqtt(s);
	}
	dprintf(4,"%s: returning: %d\n", s->name, r);
	return r;
}

int battery_write(void *handle, ...) {
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

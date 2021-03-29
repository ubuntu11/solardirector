
#include "agent.h"

struct inverter_session {
	char name[INVERTER_NAME_LEN+1];
	solard_agent_t *ap;
	module_t *driver;
	void *handle;
	uint16_t state;
	solard_inverter_t info;
};
typedef struct inverter_session inverter_session_t;

static int inverter_init(void *conf, char *section_name) {
	return 0;
}

static int inverter_get_config(inverter_session_t *s, char *section_name) {
	struct cfg_proctab tab[] = {
		CFG_PROCTAB_END
	};

	cfg_get_tab(s->ap->cfg,tab);
	if (debug >= 3) cfg_disp_tab(tab,s->name,1);

	return 0;
}

//static void *inverter_new(solard_agent_t *conf, ...) {
static void *inverter_new(solard_agent_t *conf, void *driver, void *driver_handle) {
	inverter_session_t *s;
//	va_list va;
//	char *sname;

	s = calloc(1,sizeof(*s));
	if (!s) {
		log_write(LOG_SYSERR,"inverter_new: calloc");
		return 0;
	}
	s->ap = conf;
	s->driver = driver;
	s->handle = driver_handle;

#if 0
	va_start(va,conf);
	s->driver = va_arg(va,solard_module_t *);
	s->handle = va_arg(va,void *);
	sname = va_arg(va,char *);
	va_end(va);
#endif

	/* Save a copy of the name */
	strncat(s->name,s->ap->name,sizeof(s->name)-1);

	/* Get battery-specific config */
	inverter_get_config(s,conf->section_name);

	return s;
}

static int inverter_open(inverter_session_t *s) {
	if (s->driver->open && !solard_check_state(s,SOLARD_INVERTER_STATE_OPEN)) {
		dprintf(4,"%s: opening...\n", s->name);
		if (s->driver->open(s->handle)) {
			dprintf(1,"%s: open error\n",s->name);
			return 1;
		}
		solard_set_state(s,SOLARD_INVERTER_STATE_OPEN);
	}
	return 0;
}

static int inverter_close(inverter_session_t *s) {
	if (s->driver->close && solard_check_state(s,SOLARD_INVERTER_STATE_OPEN)) {
		dprintf(4,"%s: closing...\n", s->name);
		s->driver->close(s->handle);
		solard_clear_state(s,SOLARD_INVERTER_STATE_OPEN);
	}
	return 0;
}

static char *inverter_info(void *handle) {
	inverter_session_t *s = handle;
	char *info;

	if (inverter_open(s)) return 0;
	dprintf(4,"%s: getting info...\n", s->name);
	info = (s->driver->info ? s->driver->info(s->handle) : "{}");
	inverter_close(s);
	return info;
}

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

static void _get_state(char *name, void *dest, int len, json_value_t *v) {
	dprintf(1,"value: %s\n", v->value.string.chars);

#if 0
	solard_inverter_t *inv = dest;
	char temp[128],*p;
	int i,j;

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
	json_add_string(v, "states", temp);
#endif
}

static void _set_state(char *name, void *dest, int len, json_value_t *v) {
	solard_inverter_t *inv = dest;
	char temp[128],*p;
	int i,j;

	dprintf(1,"state: %x\n", inv->state);

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
	dprintf(1,"temp: %s\n", temp);
	json_add_string(v, "states", temp);
}

#define INVERTER_TAB(ACTION) \
		{ "id",&inv->id,DATA_TYPE_STRING,sizeof(inv->id)-1,0 }, \
		{ "name",&inv->name,DATA_TYPE_STRING,sizeof(inv->name)-1,0 }, \
		{ "charge_voltage",&inv->charge_voltage,DATA_TYPE_FLOAT,0,0 }, \
		{ "discharge_voltage",&inv->discharge_voltage,DATA_TYPE_FLOAT,0,0 }, \
		{ "charge_amps",&inv->charge_amps,DATA_TYPE_FLOAT,0,0 }, \
		{ "discharge_amps",&inv->discharge_amps,DATA_TYPE_FLOAT,0,0 }, \
		{ "battery_voltage",&inv->battery_voltage,DATA_TYPE_FLOAT,0,0 }, \
		{ "battery_current",&inv->battery_current,DATA_TYPE_FLOAT,0,0 }, \
		{ "battery_temp",&inv->battery_temp,DATA_TYPE_FLOAT,0,0 }, \
		{ "battery_power",&inv->battery_power,DATA_TYPE_FLOAT,0,0 }, \
		{ "battery_capacity",&inv->battery_capacity,DATA_TYPE_FLOAT,0,0 }, \
		{ "grid_power",&inv->grid_power,DATA_TYPE_FLOAT,0,0 }, \
		{ "load_power",&inv->load_power,DATA_TYPE_FLOAT,0,0 }, \
		{ "site_power",&inv->site_power,DATA_TYPE_FLOAT,0,0 }, \
		{ "soc",&inv->soc,DATA_TYPE_FLOAT,0,0 }, \
		{ "soh",&inv->soh,DATA_TYPE_FLOAT,0,0 }, \
		{ "errcode",&inv->errcode,DATA_TYPE_INT,0,0 }, \
		{ "errmsg",&inv->errmsg,DATA_TYPE_STRING,sizeof(inv->errmsg)-1,0 }, \
		{ "state",inv,0,0,ACTION }, \
		JSON_PROCTAB_END

//		{ "state",&inv->state,DATA_TYPE_INT,0,0 },

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

static int inverter_mqtt_send(inverter_session_t *s) {
	solard_inverter_t *inv = &s->info;
	char temp[256],*p;
	json_value_t *v;
	int r;

//	strcpy(inv->uuid,s->uuid);
	strcpy(inv->name,s->name);
	v = inverter_to_json(&s->info);
	if (!v) return 1;
	p = json_dumps(v,0);
	dprintf(1,"p: %s\n",p);
	sprintf(temp,"/SolarD/%s/%s/Data",AGENT_ROLE_INVERTER,s->name);
	r = mqtt_send(s->ap->m, temp, p, 15);
	if (r) dprintf(1,"mqtt send error!\n");
	free(p);
	json_destroy(v);
	return 0;
}

static int inverter_read(inverter_session_t *s) {
	int r;

	if (!s->driver->read) return 1;

	if (s->driver->open) {
		dprintf(5,"%s: opening...\n", s->name);
		if (s->driver->open(s->handle)) {
			dprintf(1,"%s: open error\n",s->name);
			return 1;
		}
	}
	dprintf(5,"%s: reading...\n", s->name);
	r = s->driver->read(s->handle,&s->info,sizeof(s->info));
	if (s->driver->close) {
		dprintf(5,"%s: closing\n", s->name);
		s->driver->close(s->handle);
	}
	dprintf(5,"%s: returning: %d\n", s->name, r);
	inverter_mqtt_send(s);
	return r;
}

static int inverter_write(void *handle) {
	inverter_session_t *s = handle;

	dprintf(1,"driver->write: %p\n", s->driver->write);
	if (!s->driver->write) return 1;

	if (s->driver->open && s->driver->open(s->handle)) return 1;
	if (s->driver->write(s->handle,&s->info,sizeof(s->info)) < 0) {
		dprintf(1,"error writing to %s\n", s->name);
		return 1;
	}
	if (s->driver->close) s->driver->close(s->handle);
	return 0;
}

/* Return driver handle */
static void *inverter_get_handle(inverter_session_t *s) {
	return s->handle;
}

/* Return driver handle */
static void *inverter_get_info(inverter_session_t *s) {
	return &s->info;
}

solard_module_t inverter = {
	SOLARD_MODTYPE_BATTERY,
	"Battery",
	inverter_init,			/* Init */
	(solard_module_new_t)inverter_new,			/* New */
	inverter_info,			/* Info */
	(solard_module_open_t)inverter_open,			/* Open */
	(solard_module_read_t)inverter_read,			/* Read */
	(solard_module_write_t)inverter_write,			/* Write */
	(solard_module_close_t)inverter_close,			/* Close */
	0,				/* Free */
	0,				/* Shutdown */
	(solard_module_control_t)0,		/* Control */
	(solard_module_config_t)0,			/* Config */
	(solard_module_get_handle_t)inverter_get_handle,
	(solard_module_get_handle_t)inverter_get_info,
};

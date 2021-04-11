
#include "agent.h"

struct inverter_session {
	char id[INVERTER_ID_LEN];
	char name[INVERTER_NAME_LEN];
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
	solard_inverter_t *inv = &s->info;
	struct cfg_proctab tab[] = {
		{ section_name, "charge_voltage", 0, DATA_TYPE_FLOAT, &inv->charge_voltage, 0, 0 },
		{ section_name, "charge_max_voltage", 0, DATA_TYPE_FLOAT, &inv->charge_max_voltage, 0, 0 },
		{ section_name, "charge_at_max", 0, DATA_TYPE_BOOL, &inv->charge_at_max, 0, 0 },
		{ section_name, "charge_amps", 0, DATA_TYPE_FLOAT, &inv->charge_amps, 0, 0 },
		{ section_name, "discharge_voltage", 0, DATA_TYPE_FLOAT, &inv->discharge_voltage, 0, 0 },
		{ section_name, "discharge_amps", 0, DATA_TYPE_FLOAT, &inv->discharge_amps, 0, 0 },
		{ section_name, "soc", 0, DATA_TYPE_FLOAT, &inv->soc, 0, "-1" },
		CFG_PROCTAB_END
	};

	cfg_get_tab(s->ap->cfg,tab);
	if (debug >= 3) cfg_disp_tab(tab,s->name,1);

	return 0;
}

static void *inverter_new(void *handle, void *driver, void *driver_handle) {
	solard_agent_t *conf = handle;
	inverter_session_t *s;

	s = calloc(1,sizeof(*s));
	if (!s) {
		log_write(LOG_SYSERR,"inverter_new: calloc");
		return 0;
	}
	s->ap = conf;
	s->driver = driver;
	s->handle = driver_handle;

	/* Update role_data in agent conf */
	conf->role_data = &s->info;

	/* Save a copy of the name */
	strncat(s->name,s->ap->name,sizeof(s->name)-1);

	/* Get specific config */
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

static json_value_t *inverter_info(void *handle) {
	inverter_session_t *s = handle;
	json_value_t *info;

	if (inverter_open(s)) return 0;
	dprintf(4,"%s: getting info...\n", s->name);
	info = (s->driver->info ? s->driver->info(s->handle) : json_parse("{}"));
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
		{ "id",DATA_TYPE_STRING,&inv->id,sizeof(inv->id)-1,0 }, \
		{ "name",DATA_TYPE_STRING,&inv->name,sizeof(inv->name)-1,0 }, \
		{ "type",DATA_TYPE_INT,&inv->type,0,0 }, \
		{ "charge_voltage",DATA_TYPE_FLOAT,&inv->charge_voltage,0,0 }, \
		{ "charge_max_voltage",DATA_TYPE_FLOAT,&inv->charge_max_voltage,0,0 }, \
		{ "charge_at_max",DATA_TYPE_BOOL,&inv->charge_at_max,0,0 }, \
		{ "charge_amps",DATA_TYPE_FLOAT,&inv->charge_amps,0,0 }, \
		{ "discharge_voltage",DATA_TYPE_FLOAT,&inv->discharge_voltage,0,0 }, \
		{ "discharge_amps",DATA_TYPE_FLOAT,&inv->discharge_amps,0,0 }, \
		{ "battery_voltage",DATA_TYPE_FLOAT,&inv->battery_voltage,0,0 }, \
		{ "battery_current",DATA_TYPE_FLOAT,&inv->battery_current,0,0 }, \
		{ "battery_power",DATA_TYPE_FLOAT,&inv->battery_power,0,0 }, \
		{ "battery_temp",DATA_TYPE_FLOAT,&inv->battery_temp,0,0 }, \
		{ "soc",DATA_TYPE_FLOAT,&inv->soc,0,0 }, \
		{ "soh",DATA_TYPE_FLOAT,&inv->soh,0,0 }, \
		{ "grid_voltage_l1",DATA_TYPE_FLOAT,&inv->grid_voltage.l1,0,0 }, \
		{ "grid_voltage_l2",DATA_TYPE_FLOAT,&inv->grid_voltage.l2,0,0 }, \
		{ "grid_voltage_l3",DATA_TYPE_FLOAT,&inv->grid_voltage.l3,0,0 }, \
		{ "grid_frequency",DATA_TYPE_FLOAT,&inv->grid_frequency,0,0 }, \
		{ "grid_current_l1",DATA_TYPE_FLOAT,&inv->grid_current.l1,0,0 }, \
		{ "grid_current_l2",DATA_TYPE_FLOAT,&inv->grid_current.l2,0,0 }, \
		{ "grid_current_l3",DATA_TYPE_FLOAT,&inv->grid_current.l3,0,0 }, \
		{ "load_voltage_l1",DATA_TYPE_FLOAT,&inv->load_voltage.l1,0,0 }, \
		{ "load_voltage_l2",DATA_TYPE_FLOAT,&inv->load_voltage.l2,0,0 }, \
		{ "load_voltage_l3",DATA_TYPE_FLOAT,&inv->load_voltage.l3,0,0 }, \
		{ "load_frequency",DATA_TYPE_FLOAT,&inv->load_frequency,0,0 }, \
		{ "load_current_l1",DATA_TYPE_FLOAT,&inv->load_current.l1,0,0 }, \
		{ "load_current_l2",DATA_TYPE_FLOAT,&inv->load_current.l2,0,0 }, \
		{ "load_current_l3",DATA_TYPE_FLOAT,&inv->load_current.l3,0,0 }, \
		{ "pv_voltage_a",DATA_TYPE_FLOAT,&inv->pv_voltage.a,0,0 }, \
		{ "pv_voltage_b",DATA_TYPE_FLOAT,&inv->pv_voltage.b,0,0 }, \
		{ "pv_voltage_c",DATA_TYPE_FLOAT,&inv->pv_voltage.c,0,0 }, \
		{ "pv_current_a",DATA_TYPE_FLOAT,&inv->pv_current.a,0,0 }, \
		{ "pv_current_b",DATA_TYPE_FLOAT,&inv->pv_current.b,0,0 }, \
		{ "pv_current_c",DATA_TYPE_FLOAT,&inv->pv_current.c,0,0 }, \
		{ "yeild",DATA_TYPE_DOUBLE,&inv->yeild,0,0 }, \
		{ "errcode",DATA_TYPE_INT,&inv->errcode,0,0 }, \
		{ "errmsg",DATA_TYPE_STRING,&inv->errmsg,sizeof(inv->errmsg)-1,0 }, \
		{ "state",0,inv,0,ACTION }, \
		JSON_PROCTAB_END

#if 0
		{ "grid_power",&inv->grid_power,DATA_TYPE_FLOAT,0,0 }, \
		{ "load_power",&inv->load_power,DATA_TYPE_FLOAT,0,0 }, \
		{ "site_power",&inv->site_power,DATA_TYPE_FLOAT,0,0 },
#endif

static void _dump_state(char *name, void *dest, int flen, json_value_t *v) {
	solard_inverter_t *inv = dest;
	char format[16];
	int dlevel = (int) v;

	sprintf(format,"%%%ds: %%d\n",flen);
	if (debug >= dlevel) printf(format,name,inv->state);
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
		if (p->cb) p->cb(p->field,p->ptr,flen,(void *)dlevel);
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

int inverter_send_mqtt(inverter_session_t *s) {
	solard_inverter_t *inv = &s->info;
	char temp[256],*p;
	json_value_t *v;

//	strcpy(inv->id,s->id);
	strcpy(inv->name,s->name);
	v = inverter_to_json(inv);
	if (!v) return 1;
	p = json_dumps(v,0);
	sprintf(temp,"%s/%s/%s/%s",SOLARD_TOPIC_ROOT,SOLARD_ROLE_INVERTER,s->name,SOLARD_FUNC_DATA);
	dprintf(2,"sending mqtt data...\n");
	mqtt_pub(s->ap->m, temp, p, 0);
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
	return r;
}

static int inverter_write(inverter_session_t *s) {
	int r;

	if (!s->driver->write) return 1;

	if (s->driver->open) {
		dprintf(5,"%s: opening...\n", s->name);
		if (s->driver->open(s->handle)) {
			dprintf(1,"%s: open error\n",s->name);
			return 1;
		}
	}
	dprintf(5,"%s: writing...\n", s->name);
	r = s->driver->write(s->handle,&s->info,sizeof(s->info));
	if (s->driver->close) {
		dprintf(5,"%s: closing\n", s->name);
		s->driver->close(s->handle);
	}
	dprintf(5,"%s: returning: %d\n", s->name, r);
	if (!r) inverter_send_mqtt(s);
	return r;
}

int inverter_config(void *handle, char *action, char *caller, list lp) {
	inverter_session_t *s = handle;
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
	dprintf(1,"%s: returning: %d\n", s->name, r);
	return r;
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
	SOLARD_MODTYPE_INVERTER,
	"Inverter",
	inverter_init,					/* Init */
	(solard_module_new_t)inverter_new,		/* New */
	inverter_info,					/* Info */
	(solard_module_open_t)inverter_open,		/* Open */
	(solard_module_read_t)inverter_read,		/* Read */
	(solard_module_write_t)inverter_write,		/* Write */
	(solard_module_close_t)inverter_close,		/* Close */
	0,						/* Free */
	0,						/* Shutdown */
	(solard_module_control_t)0,			/* Control */
	(solard_module_config_t)inverter_config,	/* Config */
	(solard_module_get_handle_t)inverter_get_handle,
	(solard_module_get_handle_t)inverter_get_info,
};

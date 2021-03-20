
#include "agent.h"
#include "uuid.h"
#include "mqtt.h"
#include "json.h"

int inverter_get_config(solard_agent_t *ap) {
	return 0;
}

#if 0
/* Section,Keyword,Description,Type,Dest,DestLen,StrDefault */
#define INVERTER_PROCTAB \
		{ "","name","",DATA_TYPE_STRING,&inv->name,sizeof(inv->name), 0 }, \
		{ "","uuid","",DATA_TYPE_STRING,&inv->uuid,sizeof(inv->uuid), 0 }, \
		{ "","battery_voltage","",DATA_TYPE_FLOAT,&inv->battery_voltage,0, 0 }, \
		{ "","battery_current","",DATA_TYPE_FLOAT,&inv->battery_current,0, 0 }, \
		{ "","battery_power","",DATA_TYPE_FLOAT,&inv->battery_power,0, 0 }, \
		{ "","grid_power","",DATA_TYPE_FLOAT,&inv->grid_power,0, 0 }, \
		{ "","load_power","",DATA_TYPE_FLOAT,&inv->load_power,0, 0 }, \
		{ "","site_power","",DATA_TYPE_FLOAT,&inv->site_power,0, 0 }, \
		CFG_PROCTAB_END 

int inverter_to_json(JSON_Object *root_object, solard_inverter_t *inv) {
	cfg_proctab_t tab[] = { INVERTER_PROCTAB };

	return tab_to_json(root_object, tab);
}
#endif

int inverter_mqtt_send(solard_agent_t *ap) {
	solard_inverter_t *inv = &ap->inv;
	register int i,j;
	char temp[256],*p;
	json_object_t *o;
	struct inverter_states {
		int mask;
		char *label;
	} states[] = {
		{ SOLARD_INVERTER_STATE_RUNNING, "Running" },
		{ SOLARD_INVERTER_STATE_CHARGING, "Charging" },
		{ SOLARD_INVERTER_STATE_GRID, "GridConnected" },
		{ SOLARD_INVERTER_STATE_GEN, "GeneratorRunning" },
	};
#define NSTATES (sizeof(states)/sizeof(struct inverter_states))

	o = json_create_object();
	if (get_timestamp(temp,sizeof(temp),1) == 0) json_add_string(o, "timestamp", temp);
	json_add_string(o, "name", inv->name);
	json_add_string(o, "uuid", inv->uuid);
	json_add_number(o, "error", inv->error);
	json_add_string(o, "errmsg", inv->errmsg);
	json_add_number(o, "battery_voltage", inv->battery_voltage);
	json_add_number(o, "battery_current", inv->battery_current);
	json_add_number(o, "battery_power", inv->battery_power);
	json_add_number(o, "grid_power", inv->grid_power);
	json_add_number(o, "load_power", inv->load_power);
	json_add_number(o, "site_power", inv->site_power);
#if 0
	p = temp;
	p += sprintf(p,"[ ");
	for(i=0; i < inv->ntemps; i++) {
		if (i) p += sprintf(p,",");
		p += sprintf(p, "%.1f",inv->temps[i]);
	}
	strcat(temp," ]");
	dprintf(4,"temp: %s\n", temp);
	o_dotset_value(o, "temps", json_parse_string(temp));
#endif
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
	json_add_string(o, "states", temp);

	p = json_dumps(o,inv->conf->pretty);
	sprintf(temp,"/Inverter/%s",inv->name);
#if 1
	if (mqtt_send(inv->conf->m, temp, p, 15)) {
		dprintf(1,"mqtt send error!\n");
		return 1;
	}
#else
	mqtt_fullsend(inv->conf->mqtt.host,inv->name, p, temp, inv->conf->mqtt_username, inv->conf->mqtt_password);
#endif
	free(p);
	json_destroy(o);
	return 0;
}

int inverter_read(solard_agent_t *inv) {
	int r;

	if (!inv) return 1;
	solard_clear_state(inv,SOLARD_INVERTER_STATE_UPDATED);

//	dprintf(3,"inv: name: %s, type: %s, transport: %s\n", inv->name, inv->type, inv->transport);
	dprintf(5,"%s: opening...\n", inv->name);
	if (inv->funcs->open(inv->handle)) {
		dprintf(1,"%s: open error\n",inv->name);
		return 1;
	}
	dprintf(5,"%s: reading...\n", inv->name);
	r = inv->funcs->read(inv->handle,0,0);
	dprintf(5,"%s: closing\n", inv->name);
	inv->funcs->close(inv->handle);
	dprintf(5,"%s: returning: %d\n", inv->name, r);
	if (!r) solard_set_state(inv,SOLARD_INVERTER_STATE_UPDATED);
	return r;
}

int inverter_write(solard_agent_t *inv) {
	if (!inv) return 1;
	if (inv->funcs->open(inv->handle)) return 1;
	if (inv->funcs->write(inv->handle) < 0) {
		dprintf(1,"error writing to %s\n", inv->name);
		return 1;
	}
	inv->funcs->close(inv->handle);
	inverter_mqtt_send(inv);
	return 0;
}

#if 0
static void *inverter_thread(void *arg) {
	solard_agent_t *conf = arg;
	solard_inverter_t *inv = conf->inverter;
	int is_open,r;

	/* All we do here is just write out the vals every <inverter_update> seconds*/
	is_open = (inv->open(inv->handle) == 0);
	while(1) {
		solard_clear_state(inv,SOLARD_INVERTER_STATE_UPDATED);
		dprintf(1,"is_open: %d\n", is_open);
		if (!is_open) {
			is_open = (inv->open(inv->handle) == 0);
			if (!is_open) {
				sleep(1);
				continue;
			}
		}
		dprintf(1,"reading...\n");
		r = inv->read(inv->handle);
		dprintf(1,"r: %d\n", r);
		if (!r) solard_set_state(inv,SOLARD_INVERTER_STATE_UPDATED);
		else continue;
		dprintf(1,"writing...\n");
		inv->write(inv->handle);
		sleep(1);
	}
	inv->close(inv->handle);
	return 0;
}
#endif

#if 0
int inverter_add(solard_agent_t *conf, solard_inverter_t *inv) {
	solard_module_t *mp, *tp;

	dprintf(1,"conf: %p, inv: %p\n", conf, inv);

	/* Get the transport */
	dprintf(1,"transport: %s\n", inv->transport);
	tp = solard_load_module(conf,inv->transport,SOLARD_MODTYPE_TRANSPORT);
	if (!tp) return 1;

	/* Load our module */
	dprintf(1,"type: %s\n", inv->type);
	mp = solard_load_module(conf,inv->type,SOLARD_MODTYPE_INVERTER);
	if (!mp) return 1;

	/* Create an instance of the inverter*/
	dprintf(3,"mp: %p\n",mp);
	if (mp) {
		inv->handle = mp->new(conf, inv, tp);
		if (!inv->handle) {
			fprintf(stderr,"module %s->new returned null!\n", mp->name);
			return 1;
		}
	}

	/* Get capability mask */
	dprintf(1,"capabilities: %02x\n", mp->capabilities);
	inv->capabilities = mp->capabilities;

	/* Set the convienience funcs */
	inv->open = mp->open;
	inv->read = mp->read;
	inv->write = mp->write;
	inv->close = mp->close;

	/* Update conf */
	conf->inverter = inv;
	inv->conf = conf;

	dprintf(3,"done!\n");
	return 0;
}
#endif

int inverter_init(solard_agent_t *conf) {
	return 0;
#if 0
	solard_inverter_t *inv;

	/* Get the inverter config, if any (not an error if not found) */
	if (!cfg_get_item(conf->cfg,"inverter","type")) return 0;

	inv = calloc(1,sizeof(*inv));
	if (!inv) {
		perror("calloc inverter\n");
		return 1;
	}
	get_tab(conf,"inverter",inv);
	return inverter_add(conf,inv);
#endif
}

#if 0
int inverter_start_update(solard_agent_t *conf) {
	pthread_attr_t attr;

	/* Create a detached thread */
	dprintf(3,"Creating thread...\n");
	if (pthread_attr_init(&attr)) {
		dprintf(0,"pthread_attr_init: %s\n",strerror(errno));
		return 1;
	}
	if (pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_DETACHED)) {
		dprintf(0,"pthread_attr_setdetachstate: %s\n",strerror(errno));
		return 1;
	}
	if (pthread_create(&conf->inverter_tid,&attr,&inverter_thread,conf)) {
		dprintf(0,"pthread_create: %s\n",strerror(errno));
		return 1;
	}
	return 0;
}
#endif

solard_module_t inverter = {
	SOLARD_MODTYPE_BATTERY,
	"Battery",
	0,			/* Init */
	0,			/* New */
	(solard_module_info_t)0,			/* Info */
	(solard_module_open_t)0,			/* Open */
	(solard_module_read_t)0,			/* Read */
	(solard_module_write_t)0,			/* Write */
	(solard_module_close_t)0,			/* Close */
	0,				/* Free */
	0,				/* Shutdown */
	(solard_module_control_t)0,		/* Control */
	(solard_module_config_t)0,			/* Config */
};

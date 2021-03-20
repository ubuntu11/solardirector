
#include "battery.h"

int battery_init(solard_battery_t *bp, char *section_name) {
	solard_battery_t *pp = &bp->bat;
	struct cfg_proctab packtab[] = {
		{ section_name, "name", "Battery name", DATA_TYPE_STRING,&pp->name,sizeof(pp->name), 0 },
		{ section_name, "uuid", "Battery UUID", DATA_TYPE_STRING,&pp->uuid,sizeof(pp->uuid), 0 },
		{ section_name, "capacity", "Pack Capacity in AH", DATA_TYPE_FLOAT,&pp->capacity, 0, 0 },
		CFG_PROCTAB_END
	};

	cfg_get_tab(bp->cfg,packtab);
	if (!strlen(pp->name)) strcpy(pp->name,section_name);
	if (debug >= 3) cfg_disp_tab(packtab,0,1);

	return 0;
}

void *battery_new(solard_battery_t *conf, ...) {
	solard_battery_t *bp;
        va_list ap;

	bp = calloc(1,sizeof(*bp));
	if (!bp) {
		log_write(LOG_SYSERR,"battery_new: calloc");
		return 0;
	}

	va_start(ap,conf);
	bp->bms = va_arg(ap,solard_module_t *);
	bp->bms_handle = va_arg(ap,void *);
	va_end(ap);

	return bp;
}

int battery_open(solard_battery_t *bp) {
	return 0;
}

int battery_close(solard_battery_t *bp) {
	return 0;
}
int battery_send_mqtt(solard_battery_t *pp) {
	register int i,j;
	char temp[256],*p;
	unsigned long mask;
	json_object_t *o;
	struct battery_states {
		int mask;
		char *label;
	} states[] = {
		{ SOLARD_PACK_STATE_CHARGING, "Charging" },
		{ SOLARD_PACK_STATE_DISCHARGING, "Discharging" },
		{ SOLARD_PACK_STATE_BALANCING, "Balancing" },
	};
#define NSTATES (sizeof(states)/sizeof(struct battery_states))

	/* Create JSON data */
	o = json_create_object();
	if (get_timestamp(temp,sizeof(temp),1) == 0) json_add_string(o, "timestamp", temp);
	json_add_string(o, "name", pp->name);
	json_add_string(o, "uuid", pp->uuid);
	json_add_number(o, "state", pp->state);
	json_add_number(o, "errcode", pp->error);
	json_add_string(o, "errmsg", pp->errmsg);
	json_add_number(o, "capacity", pp->capacity);
	json_add_number(o, "voltage", pp->voltage);
	json_add_number(o, "current", pp->current);
	if (pp->ntemps) {
		json_array_t *a;

		a = json_create_array();
		dprintf(4,"ntemps: %d\n", pp->ntemps);
		for(i=0; i < pp->ntemps; i++) json_array_add_number(a,pp->temps[i]);
		json_add_array(o,"temps",a);
	}
	if (pp->cells) {
		json_array_t *a;

		a = json_create_array();
		dprintf(4,"cells: %d\n", pp->cells);
		for(i=0; i < pp->cells; i++) json_array_add_number(a,pp->cellvolt[i]);
		json_add_array(o,"cellvolt",a);
	}
	json_add_number(o, "cell_min", pp->cell_min);
	json_add_number(o, "cell_max", pp->cell_max);
	json_add_number(o, "cell_diff", pp->cell_diff);
	json_add_number(o, "cell_avg", pp->cell_avg);

	/* States */
	temp[0] = 0;
	p = temp;
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
	for(i=0; i < pp->cells; i++) {
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
	p = json_dumps(o,pp->conf->pretty);
	sprintf(temp,"/SolarD/Battery");
	dprintf(2,"sending mqtt data...\n");
	if (mqtt_pub(pp->conf->m, temp, p, 0)) {
		dprintf(1,"mqtt send error!\n");
		return 1;
	}
	free(p);
	json_destroy(o);
	return 0;
}

char *battery_info(solard_battery_t *bp) {
	solard_battery_t *pp = &bp->bat;
	char *json;

	dprintf(4,"%s: opening...\n", pp->name);
	if (bp->funcs->open(bp->handle)) {
		dprintf(1,"%s: open error\n",pp->name);
		return 0;
	}
	dprintf(4,"%s: getting info...\n", pp->name);
	json = bp->funcs->info(bp->handle);
	dprintf(4,"%s: closing\n", pp->name);
	bp->funcs->close(bp->handle);
	return json;
}

int battery_read(solard_battery_t *bp) {
	solard_battery_t *pp = &bp->bat;
	int r;

	dprintf(4,"%s: opening...\n", pp->name);
	if (bp->funcs->open(bp->handle)) {
		dprintf(1,"%s: open error\n",pp->name);
		return 1;
	}
	dprintf(4,"%s: reading...\n", pp->name);
	r = bp->funcs->read(bp->handle);
	dprintf(4,"%s: closing\n", pp->name);
	bp->funcs->close(bp->handle);
	dprintf(4,"%s: returning: %d\n", pp->name, r);
	if (!r) {
		float total;
		int i;

		solard_set_state(pp,SOLARD_PACK_STATE_UPDATED);
		/* Set min/max/diff/avg */
		pp->cell_max = 0.0;
//		pp->cell_min = pp->conf->cell_crit_high;
		pp->cell_min = 9999999.99;
		total = 0.0;
		for(i=0; i < pp->cells; i++) {
			if (pp->cellvolt[i] < pp->cell_min)
				pp->cell_min = pp->cellvolt[i];
			if (pp->cellvolt[i] > pp->cell_max)
				pp->cell_max = pp->cellvolt[i];
			total += pp->cellvolt[i];
		}
		pp->cell_diff = pp->cell_max - pp->cell_min;
		pp->cell_avg = total / pp->cells;
		if (!(int)pp->voltage) pp->voltage = total;
		battery_send_mqtt(pp);
	}
	return r;
}

int battery_write(solard_battery_t *bp) {
	return 0;
}

int battery_config(solard_battery_t *bp, char *action, list lp) {
//	solard_battery_t *pp = &bp->bat;

	/* If we dont have config, dont bother */
	dprintf(1,"bp->funcs: %p\n", bp->funcs);
	if (!bp->funcs) return 1;

	dprintf(1,"bp->funcs->config: %p\n", bp->funcs->config);
	if (!bp->funcs->config) return 1;

	bp->funcs->config(bp->handle,action,lp);
	return 0;
}

int battery_control(solard_battery_t *bp) {
	return 0;
}

int battery_process(solard_battery_t *bp, solard_message_t *msg) {
	return 0;
}

solard_module_t battery = {
	SOLARD_MODTYPE_BATTERY,
	"Battery",
	battery_init,			/* Init */
	battery_new,			/* New */
	(solard_module_info_t)battery_info,			/* Info */
	(solard_module_open_t)battery_open,			/* Open */
	(solard_module_read_t)battery_read,			/* Read */
	(solard_module_write_t)battery_write,			/* Write */
	(solard_module_close_t)battery_close,			/* Close */
	0,				/* Free */
	0,				/* Shutdown */
	(solard_module_control_t)battery_control,		/* Control */
	(solard_module_config_t)battery_config,			/* Config */
};

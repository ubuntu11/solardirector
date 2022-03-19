
/*
Copyright (c) 2021, Stephen P. Shoecraft
All rights reserved.

This source code is licensed under the BSD-style license found in the
LICENSE file in the root directory of this source tree.
*/

#include "si.h"
#include "transports.h"

static void *si_new(void *driver, void *driver_handle) {
	si_session_t *s;

	s = calloc(1,sizeof(*s));
	if (!s) {
		perror("si_new: calloc");
		return 0;
	}
	if (driver) {
		s->can = driver;
		s->can_handle = driver_handle;
	}

	dprintf(1,"returning: %p\n", s);
	return s;
}

static int si_destroy(void *h) {
	si_session_t *s = h;
	if (!s) return 1;
	/* Close and destroy transport */
	dprintf(1,"s->can: %p, s->can_handle: %p\n", s->can, s->can_handle);
	if (s->can && s->can_handle) {
		if (solard_check_state(s,SI_STATE_OPEN)) s->can->close(s->can_handle);
		s->can->destroy(s->can_handle);
	}
	free(s);
	return 0;
}

static int si_open(void *h) {
	si_session_t *s = h;
	int r;

	dprintf(3,"s: %p\n", s);

	r = 0;
	if (!solard_check_state(s,SI_STATE_OPEN) && s->can) {
		if (s->can->open(s->can_handle) == 0)
			solard_set_state(s,SI_STATE_OPEN);
		else {
			log_error("error opening %s device %s:%s\n", s->can_transport, s->can_target, s->can_topts);
			r = 1;
		}
	}
	dprintf(3,"returning: %d\n", r);
	return r;
}

static int si_close(void *handle) {
	si_session_t *s = handle;
	int r;

	dprintf(3,"s: %p\n", s);
	r = 0;
	if (solard_check_state(s,SI_STATE_OPEN) && s->can) {
		if (s->can->close(s->can_handle) == 0)
			solard_clear_state(s,SI_STATE_OPEN);
		else
			r = 1;
	}
	dprintf(3,"returning: %d\n", r);
	return r;
}

static int si_read(void *handle, uint32_t *control, void *buf, int buflen) {
	si_session_t *s = handle;

	dprintf(1,"can_connected: %d, smanet_connected: %d\n", s->can_connected, s->smanet_connected);

	if (!s->can_connected) {
		dprintf(1,"can_transport: %s, can_target: %s, can_topts: %s, can_init: %d\n",
			s->can_transport, s->can_target, s->can_topts, s->can_init);
		if (strlen(s->can_transport) && strlen(s->can_target) && !s->can_init) si_can_init(s);
		if (s->can_init && si_open(s) == 0) s->can_connected = true;
	}
	if (s->can_connected && si_can_read_data(s,0)) {
		dprintf(1,"===> setting can_connected to false\n");
		s->can_connected = false;
		si_close(s);
	}

	/* XXX This doesnt really belong here but I dont want to init/setup before 1 can cycle in BMS mode */
	dprintf(1,"startup: %d, can_connected: %d, smanet_connected: %d\n", s->startup, s->can_connected, s->smanet_connected);
	if ((s->startup != 1 || !s->can_connected) && !s->smanet_connected) {
		if (!s->smanet_connected) {
			dprintf(1,"smanet_transport: %s, smanet_target: %s, smanet_topts: %s\n",
				s->smanet_transport, s->smanet_target, s->smanet_topts);
			if (!s->smanet) {
				if (strlen(s->smanet_transport) && strlen(s->smanet_target)) {
					s->smanet = smanet_init(s->smanet_transport,s->smanet_target,s->smanet_topts);
					if (s->smanet) s->smanet_connected = s->smanet->connected;
				}
			} else if (strlen(s->smanet_transport) && strlen(s->smanet_target)) {
				if (smanet_connect(s->smanet, s->smanet_transport, s->smanet_target, s->smanet_topts))
					dprintf(1,"%s\n",smanet_get_errmsg(s->smanet));
				s->smanet_connected = s->smanet->connected;
				if (s->smanet_connected) dprintf(1,"===> setting smanet_connected to true\n");
			}
			if (s->smanet_connected && !s->smanet_added) si_smanet_setup(s);
		}
	}
	if (s->smanet_connected && si_smanet_read_data(s)) return 1;

	/* Auto set bms_mode if BatTyp is BMS */
	if (!s->bms_mode && s->can_connected && s->smanet_connected && !strlen(s->battery_type)) {
		char *t;
		double d;

		if (smanet_get_value(s->smanet,"BatTyp",&d,&t) == 0) {
			if (t) {
				dprintf(0,"BatTyp: %s\n", t);
				strncpy(s->battery_type,t,sizeof(s->battery_type)-1);
				if (strcmp(s->battery_type,"LiIon_Ext-BMS") == 0) {
					log_info("Setting BMS mode\n");
					s->bms_mode = 1;
				}
			}
		}
	}

	/* make sure min/max are set to _something_ */
	if (float_equals(s->min_voltage,0.0)) {
		log_warning("min_voltage is 0.0, setting to 41.0\n", 41);
		s->min_voltage = 41;
	}
	if (float_equals(s->max_voltage,0.0)) {
		log_warning("max_voltage is 0.0, setting to 58.1\n", 41);
		s->max_voltage = 58.1;
	}

	/* Calculate SOC */
	dprintf(4,"battery_voltage: %.1f, min_voltage: %.1f, max_voltage: %.1f\n", s->data.battery_voltage, s->min_voltage, s->max_voltage);
	s->soc = ( ( s->data.battery_voltage - s->min_voltage) / (s->max_voltage - s->min_voltage) ) * 100.0;
	dprintf(4,"SoC: %f\n", s->soc);
	s->soh = 100.0;

	return 0;
}

static void si_pub(si_session_t *s) {
	char out[128],status[64];
	json_proctab_t tab[] = {
		{ "name",DATA_TYPE_STRING,s->ap->instance_name,sizeof(s->ap->instance_name)-1,0 },
		{ "input_voltage",DATA_TYPE_FLOAT,&s->data.ac2_voltage,0,0 }, 
		{ "input_frequency",DATA_TYPE_FLOAT,&s->data.ac2_frequency,0,0 }, 
		{ "input_current",DATA_TYPE_FLOAT,&s->data.ac2_current,0,0 }, 
		{ "input_power",DATA_TYPE_FLOAT,&s->data.ac2_power,0,0 }, 
		{ "output_voltage",DATA_TYPE_FLOAT,&s->data.ac1_voltage,0,0 }, 
		{ "output_frequency",DATA_TYPE_FLOAT,&s->data.ac1_frequency,0,0 }, 
		{ "output_current",DATA_TYPE_FLOAT,&s->data.ac1_current,0,0 }, 
		{ "output_power",DATA_TYPE_FLOAT,&s->data.ac1_power,0,0 }, 
		{ "battery_voltage",DATA_TYPE_FLOAT,&s->data.battery_voltage,0,0 }, 
		{ "battery_current",DATA_TYPE_FLOAT,&s->data.battery_current,0,0 }, 
		{ "battery_power",DATA_TYPE_FLOAT,&s->data.battery_power,0,0 }, 
		{ "battery_temp",DATA_TYPE_FLOAT,&s->data.battery_temp,0,0 }, 
		{ "battery_level",DATA_TYPE_FLOAT,&s->data.battery_soc,0,0 }, 
		{ "status",DATA_TYPE_STRING,&status,sizeof(status),0 }, 
		{ 0 }
	};
	json_value_t *v;
	char *p;
	char topic[SOLARD_TOPIC_SIZE];

	p = status;
	*status = 0;
	#define addstat(stat,cond,str) if (cond) p += sprintf(p,"[%s]",str)
	addstat(status,s->bms_mode,"bms");
	addstat(status,s->can_connected,"can");
	addstat(status,s->smanet_connected,"smanet");
	addstat(status,s->charge_mode != 0,"charging");
//	addstat(status,s->feed != 0,"feeding");
	addstat(status,s->data.GdOn,"grid");
	addstat(status,s->data.GnOn,"gen");

	p = out;
	p += sprintf(p,"%s Battery: voltage: %.1f, current: %.1f",status,s->data.battery_voltage, s->data.battery_current);
//	if (solard_check_state(s,SI_HAVE_TEMP)) p += sprintf(p,", temp: %.1f", s->data.battery_temp);
	p += sprintf(p,", level: %.1f", s->data.battery_soc);
	if (strcmp(out,s->last_out) != 0) log_info("%s\n",out);
	strcpy(s->last_out,out);

	v = json_from_tab(tab);
	p = json_dumps(v,0);
	dprintf(2,"sending mqtt data...\n");
	agent_mktopic(topic,sizeof(topic)-1,s->ap,s->ap->instance_name,SOLARD_FUNC_DATA);
	dprintf(2,"topic: %s\n", topic);
	if (mqtt_pub(s->ap->m, topic, p, 1, 0)) log_error("si_pub: error sending mqtt message!\n");
	free(p);
	json_destroy_value(v);
	return;
}

static int si_write(void *handle, uint32_t *control, void *buffer, int len) {
	si_session_t *s = handle;

	/* Charging controlled by JS */
	if (agent_script_exists(s->ap,"charge.js")) agent_start_script(s->ap,"charge.js");

	/* We publish here */
	if (agent_script_exists(s->ap,"pub.js"))
		agent_start_script(s->ap,"pub.js");
	else
		si_pub(s);

	dprintf(1,"can_connected: %d, bms_mode: %d\n", s->can_connected, s->bms_mode);
	if (s->can_connected && s->bms_mode) {
		if (si_can_write_data(s)) return 1;
	} else if (s->startup) {
		s->startup = 0;
	}
	return 0;
}

solard_driver_t si_driver = {
	"si",
	si_new,		/* New */
	si_destroy,	/* Free */
	si_open,	/* Open */
	si_close,	/* Close */
	si_read,	/* Read */
	si_write,	/* Write */
	si_config,	/* Config */
};

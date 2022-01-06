/*
Copyright (c) 2021, Stephen P. Shoecraft
All rights reserved.

This source code is licensed under the BSD-style license found in the
LICENSE file in the root directory of this source tree.
*/

#include "si.h"

int si_read_smanet_data(si_session_t *s) {
	config_section_t *sec;
	config_property_t *p;
	struct {
		char *smanet_name;
		char *data_name;
		int can;
	} *pp, parminfo[] = {
		{ "ExtVtg", "ac2_voltage_l1", 1 },
		{ "ExtVtgSlv1", "ac2_voltage_l2", 1 },
		{ "ExtFrq", "ac2_frequency", 1 },
		{ "TotExtCur", "ac2_current", 0 },
		{ "InvVtg", "ac1_voltage_l1", 1 },
		{ "InvVtgSlv1", "ac1_voltage_l2", 1 },
		{ "InvFrq", "ac1_frequency", 1 },
		{ "TotInvCur", "ac1_current", 0 },
		{ "BatVtg", "battery_voltage", 1 },
		{ "TotBatCur", "battery_current", 1 },
		{ "BatTmp", "battery_temp", 1 },
		{ 0, 0, 0 }
	};
	smanet_multreq_t *mr;
	int count,mr_size,i;

	sec = config_get_section(s->ap->cp,"si_data");
	if (!sec) return 1;

	for(pp = parminfo; pp->smanet_name; pp++) {
		if (pp->can && s->can_connected) continue;
		count++;
	}
	mr_size = count * sizeof(smanet_multreq_t);
	dprintf(1,"mr_size: %d\n", mr_size);
	mr = malloc(mr_size);
	if (!mr) return 1;

	i = 0;
	for(pp = parminfo; pp->smanet_name; pp++) {
		if (pp->can && s->can_connected) continue;
		mr[i++].name = pp->smanet_name;
	}
	if (smanet_get_multvalues(s->smanet,mr,count)) {
		free(mr);
		return 1;
	}
	i = 0;
	for(pp = parminfo; pp->smanet_name; pp++) {
		if (pp->can && s->can_connected) continue;
		p = config_section_get_property(sec, pp->data_name);
		if (!p) break;
		dprintf(1,"mr[%d]: value: %f, text: %s\n", i, mr[i].value, mr[i].text);
		if (mr[i].text) 
			p->len = conv_type(p->type, p->dest, p->dsize, DATA_TYPE_STRING, mr[i].text, strlen(mr[i].text) );
		else 
			p->len = conv_type(p->type, p->dest, p->dsize, DATA_TYPE_DOUBLE, &mr[i].value, 0 );
		i++;
		dprintf(1,"%s: %.1f\n", p->name, *((float *)p->dest));
	}
	if (!s->can_connected) {
		s->data.ac1_voltage = s->data.ac1_voltage_l1 + s->data.ac1_voltage_l2;
		dprintf(4,"ac1_voltage: %.1f, ac1_frequency: %.1f\n",s->data.ac1_voltage,s->data.ac1_frequency);
		s->data.ac2_voltage = s->data.ac2_voltage_l1 + s->data.ac2_voltage_l2;
		dprintf(4,"ac2: voltage: %.1f, frequency: %.1f\n",s->data.ac2_voltage,s->data.ac2_frequency);
	}
	free(mr);
	return 0;
}

int si_read(si_session_t *s, void *buf, int buflen) {

	if (s->can_connected && si_read_can_data(s)) return 1;
	if ((unsigned long)buf == 0xDEADBEEF) return 0;

	dprintf(1,"startup: %d\n", s->startup);
	if (s->startup == 0 || s->can_connected == 0) {
		dprintf(1,"smanet: %p, smanet_connected: %d\n", s->smanet, s->smanet_connected);
		if (!s->smanet_connected) {
			dprintf(1,"smanet_transport: %s, smanet_target: %s, smanet_topts: %s\n",
				s->smanet_transport, s->smanet_target, s->smanet_topts);
			if (!s->smanet) {
				if (strlen(s->smanet_transport) && strlen(s->smanet_target)) {
					si_smanet_init(s);
					s->smanet_connected = s->smanet->connected;
				}
			} else if (strlen(s->smanet_transport) && strlen(s->smanet_target)) {
				if (smanet_connect(s->smanet, s->smanet_transport, s->smanet_target, s->smanet_topts))
					log_error(smanet_get_errmsg(s->smanet));
				s->smanet_connected = s->smanet->connected;
			}
			if (s->smanet_connected && !s->smanet_added) si_smanet_setup(s);
		}
		if (s->smanet_connected) si_read_smanet_data(s);
	}

	/* Calculate SOC */
	if (!si_isvrange(s->min_voltage)) {
		log_warning("setting min_voltage to %.1f\n", 41.0);
		s->min_voltage = 41.0;
	}
	if (!si_isvrange(s->max_voltage)) {
		log_warning("setting max_voltage to %.1f\n", 58.1);
		s->max_voltage = 58.1;
	}
	dprintf(4,"user_soc: %.1f, battery_voltage: %.1f\n", s->user_soc, s->data.battery_voltage);
	dprintf(4,"min_voltage: %.1f, max_voltage: %.1f\n", s->min_voltage, s->max_voltage);
	if (s->user_soc < 0.0) {
		s->soc = ( ( s->data.battery_voltage - s->min_voltage) / (s->max_voltage - s->min_voltage) ) * 100.0;
		if (s->soc < 0.0) s->soc = 0.0;
	} else  {
		s->soc = s->user_soc;
	}
	dprintf(1,"SoC: %f\n", s->soc);
	s->soh = 100.0;

#if 0
	/* Adjust SOC so GdSocEna and GnSocEna dont disconnect until we're done charging */
	dprintf(1,"grid_connected: %d, charge_mode: %d, grid_soc_stop: %.1f, soc: %.1f\n",
		s->grid_connected,s->charge_mode,s->grid_soc_stop,s->soc);
	dprintf(1,"gen_connected: %d, charge_mode: %d, gen_soc_stop: %.1f, soc: %.1f\n",
		s->gen_connected,s->charge_mode,s->gen_soc_stop,s->soc);
	if (s->grid_connected && s->charge_mode && s->grid_soc_stop > 1.0 && s->soc >= s->grid_soc_stop)
		s->soc = s->grid_soc_stop - 1;
	else if (s->gen_connected && s->charge_mode && s->gen_soc_stop > 1.0 && s->soc >= s->gen_soc_stop)
		s->soc = s->gen_soc_stop - 1;
	if (s->data.battery_voltage != s->last_battery_voltage || s->soc != s->last_soc) {
		log_info("%s%sBattery Voltage: %.1fV, SoC: %.1f%%\n",
			(s->data.Run ? "[Running] " : ""), (s->sim ? "(SIM) " : ""), s->data.battery_voltage, s->soc);
		s->last_battery_voltage = s->data.battery_voltage;
		s->last_soc = s->soc;
	}
#endif

	dprintf(1,"returning...\n");
	return 0;
}

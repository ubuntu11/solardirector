
#include "si.h"

int si_read_data(si_session_t *s) {

	dprintf(1,"can_connected: %d, smanet_connected: %d\n", s->can_connected, s->smanet_connected);

	memset(&s->data,0,sizeof(s->data));
	if (!s->can_connected) {
		dprintf(1,"can_transport: %s, can_target: %s, can_topts: %s\n",
			s->can_transport, s->can_target, s->can_topts);
		if (strlen(s->can_transport) && strlen(s->can_target)) si_can_init(s);
	}
	if (s->can_connected && si_can_read_data(s,0)) {
		dprintf(0,"===> setting can_connected to false\n");
		s->can_connected = false;
	}

	/* XXX This doesnt really belong here but I dont want to init/setup before 1 can cycle in BMS mode */
	dprintf(1,"startup: %d\n", s->startup);
	if (s->startup != 1 || s->bms_mode != 1) {
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
				if (s->smanet_connected) printf("===> setting smanet_connected to true\n");
			}
			if (s->smanet_connected && !s->smanet_added) si_smanet_setup(s);
			/* Auto set bms_mode if BatTyp is BMS */
			if (s->smanet_connected && s->bms_mode == -1) {
				char *t;
				double d;

				if (smanet_get_value(s->smanet,"BatTyp",&d,&t) == 0) {
					dprintf(0,"BatTyp: %s\n", t);
					if (t && strcmp(t,"LiIon_Ext-BMS") == 0) s->bms_mode = 1;
				}
			}
		}
	}
	if (s->smanet_connected && si_smanet_read_data(s)) return 1;

	/* Calculate SOC */
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

	return 0;
}


#include "si.h"

int si_read_data(si_session_t *s) {

	if (s->can_connected && si_can_read_data(s)) return 1;

	s->smanet_data = 1;
	if (s->smanet_data) {
		/* XXX This doesnt really belong here but I dont want to init/setup before 1 can cycle */
		dprintf(0,"startup: %d, can_connected: %d\n", s->startup, s->can_connected);
		if (s->startup != 1 || s->can_connected == 0) {
			dprintf(1,"smanet: %p, smanet_connected: %d\n", s->smanet, s->smanet_connected);
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
						log_error(smanet_get_errmsg(s->smanet));
					s->smanet_connected = s->smanet->connected;
				}
				if (s->smanet_connected && !s->smanet_added) si_smanet_setup(s);
			}
		}
		if (s->smanet_connected && si_smanet_read_data(s)) return 1;
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

	return 0;
}

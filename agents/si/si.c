
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

	memset(&s->data,0,sizeof(s->data));
	if (!s->can_connected) {
		dprintf(1,"can_transport: %s, can_target: %s, can_topts: %s, can_init: %d\n",
			s->can_transport, s->can_target, s->can_topts, s->can_init);
		if (strlen(s->can_transport) && strlen(s->can_target) && !s->can_init) si_can_init(s);
		if (s->can_init && si_open(s) == 0) s->can_connected = true;
	}
	if (s->can_connected && si_can_read_data(s,0)) {
		dprintf(0,"===> setting can_connected to false\n");
		s->can_connected = false;
		si_close(s);
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

static int si_write(void *handle, uint32_t *control, void *buffer, int len) {
	si_session_t *s = handle;

	dprintf(1,"can_connected: %d, bms_mode: %d\n", s->can_connected, s->bms_mode);
	return(s->can_connected && s->bms_mode ? si_can_write_data(s) : 0);
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


/*
Copyright (c) 2021, Stephen P. Shoecraft
All rights reserved.

This source code is licensed under the BSD-style license found in the
LICENSE file in the root directory of this source tree.
*/

#include "si.h"
#include <pthread.h>
#ifndef __WIN32
#include <linux/can.h>
#include <sys/signal.h>
#endif
#include "transports.h"

solard_driver_t *si_transports[] = { &can_driver, &rdev_driver, 0 };

#define _getshort(v) (short)( ((v)[1]) << 8 | ((v)[0]) )
#define _getlong(v) (long)( ((v)[3]) << 24 | ((v)[2]) << 16 | ((v)[1]) << 8 | ((v)[0]) )
#define si_putshort(p,v) { float tmp; *((p+1)) = ((int)(tmp = v) >> 8); *((p)) = (int)(tmp = v); }
#define si_putlong(p,v) { *((p+3)) = ((int)(v) >> 24); *((p)+2) = ((int)(v) >> 16); *((p+1)) = ((int)(v) >> 8); *((p)) = ((int)(v) & 0x0F); }

static void *si_can_recv_thread(void *handle) {
	si_session_t *s = handle;
	struct can_frame frame;
	int bytes,id,idx;
	uint32_t mask;
#ifndef __WIN32
	sigset_t set;

	/* Ignore SIGPIPE */
	sigemptyset(&set);
	sigaddset(&set, SIGPIPE);
	sigprocmask(SIG_BLOCK, &set, NULL);
#endif

	dprintf(3,"thread started!\n");
	while(solard_check_state(s,SI_STATE_RUNNING)) {
		dprintf(8,"open: %d\n", solard_check_state(s,SI_STATE_OPEN));
		if (!solard_check_state(s,SI_STATE_OPEN)) {
			memset(&s->bitmaps,0,sizeof(s->bitmaps));
			sleep(1);
			continue;
		}
		bytes = s->can->read(s->can_handle,&frame,0xffff);
		dprintf(8,"bytes: %d\n", bytes);
		if (bytes < 1) {
			memset(&s->bitmaps,0,sizeof(s->bitmaps));
			sleep(1);
			continue;
		}
		dprintf(8,"frame.can_id: %03x\n",frame.can_id);
		if (frame.can_id < SI_FRAME_START || frame.can_id >= SI_FRAME_START+SI_NFRAMES) continue;
//		bindump("frame",&frame,sizeof(frame));
		id = frame.can_id - SI_FRAME_START;
		idx = id / 32;
		mask = 1 << (id % 32);
		memcpy(&s->frames[id],&frame,sizeof(frame));
		s->bitmaps[idx] |= mask;
	}
	dprintf(1,"returning!\n");
	return 0;
}

static int si_can_get_local(si_session_t *s, int can_id, uint8_t *data, int datasz) {
	char what[16];
	uint32_t mask;
	int id,idx,retries,len;

	dprintf(5,"id: %03x, data: %p, len: %d\n", can_id, data, datasz);
	if (can_id < SI_FRAME_START || can_id >= SI_FRAME_START+SI_NFRAMES) return 1;

	id = can_id - SI_FRAME_START;
	idx = id / 32;
	mask = 1 << (id % 32);
	dprintf(5,"id: %03x, mask: %08x, bitmaps[%d]: %08x\n", id, mask, idx, s->bitmaps[idx]);
	retries=5;
	while(retries--) {
		if ((s->bitmaps[idx] & mask) == 0) {
			dprintf(5,"bit not set, sleeping...\n");
			sleep(1);
			continue;
		}
		sprintf(what,"%03x", id);
//		if (debug >= 5) bindump(what,&s->frames[id],sizeof(struct can_frame));
		len = (datasz > 8 ? 8 : datasz);
		memcpy(data,s->frames[id].data,len);
		return 0;
	}
	return 1;
}

/* Func for can data that is remote (dont use thread/messages) */
static int si_can_get_remote(si_session_t *s, int id, uint8_t *data, int datasz) {
	int retries,bytes,len;
	struct can_frame frame;

	dprintf(5,"id: %03x, data: %p, data_len: %d\n", id, data, datasz);

	retries=5;
	dprintf(5,"s->can: %p\n", s->can);
	while(retries--) {
		bytes = s->can->read(s->can_handle,&frame,id);
		dprintf(5,"bytes: %d\n", bytes);
		if (bytes < 0) return 1;
		if (bytes == sizeof(frame)) {
			len = (frame.can_dlc > datasz ? datasz : frame.can_dlc);
			memcpy(data,&frame.data,len);
//			if (debug >= 7) bindump("FROM SERVER",data,len);
			break;
		}
		sleep(1);
	}
	dprintf(5,"returning: %d\n", (retries > 0 ? 0 : 1));
	return (retries > 0 ? 0 : 1);
}

int si_can_set_reader(si_session_t *s) {
	/* Start background recv thread */
	dprintf(1,"driver name: %s\n", s->can->name);
	if (strcmp(s->can->name,"can") == 0) {
		pthread_attr_t attr;
		
		/* Create a detached thread */
		if (pthread_attr_init(&attr)) {
			sprintf(s->errmsg,"pthread_attr_init: %s",strerror(errno));
			goto _can_init_error;
		}
		if (pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_DETACHED)) {
			sprintf(s->errmsg,"pthread_attr_setdetachstate: %s",strerror(errno));
			goto _can_init_error;
		}
		solard_set_state(s,SI_STATE_RUNNING);
		if (pthread_create(&s->th,&attr,&si_can_recv_thread,s)) {
			sprintf(s->errmsg,"pthread_create: %s",strerror(errno));
			goto _can_init_error;
		}
		pthread_attr_destroy(&attr);
		dprintf(1,"setting func to local data\n");
		s->can_read = si_can_get_local;
	} else {
		dprintf(1,"setting func to remote data\n");
		s->can_read = si_can_get_remote;
	}
	return 0;
_can_init_error:
	s->can_read = si_can_get_remote;
	return 1;
}

/* Function cannot return error */
int si_can_init(si_session_t *s) {

	/* Stop the read thread and wait for it to exit */
	dprintf(1,"SI_STATE_RUNNING: %d\n", solard_check_state(s,SI_STATE_RUNNING))
	if (solard_check_state(s,SI_STATE_RUNNING)) {
		solard_clear_state(s,SI_STATE_RUNNING);
		sleep(1);
	}

	/* If it's open, close it */
	if (solard_check_state(s,SI_STATE_OPEN)) si_driver.close(s);

	s->can_connected = 0;

	/* Find the driver */
	s->can = find_driver(si_transports,s->can_transport);
	dprintf(1,"s->can: %p\n", s->can);
	if (!s->can) {
		 if (strlen(s->can_transport)) {
			sprintf(s->errmsg,"unable to find CAN driver for transport: %s",s->can_transport);
			return 1;
		} else {
			sprintf(s->errmsg,"can_transport is empty!\n");
			return 1;
		}
	}

	/* Create new instance */
	dprintf(1,"using driver: %s\n", s->can->name);
	s->can_handle = s->can->new(0, s->can_target, s->can_topts);
	dprintf(1,"can_handle: %p\n", s->can_handle);
	if (!s->can_handle) {
		sprintf(s->errmsg,"unable to create new instance of %s driver: %s",
			s->can->name,strerror(errno));
		return 1;
	}
	if (si_can_set_reader(s)) return 1;

	/* Open it */
	if (si_driver.open(s)) {
		sprintf(s->errmsg,"error opening can driver");
		return 1;
	}

	/* OK, we're connected */
	s->can_connected = 1;
	return 0;
}

#ifdef DEBUG
static void _dump_bits(char *label, uint8_t bits) {
	register uint8_t i,mask;
	char bitstr[9];

	i = 0;
	mask = 1;
	while(mask) {
		bitstr[i++] = (bits & mask ? '1' : '0');
		mask <<= 1;
	}
	bitstr[i] = 0;
	dprintf(5,"%s(%02x): %s\n",label,bits,bitstr);
}
#define dump_bits(l,b) if (debug >= 5) _dump_bits(l,b)
#else
#define dump_bits(l,b) /* noop */
#endif

#define SET(m,b) { s->data.m = ((bits & b) != 0); dprintf(4,"%s: %d\n",#m,s->data.m); }

int si_can_read_relays(si_session_t *s) {
	uint8_t data[8],bits;

	/* 0x307 Relay state / Relay function bit 1 / Relay function bit 2 / Synch-Bits */
	s->can_read(s,0x307,data,8);
	/* 0-1 Relay State */
	bits = data[0];
	dump_bits("data[0]",bits);
	SET(relay1,	0x01);
	SET(relay2,	0x02);
	SET(s1_relay1,	0x04);
	SET(s1_relay2,	0x08);
	SET(s2_relay1,	0x10);
	SET(s2_relay2,	0x20);
	SET(s3_relay1,	0x40);
	SET(s3_relay2,	0x80);
	bits = data[1];
	dump_bits("data[1]",bits);
	SET(GnRn,	0x01);
	SET(s1_GnRn,	0x02);
	SET(s2_GnRn,	0x04);
	SET(s3_GnRn,	0x08);

	s->data.GnOn = (s->data.GnRn && s->data.AutoGn && s->data.ac1_frequency > 45.0) ? 1 : 0;
	dprintf(4,"GnOn: %d\n",s->data.GnOn);

	/* 2-3 Relay function bit 1 */
	bits = data[2];
	dump_bits("data[2]",bits);
	SET(AutoGn,	0x01);
	SET(AutoLodExt,	0x02);
	SET(AutoLodSoc,	0x04);
	SET(Tm1,	0x08);
	SET(Tm2,	0x10);
	SET(ExtPwrDer,	0x20);
	SET(ExtVfOk,	0x40);
	SET(GdOn,	0x80);
	bits = data[3];
	dump_bits("data[3]",bits);
	SET(Error,	0x01);
	SET(Run,	0x02);
	SET(BatFan,	0x04);
	SET(AcdCir,	0x08);
	SET(MccBatFan,	0x10);
	SET(MccAutoLod,	0x20);
	SET(Chp,	0x40);
	SET(ChpAdd,	0x80);

	/* 4 & 5 Relay function bit 2 */
	bits = data[4];
	dump_bits("data[4]",bits);
	SET(SiComRemote,0x01);
	SET(OverLoad,	0x02);
	bits = data[5];
	dump_bits("data[5]",bits);
	SET(ExtSrcConn,	0x01);
	SET(Silent,	0x02);
	SET(Current,	0x04);
	SET(FeedSelfC,	0x08);
	SET(Esave,	0x10);

	/* 6 Sync-bits */
	/* 7 unk */

	return 0;
}

int si_can_read_data(si_session_t *s) {
	uint8_t data[8];

	/* x300 Active power grid/gen */
	if (s->can_read(s,0x300,data,8)) return 1;
	s->data.active_grid_l1 = _getshort(&data[0]) * 100.0;
	s->data.active_grid_l2 = _getshort(&data[2]) * 100.0;
	s->data.active_grid_l3 = _getshort(&data[4]) * 100.0;
	dprintf(4,"active_grid_ l1: %.1f, l2: %.1f, l3: %.1f\n", s->data.active_grid_l1, s->data.active_grid_l2, s->data.active_grid_l3);

	/* x301 Active power Sunny Island */
	if (s->can_read(s,0x301,data,8)) return 1;
	s->data.active_si_l1 = _getshort(&data[0]) * 100.0;
	s->data.active_si_l2 = _getshort(&data[2]) * 100.0;
	s->data.active_si_l3 = _getshort(&data[4]) * 100.0;
	dprintf(4,"active_si_ l1: %.1f, l2: %.1f, l3: %.1f\n", s->data.active_si_l1, s->data.active_si_l2, s->data.active_si_l3);

	/* x302 Reactive power grid/gen */
	if (s->can_read(s,0x302,data,8)) return 1;
	s->data.reactive_grid_l1 = _getshort(&data[0]) * 100.0;
	s->data.reactive_grid_l2 = _getshort(&data[2]) * 100.0;
	s->data.reactive_grid_l3 = _getshort(&data[4]) * 100.0;
	dprintf(4,"reactive_grid_ l1: %.1f, l2: %.1f, l3: %.1f\n", s->data.reactive_grid_l1, s->data.reactive_grid_l2, s->data.reactive_grid_l3);

	/* x303 Reactive power Sunny Island */
	if (s->can_read(s,0x303,data,8)) return 1;
	s->data.reactive_si_l1 = _getshort(&data[0]) * 100.0;
	s->data.reactive_si_l2 = _getshort(&data[2]) * 100.0;
	s->data.reactive_si_l3 = _getshort(&data[4]) * 100.0;
	dprintf(4,"reactive_si_ l1: %.1f, l2: %.1f, l3: %.1f\n", s->data.reactive_si_l1, s->data.reactive_si_l2, s->data.reactive_si_l3);

	/* 0x304 OutputVoltage - L1 / L2 / L3 / Output Freq */
	if (s->can_read(s,0x304,data,8)) return 1;
	s->data.ac1_voltage_l1 = _getshort(&data[0]) / 10.0;
	s->data.ac1_voltage_l2 = _getshort(&data[2]) / 10.0;
	s->data.ac1_voltage_l3 = _getshort(&data[4]) / 10.0;
	s->data.ac1_frequency = _getshort(&data[6]) / 100.0;
	dprintf(4,"volt: l1: %.1f, l2: %.1f, l3: %.1f\n", s->data.ac1_voltage_l1, s->data.ac1_voltage_l2, s->data.ac1_voltage_l3);
	s->data.ac1_voltage = s->data.ac1_voltage_l1 + s->data.ac1_voltage_l2;
	dprintf(4,"ac1_voltage: %.1f, ac1_frequency: %.1f\n",s->data.ac1_voltage,s->data.ac1_frequency);

	/* 0x305 Battery voltage Battery current Battery temperature SOC battery */
	if (s->can_read(s,0x305,data,8)) return 1;
	s->data.battery_voltage = _getshort(&data[0]) / 10.0;
	s->data.battery_current = _getshort(&data[2]) / 10.0;
	if (s->charge_mode)
		s->data.battery_power = s->charge_voltage * s->data.battery_current;
	else
		s->data.battery_power = s->data.battery_voltage * s->data.battery_current;
	s->data.battery_temp = _getshort(&data[4]) / 10.0;
	s->data.battery_soc = _getshort(&data[6]) / 10.0;
	dprintf(4,"battery_voltage: %.1f\n", s->data.battery_voltage);
	dprintf(4,"battery_current: %.1f\n", s->data.battery_current);
	dprintf(4,"battery_temp: %.1f\n", s->data.battery_temp);
	dprintf(4,"battery_soc: %.1f\n", s->data.battery_soc);

	/* 0x306 SOH battery / Charging procedure / Operating state / active Error Message / Battery Charge Voltage Set-point */
	s->can_read(s,0x306,data,8);
	s->data.battery_soh = _getshort(&data[0]);
	s->data.charging_proc = data[2];
	s->data.state = data[3];
	s->data.errmsg = _getshort(&data[4]);
	s->data.battery_cvsp = _getshort(&data[6]) / 10.0;
	dprintf(4,"battery_soh: %.1f, charging_proc: %d, state: %d, errmsg: %d, battery_cvsp: %.1f\n",
		s->data.battery_soh, s->data.charging_proc, s->data.state, s->data.errmsg, s->data.battery_cvsp);

	if (si_can_read_relays(s)) return 1;

	/* 0x308 TotLodPwr */
	if (s->can_read(s,0x308,data,8)) return 1;
	s->data.TotLodPwr = _getshort(&data[0]) * 100.0;
	dprintf(4,"TotLodPwr: %.1f\n", s->data.TotLodPwr);

	/* 0x309 AC2 Voltage L1 / AC2 Voltage L2 / AC2 Voltage L3 / AC2 Frequency */
	if (s->can_read(s,0x309,data,8)) return 1;
	s->data.ac2_voltage_l1 = _getshort(&data[0]) / 10.0;
	s->data.ac2_voltage_l2 = _getshort(&data[2]) / 10.0;
	s->data.ac2_voltage_l3 = _getshort(&data[4]) / 10.0;
	s->data.ac2_frequency = _getshort(&data[6]) / 100.0;
	dprintf(4,"ac2: l1: %.1f, l2: %.1f, l3: %.1f\n",s->data.ac2_voltage_l1, s->data.ac2_voltage_l2, s->data.ac2_voltage_l3);
	s->data.ac2_voltage = s->data.ac2_voltage_l1 + s->data.ac2_voltage_l2;
	dprintf(4,"ac2: voltage: %.1f, frequency: %.1f\n",s->data.ac2_voltage,s->data.ac2_frequency);

	/* 0x30A PVPwrAt / GdCsmpPwrAt / GdFeedPwr */
	s->can_read(s,0x30a,data,8);
	s->data.PVPwrAt = _getshort(&data[0]) / 10.0;
	s->data.GdCsmpPwrAt = _getshort(&data[0]) / 10.0;
	s->data.GdFeedPwr = _getshort(&data[0]) / 10.0;
	dprintf(4,"PVPwrAt: %.1f\n", s->data.PVPwrAt);
	dprintf(4,"GdCsmpPwrAt: %.1f\n", s->data.GdCsmpPwrAt);
	dprintf(4,"GdFeedPwr: %.1f\n", s->data.GdFeedPwr);

	/* This is wrong ... */
	if (s->input_current_type == CURRENT_SOURCE_CALCULATED) {
//		s->data.ac2_power = s->data.active_grid_l1 + s->data.active_grid_l2 + s->data.active_grid_l3;
		s->data.ac2_power = s->data.active_grid_l1 + s->data.active_grid_l2;
//		s->data.ac2_power += (s->data.reactive_grid_l1*(-1)) + (s->data.reactive_grid_l2 *(-1)) + (s->data.reactive_grid_l3 *(-1));
		s->data.ac2_power += (s->data.reactive_grid_l1*(-1)) + (s->data.reactive_grid_l2 *(-1));
		s->data.ac2_current = s->data.ac2_power / s->data.ac2_voltage;
	}

	if (s->output_current_type == CURRENT_SOURCE_CALCULATED) {
//		s->data.ac1_power = s->data.active_si_l1 + s->data.active_si_l2 + s->data.active_si_l3;
		s->data.ac1_power = s->data.active_si_l1 + s->data.active_si_l2;
//		s->data.ac1_power += (s->data.reactive_si_l1*(-1)) + (s->data.reactive_si_l2 *(-1)) + (s->data.reactive_si_l3 *(-1));
		s->data.ac1_power += (s->data.reactive_si_l1*(-1)) + (s->data.reactive_si_l2 *(-1));
		s->data.ac1_power *= (-1);
		s->data.ac1_current = s->data.ac1_power / s->data.ac1_voltage;
	}

	return 0;
}

int si_can_write(si_session_t *s, int id, uint8_t *data, int data_len) {
	struct can_frame frame;
	int len,bytes;

	dprintf(0,"id: %03x, data: %p, data_len: %d\n",id,data,data_len);

	if (!data_len) return 0;

	/* No write!!! */
	dprintf(0,"readonly: %d\n", s->readonly);
	if (s->readonly) return 0;
	bindump("data",data,data_len);
	dprintf(0,"*** NOT WRITING ***\n");
	return data_len;

	len = data_len > 8 ? 8 : data_len;

	memset(&frame,0,sizeof(frame));
	frame.can_id = id;
	frame.can_dlc = len;
	memcpy(&frame.data,data,len);
//	bindump("write data",data,data_len);
	bytes = s->can->write(s->can_handle,&frame,sizeof(frame));
	dprintf(6,"bytes: %d\n", bytes);
	return bytes;
}

int si_can_write_va(si_session_t *s) {
	uint8_t data[8];
	float cv,ca;

	/* 0x351 Battery charge voltage / DC charge current limitation / DC discharge current limitation / discharge voltage */
	dprintf(1,"0x351: charge_voltage: %3.2f, charge_amps: %3.2f, min_voltage: %3.2f, discharge_amps: %3.2f\n",
		s->charge_voltage, s->charge_amps, s->min_voltage, s->discharge_amps);
        if (s->charge_mode) {
                cv = s->charge_voltage;
                if (s->data.GdOn) {
                        if (s->force_charge) ca = s->grid_charge_amps;
                        else ca = s->charge_min_amps;
                } else if (s->data.GnRn) {
                        ca = s->gen_charge_amps;
                } else {
                        cv += 1.0;
                        ca = s->charge_amps;
                }

		/* Apply modifiers to charge amps */
		dprintf(5,"cv: %.1f\n", cv);
		dprintf(6,"ca(1): %.1f\n", ca);
		ca *= s->charge_amps_temp_modifier;
		dprintf(6,"ca(2): %.1f\n", ca);
		ca *= s->charge_amps_soc_modifier;
		dprintf(5,"ca(3): %.1f\n", ca);
        } else {
                /* Always keep the CSVP = battery_voltage so it doesnt keep pushing power to the batts */
                /* XXX keeping the CSVP < battery (even - 0.1) will always draw from batt and turn grid off */
                /* XXX need max_charge_amps = never go above */
                /* Add solar output to charge amps? */
                if (s->data.GdOn | s->data.GnRn) cv = s->data.battery_voltage;
                else cv = s->data.battery_voltage + 1.0;
                ca = s->charge_amps;
        }

#if 0
	if (s->charge_mode) {
		cv = s->charge_voltage;
		if (s->data.GdOn) {
			if (s->charge_from_grid || s->force_charge) ca = s->grid_charge_amps;
			else ca = s->charge_min_amps;
		} else if (s->data.GnOn) {
			ca = s->gen_charge_amps;
		} else {
			cv += 1.0;
			ca = s->std_charge_amps;
		}

		/* Apply modifiers to charge amps */
		dprintf(5,"cv: %.1f\n", cv);
		dprintf(6,"ca(1): %.1f\n", ca);
		ca *= s->charge_amps_temp_modifier;
		dprintf(6,"ca(2): %.1f\n", ca);
		ca *= s->charge_amps_soc_modifier;
		dprintf(5,"ca(3): %.1f\n", ca);
	} else {
		/* make sure no charge goes into batts */
		cv = s->data.battery_voltage - 0.1;
		ca = 0.0;
	}
#endif

#if 0
/* XXX I have seen the system go from 350 charge_amps to 0 in 1 second and not alarm */
Battery charging current limitation: This is solely the limit for the charging current sent to Sunny Island. It is not to be
understood as a set-point and cannot be required from the battery. The available charging current is calculated by other
application dependent algorithm and cannot be set by the battery. Note that the actual charging current is not constant
but changes according to the algorithm. Battery charging current limit is the allowed, typical or rated charging current
value for the whole battery pack, which must not be exceeded during charging. The changes in charging current limit
sent by an external BMS should be ramped to not more than 10 A/sec. Please note that Sunny Island control algorithm
cannot immediately follow the changes, so that the alarm activation (in case that the limit is exceeded) must be slower
than a ramping down period.

	dprintf(1,"ca: %f, last_ca: %f\n", ca, s->last_ca);
	if (ca > s->last_ca + 10) {
		ca = s->last_ca + 10;
	} else if (ca < s->last_ca - 10) {
		ca = s->last_ca - 10;
	}
	s->last_ca = ca;
#endif
	si_putshort(&data[0],(cv * 10.0));
	si_putshort(&data[2],(ca * 10.0));
	si_putshort(&data[4],(s->discharge_amps * 10.0));
	si_putshort(&data[6],(s->min_voltage * 10.0));
	return (si_can_write(s,0x351,data,8) < 0);
}

int si_can_write_data(si_session_t *s) {
	uint8_t data[8];
	float soc;

	/* Startup SOC/Charge amps */
	dprintf(5,"startup: %d\n", s->startup);
	if (s->startup == 1) {
		soc = 99.9;
		s->charge_amps = 1;
		s->startup = 2;
	} else if (s->startup == 2) {
		soc = s->soc;
		s->charge_amps = 0;
		charge_control(s,s->startup_charge_mode,1);
		s->startup = 0;
	} else {
		soc = s->soc;
		charge_check(s);
	}

	/* Take charge amps to 0.0? need to go to 1 first then 0.0 next pass */
	dprintf(5,"tozero: %d, charge_amps: %.1f\n", s->tozero, s->charge_amps);
	if (s->tozero == 1) {
		if ((int)s->charge_amps == 1) {
			dprintf(5,"setting charge amps to 0...\n");
			s->charge_amps = 0.0;
			s->tozero = 2;
		} else {
			s->tozero = 0;
		}
	} else {
		if ((int)s->charge_amps == 0) {
			if (s->tozero != 2) {
				s->charge_amps = 1;
				s->tozero = 1;
			}
		} else {
			s->tozero = 0;
		}
	}

	/* 0x350 Active Current Set-point / Reactive current Set-Point */
	
	if (si_can_write_va(s)) return 1;

	/* 0x352 Nominal frequency (F0) */
	/* 0x353 Nominal voltage L1 (U0) */
	/* 0x354 Function Bits SiCom V1 */

	/* 0x355 SOC value / SOH value / HiResSOC */
	dprintf(1,"0x355: SoC: %.1f, SoH: %.1f\n", soc, s->soh);
	si_putshort(&data[0],soc);
	si_putshort(&data[2],s->soh);
	si_putlong(&data[4],(soc * 100.0));
	if (si_can_write(s,0x355,data,8) < 0) return 1;

#if 0
	/* 0x356 Battery Voltage / Battery Current / Battery Temperature */
	dprintf(1,"0x356: battery_voltage: %3.2f, battery_amps: %3.2f, battery_temp: %3.2f\n",
		s->battery_voltage, s->battery_amps, s->battery_temp);
	si_putshort(&data[0],s->battery_voltage * 100.0);
	si_putshort(&data[2],s->battery_current * 10.0);
	if (s->have_battery_temp) {
		si_putshort(&data[4],s->battery_temp * 10.0);
	} else {
		si_putshort(&data[4],0);
	}
	if (si_can_write(s,0x356,data,8) < 0) return 1;
#endif

	/* 0x357 - 0x359 ?? */

#if 0
	/* 0x35A Alarms / Warnings */
	memset(data,0,sizeof(data));
	if (si_can_write(s,0x35A,data,8) < 0) return 1;

	/* 0x35B Events */
	memset(data,0,sizeof(data));
	if (si_can_write(s,0x35B,data,8) < 0) return 1;
#endif

	/* 0x35C Function Bits / Function Bits Relays */

#if 0
	/* 0x35E Manufacturer-Name-ASCII (8 chars) */
	memset(data,0x20,sizeof(data));
	data[0] = 0x53; data[1] = 0x50; data[2] = 0x53;
	if (si_can_write(s,0x35E,data,8) < 0) return 1;

	/* 0x35F - Bat-Type / BMS Version / Bat-Capacity / reserved Manufacturer ID */
	si_putshort(&data[0],1);
	/* major.minor.build.revision - encoded as MmBBr 10142 = 1.0.14.2 */
	si_putshort(&data[2],10000);
	si_putshort(&data[4],2500);
	si_putshort(&data[6],1);
	if (si_can_write(s,0x35F,data,8) < 0) return 1;
#endif

	/* 0x360 Limitation of external current (Gen/Grid) */

	return 0;
}

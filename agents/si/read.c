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
//#include <math.h>

void *si_recv_thread(void *handle) {
	si_session_t *s = handle;
	struct can_frame frame;
	int bytes;
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
			memset(&s->bitmap,0,sizeof(s->bitmap));
			sleep(1);
			continue;
		}
		bytes = s->can->read(s->can_handle,&frame,0xffff);
		dprintf(8,"bytes: %d\n", bytes);
		if (bytes < 1) {
			memset(&s->bitmap,0,sizeof(s->bitmap));
			sleep(1);
			continue;
		}
		dprintf(8,"frame.can_id: %03x\n",frame.can_id);
		if (frame.can_id < 0x300 || frame.can_id > 0x30F) continue;
//		bindump("frame",&frame,sizeof(frame));
		memcpy(&s->frames[frame.can_id - 0x300],&frame,sizeof(frame));
		s->bitmap |= 1 << (frame.can_id - 0x300);
	}
	dprintf(1,"returning!\n");
	return 0;
}

int si_get_local_can_data(si_session_t *s, int id, uint8_t *data, int datasz) {
	char what[16];
	uint16_t mask;
	int idx,retries,len;

	dprintf(5,"id: %03x, data: %p, len: %d\n", id, data, datasz);

	idx = id - 0x300;
	mask = 1 << idx;
	dprintf(5,"mask: %04x, bitmap: %04x\n", mask, s->bitmap);
	retries=5;
	while(retries--) {
		if ((s->bitmap & mask) == 0) {
			dprintf(5,"bit not set, sleeping...\n");
			sleep(1);
			continue;
		}
		sprintf(what,"%03x", id);
//		if (debug >= 5) bindump(what,&s->frames[idx],sizeof(struct can_frame));
		len = (datasz > 8 ? 8 : datasz);
		memcpy(data,s->frames[idx].data,len);
		return 0;
	}
	return 1;
}

/* Func for can data that is remote (dont use thread/messages) */
int si_get_remote_can_data(si_session_t *s, int id, uint8_t *data, int datasz) {
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

#ifdef DEBUG
void _dump_bits(char *label, uint8_t bits) {
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

#define _getshort(v) (short)( ((v)[1]) << 8 | ((v)[0]) )
#define SET(m,b) { s->info.m = ((bits & b) != 0); dprintf(4,"%s: %d\n",#m,s->info.m); }

int si_get_bits(si_session_t *s) {
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

	s->info.GnOn = (s->info.GnRn && s->info.AutoGn && s->info.frequency > 45.0) ? 1 : 0;
	dprintf(4,"GnOn: %d\n",s->info.GnOn);

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

int si_read(si_session_t *s, void *buf, int buflen) {
	uint8_t data[8];

	/* x300 Active power grid/gen */
	if (s->can_read(s,0x300,data,8)) return 1;
	s->info.active.grid.l1 = _getshort(&data[0]) * 100.0;
	s->info.active.grid.l2 = _getshort(&data[2]) * 100.0;
	s->info.active.grid.l3 = _getshort(&data[4]) * 100.0;
	dprintf(4,"active grid: l1: %.1f, l2: %.1f, l3: %.1f\n", s->info.active.grid.l1, s->info.active.grid.l2, s->info.active.grid.l3);

	/* x301 Active power Sunny Island */
	if (s->can_read(s,0x301,data,8)) return 1;
	s->info.active.si.l1 = _getshort(&data[0]) * 100.0;
	s->info.active.si.l2 = _getshort(&data[2]) * 100.0;
	s->info.active.si.l3 = _getshort(&data[4]) * 100.0;
	dprintf(4,"active si: l1: %.1f, l2: %.1f, l3: %.1f\n", s->info.active.si.l1, s->info.active.si.l2, s->info.active.si.l3);

	/* x302 Reactive power grid/gen */
	if (s->can_read(s,0x302,data,8)) return 1;
	s->info.reactive.grid.l1 = _getshort(&data[0]) * 100.0;
	s->info.reactive.grid.l2 = _getshort(&data[2]) * 100.0;
	s->info.reactive.grid.l3 = _getshort(&data[4]) * 100.0;
	dprintf(4,"reactive grid: l1: %.1f, l2: %.1f, l3: %.1f\n", s->info.reactive.grid.l1, s->info.reactive.grid.l2, s->info.reactive.grid.l3);

	/* x303 Reactive power Sunny Island */
	if (s->can_read(s,0x303,data,8)) return 1;
	s->info.reactive.si.l1 = _getshort(&data[0]) * 100.0;
	s->info.reactive.si.l2 = _getshort(&data[2]) * 100.0;
	s->info.reactive.si.l3 = _getshort(&data[4]) * 100.0;
	dprintf(4,"reactive si: l1: %.1f, l2: %.1f, l3: %.1f\n", s->info.reactive.si.l1, s->info.reactive.si.l2, s->info.reactive.si.l3);

	s->info.ac1_current = s->info.active.si.l1 + s->info.active.si.l2 + s->info.active.si.l3;
	s->info.ac1_current += (s->info.reactive.si.l1*(-1)) + (s->info.reactive.si.l2 *(-1)) + (s->info.reactive.si.l2 *(-1));

	s->info.ac2_current = s->info.active.grid.l1 + s->info.active.grid.l2 + s->info.active.grid.l3;
	s->info.ac2_current += (s->info.reactive.grid.l1*(-1)) + (s->info.reactive.grid.l2 *(-1)) + (s->info.reactive.grid.l2 *(-1));

	/* 0x304 OutputVoltage - L1 / L2 / L3 / Output Freq */
	if (s->can_read(s,0x304,data,8)) return 1;
	s->info.volt.l1 = _getshort(&data[0]) / 10.0;
	s->info.volt.l2 = _getshort(&data[2]) / 10.0;
	s->info.volt.l3 = _getshort(&data[4]) / 10.0;
	s->info.frequency = _getshort(&data[6]) / 100.0;
	dprintf(4,"volt: l1: %.1f, l2: %.1f, l3: %.1f\n", s->info.volt.l1, s->info.volt.l2, s->info.volt.l3);
	s->info.voltage = s->info.volt.l1 + s->info.volt.l2;
	dprintf(4,"voltage: %.1f, frequency: %.1f\n",s->info.voltage,s->info.frequency);

	/* 0x305 Battery voltage Battery current Battery temperature SOC battery */
	if (s->can_read(s,0x305,data,8)) return 1;
	s->info.battery_voltage = _getshort(&data[0]) / 10.0;
	s->info.battery_current = _getshort(&data[2]) / 10.0;
	s->info.battery_temp = _getshort(&data[4]) / 10.0;
	s->info.battery_soc = _getshort(&data[6]) / 10.0;
	dprintf(4,"battery_voltage: %.1f\n", s->info.battery_voltage);
	dprintf(4,"battery_current: %.1f\n", s->info.battery_current);
	dprintf(4,"battery_temp: %.1f\n", s->info.battery_temp);
	dprintf(4,"battery_soc: %.1f\n", s->info.battery_soc);

	/* 0x306 SOH battery / Charging procedure / Operating state / active Error Message / Battery Charge Voltage Set-point */
	s->can_read(s,0x306,data,8);
	s->info.battery_soh = _getshort(&data[0]);
	s->info.charging_proc = data[2];
	s->info.state = data[3];
	s->info.errmsg = _getshort(&data[4]);
	s->info.battery_cvsp = _getshort(&data[6]) / 10.0;
	dprintf(4,"battery_soh: %.1f, charging_proc: %d, state: %d, errmsg: %d, battery_cvsp: %.1f\n",
		s->info.battery_soh, s->info.charging_proc, s->info.state, s->info.errmsg, s->info.battery_cvsp);

#ifndef SI_CB
	if (si_get_bits(s)) return 1;
#endif

	/* 0x308 TotLodPwr */
	if (s->can_read(s,0x308,data,8)) return 1;
	s->info.TotLodPwr = _getshort(&data[0]) * 100.0;
	dprintf(4,"TotLodPwr: %.1f\n", s->info.TotLodPwr);

	/* 0x309 AC2 Voltage L1 / AC2 Voltage L2 / AC2 Voltage L3 / AC2 Frequency */
	if (s->can_read(s,0x309,data,8)) return 1;
	s->info.ac2_volt.l1 = _getshort(&data[0]) / 10.0;
	s->info.ac2_volt.l2 = _getshort(&data[2]) / 10.0;
	s->info.ac2_volt.l3 = _getshort(&data[4]) / 10.0;
	s->info.ac2_frequency = _getshort(&data[6]) / 100.0;
	dprintf(4,"ac2: l1: %.1f, l2: %.1f, l3: %.1f\n",s->info.ac2_volt.l1, s->info.ac2_volt.l2, s->info.ac2_volt.l3);
	s->info.ac2_voltage = s->info.ac2_volt.l1 + s->info.ac2_volt.l2;
	dprintf(4,"ac2: voltage: %.1f, frequency: %.1f\n",s->info.ac2_voltage,s->info.ac2_frequency);

	/* 0x30A PVPwrAt / GdCsmpPwrAt / GdFeedPwr */
	s->can_read(s,0x30a,data,8);
	s->info.PVPwrAt = _getshort(&data[0]) / 10.0;
	s->info.GdCsmpPwrAt = _getshort(&data[0]) / 10.0;
	s->info.GdFeedPwr = _getshort(&data[0]) / 10.0;
	dprintf(4,"PVPwrAt: %.1f\n", s->info.PVPwrAt);
	dprintf(4,"GdCsmpPwrAt: %.1f\n", s->info.GdCsmpPwrAt);
	dprintf(4,"GdFeedPwr: %.1f\n", s->info.GdFeedPwr);

	if ((unsigned long)buf == 0xDEADBEEF) return 0;

	if (s->info.GdOn != s->grid_connected) {
		s->grid_connected = s->info.GdOn;
		log_info("Grid %s\n",(s->grid_connected ? "connected" : "disconnected"));
	}
	if (s->info.GnOn != s->gen_connected) {
		s->gen_connected = s->info.GnOn;
		log_info("Generator %s\n",(s->gen_connected ? "connected" : "disconnected"));
	}

	/* Sim? */
	if (s->sim) {
		if (s->startup == 1) {
			s->tvolt = s->charge_start_voltage + 4.0;
			s->sim_amps = -50;
		}
		else if (s->charge_mode == 0) s->info.battery_voltage = (s->tvolt -= 0.8);
		else if (s->charge_mode == 1) s->info.battery_voltage = (s->tvolt += 2.4);
		else if (s->charge_mode == 2) {
			s->info.battery_voltage = s->charge_end_voltage;
			if (s->cv_method == 0) {
				s->cv_start_time -= 1800;
			} else if (s->cv_method == 1) {
				s->sim_amps += 8;
				if (s->sim_amps >= 0) s->sim_amps = (s->cv_cutoff - 1.0) * -1;
				if (!s->gen_started && s->bafull) {
					s->sim_amps = 27;
					s->gen_started = 1;
				}
				s->info.battery_current = s->sim_amps;
			}
		}
	}

	/* Calculate SOC */
	if (!si_isvrange(s->min_voltage)) {
		log_warning("setting min_voltage to %.1f\n", 41.0);
		s->min_voltage = 41.0;
	}
	if (!si_isvrange(s->max_voltage)) {
		log_warning("setting max_voltage to %.1f\n", 58.4);
		s->max_voltage = 58.4;
	}
	/* Calc SOC if possible */
	dprintf(4,"user_soc: %.1f, battery_voltage: %.1f\n", s->user_soc, s->info.battery_voltage);
	if (s->user_soc < 0.0) {
		s->soc = ( ( s->info.battery_voltage - s->min_voltage) / (s->max_voltage - s->min_voltage) ) * 100.0;
	} else  {
		s->soc = s->user_soc;
	}
	/* Adjust SOC so GdSocEna and GnSocEna dont disconnect until we're done charging */
	dprintf(1,"grid_connected: %d, charge_mode: %d, grid_soc_stop: %.1f, soc: %.1f\n",
		s->grid_connected,s->charge_mode,s->grid_soc_stop,s->soc);
	dprintf(1,"gen_connected: %d, charge_mode: %d, gen_soc_stop: %.1f, soc: %.1f\n",
		s->gen_connected,s->charge_mode,s->gen_soc_stop,s->soc);
	if (s->grid_connected && s->charge_mode && s->grid_soc_stop > 1.0 && s->soc >= s->grid_soc_stop)
		s->soc = s->grid_soc_stop - 1;
	else if (s->gen_connected && s->charge_mode && s->gen_soc_stop > 1.0 && s->soc >= s->gen_soc_stop)
		s->soc = s->gen_soc_stop - 1;
	if (s->info.battery_voltage != s->last_battery_voltage || s->soc != s->last_soc) {
		log_info("%s%sBattery Voltage: %.1fV, SoC: %.1f%%\n",
			(s->info.Run ? "[Running] " : ""), (s->sim ? "(SIM) " : ""), s->info.battery_voltage, s->soc);
		s->last_battery_voltage = s->info.battery_voltage;
		s->last_soc = s->soc;
	}
	s->soh = 100.0;

	return 0;
}

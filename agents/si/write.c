
/*
Copyright (c) 2021, Stephen P. Shoecraft
All rights reserved.

This source code is licensed under the BSD-style license found in the
LICENSE file in the root directory of this source tree.
*/

#include "si.h"

int si_can_write(si_session_t *s, int id, uint8_t *data, int data_len) {
	struct can_frame frame;
	int len;

	dprintf(1,"id: %03x, data: %p, data_len: %d\n",id,data,data_len);

	len = data_len > 8 ? 8 : data_len;

	memset(&frame,0,sizeof(frame));
	frame.can_id = id;
	frame.can_dlc = len;
	memcpy(&frame.data,data,len);
//	bindump("write data",data,data_len);
	return s->can->write(s->can_handle,&frame,sizeof(frame));
}

int si_write(si_session_t *s, void *buf, int buflen) {
	solard_inverter_t *inv = buf;
	uint8_t data[8];
	float soc;

	/* Calculate SOC */
	dprintf(1,"startup: %d\n", s->startup);
	if (s->startup == 1) {
		soc = 99.9;
		/* Save a copy of charge_amps */
		dprintf(1,"**** SAVING CHARGE AMPS: %.1f\n", inv->charge_amps);
		s->save_charge_amps = inv->charge_amps;
		inv->charge_amps = 0.1;
		s->startup++;
	} else if (s->startup == 2) {
		dprintf(1,"**** RESTORING CHARGE AMPS: %.1f\n", s->save_charge_amps);
		inv->charge_amps = s->save_charge_amps;
		soc = inv->soc;
		charge_control(s,s->startup_charge_mode,1);
//		charge_start(s,1);
		s->startup++;
	} else {
		soc = inv->soc;
		charge_check(s);
	}

	dprintf(1,"0x351: charge_voltage: %3.2f, charge_amps: %3.2f, min_voltage: %3.2f, discharge_amps: %3.2f\n",
		s->charge_voltage, s->charge_amps, inv->min_voltage, inv->discharge_amps);
	si_putshort(&data[0],(s->charge_voltage * 10.0));
	si_putshort(&data[2],(s->charge_amps * 10.0));
	si_putshort(&data[4],(inv->discharge_amps * 10.0));
	si_putshort(&data[6],(inv->min_voltage * 10.0));
	if (si_can_write(s,0x351,data,8) < 0) return 1;

	dprintf(1,"0x355: SOC: %.1f, SOH: %.1f\n", soc, inv->soh);
	si_putshort(&data[0],soc);
	si_putlong(&data[4],(soc * 100.0));
	si_putshort(&data[2],inv->soh);
	if (si_can_write(s,0x355,data,8) < 0) return 1;

#if 0
	dprintf(1,"0x356: battery_voltage: %3.2f, battery_current: %3.2f, battery_temp: %3.2f\n",
		inv->battery_voltage, inv->battery_current, inv->battery_temp);
	si_putshort(&data[0],inv->battery_voltage * 100.0);
	si_putshort(&data[2],inv->battery_current * 10.0);
	si_putshort(&data[4],inv->battery_temp * 10.0);
	if (si_can_write(s,0x356,data,8) < 0) return 1;
#endif

	/* Alarms/Warnings */
	memset(data,0,sizeof(data));
//	if (si_can_write(s,0x35A,data,8) < 0) return 1;

	/* Events */
	memset(data,0,sizeof(data));
	if (si_can_write(s,0x35B,data,8) < 0) return 1;

#if 0
	/* MFG Name */
	memset(data,' ',sizeof(data));
#define MFG_NAME "SPS"
	memcpy(data,MFG_NAME,strlen(MFG_NAME));
	if (si_can_write(s,0x35E,data,8) < 0) return 1;

	/* 0x35F - Bat-Type / BMS Version / Bat-Capacity / reserved Manufacturer ID */
	si_putshort(&data[0],1);
	/* major.minor.build.revision - encoded as MmBBr 10142 = 1.0.14.2 */
	si_putshort(&data[2],10000);
	si_putshort(&data[4],inv->battery_capacity);
	si_putshort(&data[6],1);
	if (si_can_write(s,0x35F,data,8) < 0) return 1;
#endif

	solard_clear_state(s,SI_STATE_STARTUP);
	return 0;
}

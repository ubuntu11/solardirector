
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
		/* Charge amps must be non-zero before setting to 0 */
		inv->charge_amps = 0.1;
		s->startup++;
	} else if (s->startup == 2) {
		dprintf(1,"**** RESTORING CHARGE AMPS: %.1f\n", s->save_charge_amps);
		inv->charge_amps = s->save_charge_amps;
		soc = inv->soc;
		charge_control(s,s->startup_charge_mode,1);
		s->startup++;
	} else {
		soc = inv->soc;
		charge_check(s);
	}

	/* Take charge amps to 0.0? need to go to 0.1 first then 0.0 next pass */
	dprintf(1,"startup: %d, tozero: %d, charge_amps: %.1f\n", s->startup, s->tozero, s->charge_amps);
	if (s->tozero) {
		/* Is it still 0? */
		if (((int)s->charge_amps) == 0) {
			s->charge_amps = 0.0;
		} else {
			s->tozero = 0;
		}
	} else if (s->startup > 1 && ((int)s->charge_amps) == 0) {
		s->charge_amps = 0.1;
		s->tozero = 1;
	}

	/* 0x350 Active Current Set-point / Reactive current Set-Point */
	
	/* 0x351 Battery charge voltage / DC charge current limitation / DC discharge current limitation / discharge voltage */
	dprintf(1,"0x351: charge_voltage: %3.2f, charge_amps: %3.2f, min_voltage: %3.2f, discharge_amps: %3.2f\n",
		s->charge_voltage, s->charge_amps, inv->min_voltage, inv->discharge_amps);
	if (s->bits.GdOn) {
		si_putshort(&data[0],(s->charge_voltage * 10.0));
	} else {
		si_putshort(&data[0],((s->charge_voltage + 1.0) * 10.0));
	}
	si_putshort(&data[2],(s->charge_amps * 10.0));
	si_putshort(&data[4],(inv->discharge_amps * 10.0));
	si_putshort(&data[6],(inv->min_voltage * 10.0));
	if (si_can_write(s,0x351,data,8) < 0) return 1;

	/* 0x352 Nominal frequency (F0) */
	/* 0x353 Nominal voltage L1 (U0) */
	/* 0x354 Function Bits SiCom V1 */

	/* 0x355 SOC value / SOH value / HiResSOC */
	dprintf(1,"0x355: SOC: %.1f, SOH: %.1f\n", soc, inv->soh);
	si_putshort(&data[0],soc);
	si_putlong(&data[4],(soc * 100.0));
	si_putshort(&data[2],inv->soh);
	if (si_can_write(s,0x355,data,8) < 0) return 1;

#if 0
	/* 0x356 Battery Voltage / Battery Current / Battery Temperature */
	dprintf(1,"0x356: battery_voltage: %3.2f, battery_amps: %3.2f, battery_temp: %3.2f\n",
		inv->battery_voltage, inv->battery_amps, inv->battery_temp);
	si_putshort(&data[0],inv->battery_voltage * 100.0);
	si_putshort(&data[2],inv->battery_amps * 10.0);
	if (s->have_battery_temp) {
		si_putshort(&data[4],inv->battery_temp * 10.0);
	} else {
		si_putshort(&data[4],0);
	}
	if (si_can_write(s,0x356,data,8) < 0) return 1;
#endif

	/* 0x357 - 0x359 ?? */

	/* 0x35A Alarms / Warnings */
	memset(data,0,sizeof(data));
	if (si_can_write(s,0x35A,data,8) < 0) return 1;

	/* 0x35B Events */
	memset(data,0,sizeof(data));
	if (si_can_write(s,0x35B,data,8) < 0) return 1;

	/* 0x35C Function Bits / Function Bits Relais */

	/* 0x35E Manufacturer-Name-ASCII (8 chars) */
#define MFG_NAME "SHOECRAFT"
	memcpy(data,MFG_NAME,strlen(MFG_NAME));
	if (si_can_write(s,0x35E,data,8) < 0) return 1;

	/* 0x35F - Bat-Type / BMS Version / Bat-Capacity / reserved Manufacturer ID */
	si_putshort(&data[0],1);
	/* major.minor.build.revision - encoded as MmBBr 10142 = 1.0.14.2 */
	si_putshort(&data[2],10000);
	si_putshort(&data[4],2500);
	si_putshort(&data[6],1);
	if (si_can_write(s,0x35F,data,8) < 0) return 1;

#if 0
	/* 0x360 Limitation of external current (Gen/Grid) */
	si_putshort(&data[0],10);
	if (si_can_write(s,0x360,data,2) < 0) return 1;
#endif

	solard_clear_state(s,SI_STATE_STARTUP);
	return 0;
}


/*
Copyright (c) 2021, Stephen P. Shoecraft
All rights reserved.

This source code is licensed under the BSD-style license found in the
LICENSE file in the root directory of this source tree.
*/

#include "si.h"

static int write_data(si_session_t *s, int id, uint8_t *data, int data_len) {
	struct can_frame frame;
	int len;

	dprintf(1,"id: %03x, data: %p, data_len: %d\n",id,data,data_len);

	len = data_len > 8 ? 8 : data_len;

	frame.can_id = id;
	frame.can_dlc = len;
	memcpy(&frame.data,data,len);
	return s->can->write(s->can_handle,&frame,sizeof(frame));
}

int si_write(si_session_t *s, void *buf, int buflen) {
	solard_inverter_t *inv = buf;
	uint8_t data[8];
	float soc;

	dprintf(1,"charge_voltage: %.1f, discharge_voltage: %.1f\n", inv->charge_voltage, inv->discharge_voltage);
	if (!si_isvrange(inv->discharge_voltage) || !si_isvrange(inv->charge_voltage)) {
		log_write(LOG_ERROR,"charge_voltage and/or discharge_voltage not set\n");
		return 1;
	}

	/* Calculate SOC */
	if (solard_check_state(s,SI_STATE_STARTUP)) {
		soc = 99.9;
		/* Save a copy of charge_amps */
		s->save_charge_amps = inv->charge_amps;
		inv->charge_amps = 0.1;
	} else {
		soc = inv->soc >= 0.0 ? inv->soc : ( ( inv->battery_voltage - inv->discharge_voltage) / (inv->charge_voltage - inv->discharge_voltage) ) * 100.0;
		inv->charge_amps = s->save_charge_amps;
	}
        lprintf(0,"SoC: %.1f\n", soc);
        inv->soh = 100.0;

	dprintf(1,"0x351: charge_voltage: %3.2f, charge_amps: %3.2f, discharge_voltage: %3.2f, discharge_amps: %3.2f\n",
		inv->charge_voltage, inv->charge_amps, inv->discharge_voltage, inv->discharge_amps);
	si_putshort(&data[0],(inv->charge_voltage * 10.0));
	si_putshort(&data[2],(inv->charge_amps * 10.0));
	si_putshort(&data[4],(inv->discharge_amps * 10.0));
	si_putshort(&data[6],(inv->discharge_voltage * 10.0));
//	if (write_data(s,0x351,data,8) < 0) return 1;

	dprintf(1,"0x355: SOC: %.1f, SOH: %.1f\n", soc, inv->soh);
	si_putshort(&data[0],soc);
	si_putlong(&data[4],(soc * 100.0));
	si_putshort(&data[2],inv->soh);
//	if (write_data(s,0x355,data,8) < 0) return 1;

#if 0
	dprintf(1,"0x356: battery_voltage: %3.2f, battery_current: %3.2f, battery_temp: %3.2f\n",
		inv->battery_voltage, inv->battery_current, inv->battery_temp);
	si_putshort(&data[0],inv->battery_voltage * 100.0);
	si_putshort(&data[2],inv->battery_current * 10.0);
	si_putshort(&data[4],inv->battery_temp * 10.0);
	if (write_data(s,0x356,data,8) < 0) return 1;
#endif

	/* Alarms/Warnings */
	memset(data,0,sizeof(data));
//	if (write_data(s,0x35A,data,8) < 0) return 1;

	/* Events */
	memset(data,0,sizeof(data));
//	if (write_data(s,0x35B,data,8)) return 1;

#if 0
	/* MFG Name */
	memset(data,' ',sizeof(data));
#define MFG_NAME "RSW"
	memcpy(data,MFG_NAME,strlen(MFG_NAME));
	if (write_data(s,0x35E,data,8) < 0) return 1;

	/* 0x35F - Bat-Type / BMS Version / Bat-Capacity / reserved Manufacturer ID */
	si_putshort(&data[0],1);
	/* major.minor.build.revision - encoded as MmBBr 10142 = 1.0.14.2 */
	si_putshort(&data[2],10000);
	si_putshort(&data[4],inv->battery_capacity);
	si_putshort(&data[6],1);
//	if (write_data(s,0x35F,data,8) < 0) return 1;
#endif

	solard_clear_state(s,SI_STATE_STARTUP);
	return 1;
}

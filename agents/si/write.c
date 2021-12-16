
/*
Copyright (c) 2021, Stephen P. Shoecraft
All rights reserved.

This source code is licensed under the BSD-style license found in the
LICENSE file in the root directory of this source tree.
*/

#include "si.h"

#define si_putshort(p,v) { float tmp; *((p+1)) = ((int)(tmp = v) >> 8); *((p)) = (int)(tmp = v); }
#define si_putlong(p,v) { *((p+3)) = ((int)(v) >> 24); *((p)+2) = ((int)(v) >> 16); *((p+1)) = ((int)(v) >> 8); *((p)) = ((int)(v) & 0x0F); }

int si_can_write(si_session_t *s, int id, uint8_t *data, int data_len) {
	struct can_frame frame;
	int len,bytes;

	dprintf(5,"id: %03x, data: %p, data_len: %d\n",id,data,data_len);

	/* No write!!! */
	if (s->readonly) return 0;
	printf("si_can_write: *** NOT WRITING ***\n");
	return 0;


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

int si_write_va(si_session_t *s) {
	uint8_t data[8];
	float cv,ca;

	if (s->readonly) return 1;

	/* 0x351 Battery charge voltage / DC charge current limitation / DC discharge current limitation / discharge voltage */
	dprintf(1,"0x351: charge_voltage: %3.2f, charge_amps: %3.2f, min_voltage: %3.2f, discharge_amps: %3.2f\n",
		s->charge_voltage, s->charge_amps, s->min_voltage, s->discharge_amps);
	cv = s->charge_voltage;
	ca = s->charge_amps;
	if (s->info.GdOn) {
		if (s->charge_mode && (s->charge_from_grid || s->force_charge)) ca = s->grid_charge_amps;
	} else if (s->info.GnOn) {
		if (s->charge_mode) ca = s->gen_charge_amps;
	} else {
		cv = s->charge_voltage + 1.0;
	}
	/* Apply modifiers to charge amps */
	dprintf(5,"cv: %.1f\n", cv);
	dprintf(6,"ca(1): %.1f\n", ca);
	ca *= s->charge_amps_temp_modifier;
	dprintf(6,"ca(2): %.1f\n", ca);
	ca *= s->charge_amps_soc_modifier;
	dprintf(5,"ca(3): %.1f\n", ca);
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

int si_write(si_session_t *s, void *buf, int buflen) {
	uint8_t data[8];
	float soc;

	dprintf(1,"readonly: %d\n", s->readonly);
	if (s->readonly) return 1;

	/* Startup SOC/Charge amps */
	dprintf(5,"startup: %d\n", s->startup);
	if (s->startup == 1) {
		soc = 99.9;
		s->charge_amps = 0;
		s->startup = 2;
	} else if (s->startup == 2) {
		soc = s->soc;
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
	
	if (si_write_va(s)) return 1;

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
#endif

	/* 0x360 Limitation of external current (Gen/Grid) */

	return 0;
}

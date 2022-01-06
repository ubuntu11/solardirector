
/*
Copyright (c) 2021, Stephen P. Shoecraft
All rights reserved.

This source code is licensed under the BSD-style license found in the
LICENSE file in the root directory of this source tree.
*/

//printf("********************************* WRITE ***************************************\n");

include("charge.js");

//#define si_putshort(p,v) { float tmp; *((p+1)) = ((int)(tmp = v) >> 8); *((p)) = (int)(tmp = v); }
//#define si_putlong(p,v) { *((p+3)) = ((int)(v) >> 24); *((p)+2) = ((int)(v) >> 16); *((p+1)) = ((int)(v) >> 8); *((p)) = ((int)(v) & 0x0F); }

function si_write_va() {
	var cv, ca;

	if (si.readonly) return 1;

	/* 0x351 Battery charge voltage / DC charge current limitation / DC discharge current limitation / discharge voltage */
	dprintf(1,"0x351: charge_voltage: %3.2f, charge_amps: %3.2f, min_voltage: %3.2f, discharge_amps: %3.2f\n",
		si.charge_voltage, si.charge_amps, si.min_voltage, si.discharge_amps);
	dprintf(1,"charge_mode: %d\n", si.charge_mode);
	if (si.charge_mode) {
		cv = si.charge_voltage;
		dprintf(1,"GdOn: %d, GnOn: %d\n", si.info.GdOn, si.info.GnOn);
		if (si.info.GdOn) {
			if (si.charge_from_grid || si.force_charge) ca = si.grid_charge_amps;
			else ca = si.charge_min_amps;
		} else if (si.info.GnOn) {
			ca = si.gen_charge_amps;
		} else {
			cv += 1.0;
			dprintf(1,"std_charge_amps: %.1f\n", si.std_charge_amps);
			ca = si.std_charge_amps;
		}

		/* Apply modifiers to charge amps */
//		dprintf(1,"cv: %.1f\n", cv);
//		dprintf(1,"ca(1): %.1f\n", ca);
//		dprintf(1,"si.charge_amps_temp_modifier: %f, si.charge_amps_soc_modifier: %f\n",
//			si.charge_amps_temp_modifier, si.charge_amps_soc_modifier);
		ca *= si.charge_amps_temp_modifier;
//		dprintf(1,"ca(2): %.1f\n", ca);
		ca *= si.charge_amps_soc_modifier;
//		dprintf(1,"ca(3): %.1f\n", ca);
		if (!ca) ca = 0.0;
	} else {
		/* make sure no charge goes into batts */
		cv = si.info.battery_voltage - 0.1;
		ca = 0.0;
	}

	var data = [];
	dprintf(1,"cv: %.1f\n", cv);
	dprintf(1,"ca: %.1f\n", ca);
	putshort(data,0,cv * 10.0);
	putshort(data,2,ca * 10.0)
	putshort(data,4,si.discharge_amps * 10.0)
	putshort(data,6,si.min_voltage * 10.0);
	return (si.can_write(0x351,data));
}

function si_write() {
	var data = [];
	var soc;

	dprintf(1,"readonly: %d\n", si.readonly);
	if (si.readonly) return 1;

	/* Startup SOC/Charge amps */
	dprintf(5,"startup: %d\n", si.startup);
	if (si.startup == 1) {
		soc = 99.9;
		si.charge_amps = 1;
		si.startup = 2;
	} else if (si.startup == 2) {
		soc = si.soc;
		si.charge_amps = 0;
		charge_control(s,si.startup_charge_mode,1);
		si.startup = 0;
	} else {
		soc = si.soc;
//		charge_check(s);
	}

	/* Take charge amps to 0.0? need to go to 1 first then 0.0 next pass */
	dprintf(5,"tozero: %d, charge_amps: %.1f\n", si.tozero, si.charge_amps);
	if (si.tozero == 1) {
		if (si.charge_amps == 1) {
			dprintf(5,"setting charge amps to 0...\n");
			si.charge_amps = 0.0;
			si.tozero = 2;
		} else {
			si.tozero = 0;
		}
	} else {
		if (si.charge_amps == 0) {
			if (si.tozero != 2) {
				si.charge_amps = 1;
				si.tozero = 1;
			}
		} else {
			si.tozero = 0;
		}
	}

	/* 0x350 Active Current Set-point / Reactive current Set-Point */
	
//	if (si_write_va()) return 1;
	si_write_va();

	/* 0x352 Nominal frequency (F0) */
	/* 0x353 Nominal voltage L1 (U0) */
	/* 0x354 Function Bits SiCom V1 */

	/* 0x355 SOC value / SOH value / HiResSOC */
	dprintf(1,"0x355: SoC: %.1f, SoH: %.1f\n", soc, si.soh);
	putshort(data,0,soc);
	putshort(data,2,si.soh);
	putlong(data,4,(soc * 100.0));
	if (si.can_write(0x355,data) < 0) return 1;

if (0 == 1) {
	/* 0x356 Battery Voltage / Battery Current / Battery Temperature */
	dprintf(1,"0x356: battery_voltage: %3.2f, battery_amps: %3.2f, battery_temp: %3.2f\n",
		si.battery_voltage, si.battery_amps, si.battery_temp);
	putshort(data,0,si.battery_voltage * 100.0);
	putshort(data,2,si.battery_current * 10.0);
	if (si.have_battery_temp) {
		putshort(data[4],si.battery_temp * 10.0);
	} else {
		putshort(data[4],0);
	}
	if (si.can_write(0x356,data) < 0) return 1;
}

	/* 0x357 - 0x359 ?? */

if (0 == 1) {
	/* 0x35A Alarms / Warnings */
	memset(data,0,sizeof(data));
	if (si.can_write(0x35A,data) < 0) return 1;

	/* 0x35B Events */
	memset(data,0,sizeof(data));
	if (si.can_write(0x35B,data) < 0) return 1;
}

	/* 0x35C Function Bits / Function Bits Relays */

if (0 == 1) {
	/* 0x35E Manufacturer-Name-ASCII (8 chars) */
	memset(data,0x20,sizeof(data));
	data[0] = 0x53; data[1] = 0x50; data[2] = 0x53;
	if (si.can_write(0x35E,data) < 0) return 1;

	/* 0x35F - Bat-Type / BMS Version / Bat-Capacity / reserved Manufacturer ID */
	putshort(data,0,1);
	/* major.minor.build.revision - encoded as MmBBr 10142 = 1.0.14.2 */
	putshort(data,2,10000);
	putshort(data,4,2500);
	putshort(data,6,1);
	if (si.can_write(0x35F,data) < 0) return 1;
}

	/* 0x360 Limitation of external current (Gen/Grid) */

	return 0;
}

si_write();

//printf("====> write completed\n");

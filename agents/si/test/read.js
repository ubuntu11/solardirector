/*
Copyright (c) 2021, Stephen P. Shoecraft
All rights reserved.

This source code is licensed under the BSD-style license found in the
LICENSE file in the root directory of this source tree.
*/

printf("********************************* READ ***************************************\n");

if (0 == 1) {
var st = typeof(smanet);
printf("st: %s\n", st);
if (st != "undefined") {
	printf("smanet: %s\n", smanet);
} else {
	si.smanet_transport = "rdev";
	si.smanet_target = "192.168.1.7";
	si.smanet_topts = "serial0";
	si.init_smanet();
printf("====> BACK FROM INIT_SMANET\n");
}
}

if (typeof(s) == "undefined") {
var s = smanet("rdev","192.168.1.7","serial0");
//printf("s: %s\n", s);
s.load_channels("/opt/sd/lib/SI6048UM.dat");
}
//kvar g = s.get("GdManStr");
//printf("g:%s\n",  g);
var g = s.get("GdManStr");
printf("g:%s\n",  g);

//include(agent.script_dir+"/utils.js")

if (typeof(info) == "undefined") var info = si.info;
if (typeof(si_run_completed) == "undefined") var si_run_completed = false;

function _getshort(data,index) {
//	printf("data[%d]: %02x\n", index+1, data[index+1]);
//	printf("data[%d]: %02x\n", index, data[index]);
	var num = (data[index+1] & 0xff) << 8 | data[index] & 0xff;
//	printf("num type: %s\n", typeof(num));
//	printf("returning: %04x\n", num);
	return num;
}

function si_isvrange(v) {return (((v >= SI_VOLTAGE_MIN) && (v  <= SI_VOLTAGE_MAX)) ? true : false); }

function dump_bits(label,bits) {
	var i, mask;
	var bitstr = [];

	mask = 1;
	for(i=0; i < 8; i++) {
		bitstr[i] = (bits & mask ? '1' : '0');
		mask <<= 1;
	}
	bitstr[i] = 0;
	dprintf(5,"%s(%02x): %s\n",label,bits,bitstr);
}

//#define _getshort(v) (short)( ((v)[1]) << 8 | ((v)[0]) )
//#define _getlong(v) (long)( ((v)[3]) << 24 | ((v)[2]) << 16 | ((v)[1]) << 8 | ((v)[0]) )
//#define SET(m,b) { info.m = ((bits & b) != 0); dprintf(4,"%s: %d\n",#m,info.m); }
function SET(what,mask,bits) {
	info[what] = ((bits & mask) ? true : false);
//	dprintf(4,"%s: %s\n", what, info[what]);
}

function si_get_relays() {
	var bits;

	/* 0x307 Relay state / Relay function bit 1 / Relay function bit 2 / Synch-Bits */
	data = si.can_read(0x307,8);
	/* 0-1 Relay State */
	bits = data[0];
	dump_bits("data[0]",bits);
	SET("relay1", 		0x01,bits);
	SET("relay2", 		0x02,bits);
	SET("s1_relay1", 	0x04,bits);
	SET("s1_relay2",	0x08,bits);
	SET("s2_relay1",	0x10,bits);
	SET("s2_relay2",	0x20,bits);
	SET("s3_relay1",	0x40,bits);
	SET("s3_relay2",	0x80,bits);
	bits = data[1];
	dump_bits("data[1]",bits);
	SET("GnRn",		0x01,bits);
	SET("s1_GnRn",		0x02,bits);
	SET("s2_GnRn",		0x04,bits);
	SET("s3_GnRn",		0x08,bits);

	info.GnOn = (info.GnRn && info.AutoGn && info.ac1_frequency > 10.0) ? 1 : 0;
	dprintf(4,"GnOn: %d\n",info.GnOn,bits);

	/* 2-3 Relay function bit 1 */
	bits = data[2];
	dump_bits("data[2]",bits);
	SET("AutoGn",		0x01,bits);
	SET("AutoLodExt",	0x02,bits);
	SET("AutoLodSoc",	0x04,bits);
	SET("Tm1",		0x08,bits);
	SET("Tm2",		0x10,bits);
	SET("ExtPwrDer",	0x20,bits);
	SET("ExtVfOk",		0x40,bits);
	SET("GdOn",		0x80,bits);
	bits = data[3];
	dump_bits("data[3]",bits);
	SET("Error",		0x01,bits);
	SET("Run",		0x02,bits);
	SET("BatFan",		0x04,bits);
	SET("AcdCir",		0x08,bits);
	SET("MccBatFan",	0x10,bits);
	SET("MccAutoLod",	0x20,bits);
	SET("Chp",		0x40,bits);
	SET("ChpAdd",		0x80,bits);

	/* 4 & 5 Relay function bit 2 */
	bits = data[4];
	dump_bits("data[4]",bits);
	SET("SiComRemote",	0x01,bits);
	SET("OverLoad",		0x02,bits);
	bits = data[5];
	dump_bits("data[5]",bits);
	SET("ExtSrcConn",	0x01,bits);
	SET("Silent",		0x02,bits);
	SET("Current",		0x04,bits);
	SET("FeedSelfC",	0x08,bits);
	SET("Esave",		0x10,bits);

	/* 6 Sync-bits */
	/* 7 unk */

	return 0;
}

function si_read() {
	var data = [];

if (0 == 1) {
	/* x300 Active power grid/gen */
	data = si.can_read(0x300,8);
	if (!data) {
		printf("returning\n");
		return 1;
	}
	printf("len: %d\n", data.length);
	info.active.grid.l1 = _getshort(data,0) * 100.0;
	info.active.grid.l2 = _getshort(data,2) * 100.0;
	info.active.grid.l3 = _getshort(data,4) * 100.0;
	dprintf(4,"active grid: l1: %.1f, l2: %.1f, l3: %.1f\n", info.active.grid.l1, info.active.grid.l2, info.active.grid.l3);

	/* x301 Active power Sunny Island */
	data = si.can_read(0x301,8);
	info.active.si.l1 = _getshort(data,0) * 100.0;
	info.active.si.l2 = _getshort(data,2) * 100.0;
	info.active.si.l3 = _getshort(data,4) * 100.0;
	dprintf(4,"active si: l1: %.1f, l2: %.1f, l3: %.1f\n", info.active.si.l1, info.active.si.l2, info.active.si.l3);

	/* x302 Reactive power grid/gen */
	data = si.can_read(0x302,8);
	info.reactive.grid.l1 = _getshort(data,0) * 100.0;
	info.reactive.grid.l2 = _getshort(data,2) * 100.0;
	info.reactive.grid.l3 = _getshort(data,4) * 100.0;
	dprintf(4,"reactive grid: l1: %.1f, l2: %.1f, l3: %.1f\n", info.reactive.grid.l1, info.reactive.grid.l2, info.reactive.grid.l3);

	/* x303 Reactive power Sunny Island */
	data = si.can_read(0x303,8);
	info.reactive.si.l1 = _getshort(data,0) * 100.0;
	info.reactive.si.l2 = _getshort(data,2) * 100.0;
	info.reactive.si.l3 = _getshort(data,4) * 100.0;
	dprintf(4,"reactive si: l1: %.1f, l2: %.1f, l3: %.1f\n", info.reactive.si.l1, info.reactive.si.l2, info.reactive.si.l3);

	info.ac1_current = info.active.si.l1 + info.active.si.l2;
	info.ac1_current += (info.reactive.si.l1*(-1)) + (info.reactive.si.l2 *(-1));

	info.ac2_current = info.active.grid.l1 + info.active.grid.l2;
	info.ac2_current += (info.reactive.grid.l1*(-1)) + (info.reactive.grid.l2 *(-1));
}

	/* 0x304 OutputVoltage - L1 / L2 / L3 / Output Freq */
	data = si.can_read(0x304,8);
	info.ac1_voltage_l1 = _getshort(data,0) / 10.0;
	info.ac1_voltage_l2 = _getshort(data,2) / 10.0;
	info.ac1_voltage_l3 = _getshort(data,4) / 10.0;
	info.ac1_frequency = _getshort(data,6) / 100.0;
//	dprintf(4,"ac1_volts: l1: %.1f, l2: %.1f, l3: %.1f\n", info.ac1_voltage_l1, info.ac1_voltage_l2, info.ac1_voltage_l3);
	info.ac1_voltage = info.ac1_voltage_l1 + info.ac1_voltage_l2;
	dprintf(4,"ac1_voltage: %.1f, ac1_frequency: %.1f\n",info.ac1_voltage,info.ac1_frequency);

	/* 0x305 Battery voltage Battery current Battery temperature SOC battery */
	data = si.can_read(0x305,8);
	info.battery_voltage = _getshort(data,0) / 10.0;
	info.battery_current = _getshort(data,2) / 10.0;
	info.battery_temp = _getshort(data,4) / 10.0;
	info.battery_soc = _getshort(data,6) / 10.0;
	dprintf(4,"battery_voltage: %.1f\n", info.battery_voltage);
	dprintf(4,"battery_current: %.1f\n", info.battery_current);
	dprintf(4,"battery_temp: %.1f\n", info.battery_temp);
	dprintf(4,"battery_soc: %.1f\n", info.battery_soc);

if (0 == 1) {
	/* 0x306 SOH battery / Charging procedure / Operating state / active Error Message / Battery Charge Voltage Set-point */
	data = si.can_read(0x306,8);
	info.battery_soh = _getshort(data,0);
	info.charging_proc = data[2];
	info.state = data[3];
	info.errmsg = _getshort(data,4);
	info.battery_cvsp = _getshort(data,6) / 10.0;
//	dprintf(4,"battery_soh: %.1f\n", info.battery_soh);
//	dprintf(4,"charging_proc: %.1f\n", info.charging_proc);
//	dprintf(4,"state: %.1f\n", info.state);
//	dprintf(4,"errmsg: %.1f\n", info.errmsg);
	dprintf(4,"battery_cvsp: %.1f\n", info.battery_cvsp);
	dprintf(4,"battery_soh: %.1f, charging_proc: %d, state: %d, errmsg: %d, battery_cvsp: %.1f\n",
		info.battery_soh, info.charging_proc, info.state, info.errmsg, info.battery_cvsp);
}

	if (!si_run_completed) if (si_get_relays()) return 1;

	/* 0x308 TotLodPwr */
	data = si.can_read(0x308,8)
	info.TotLodPwr = _getshort(data,0) * 100.0;
	dprintf(4,"TotLodPwr: %.1f\n", info.TotLodPwr);

	/* 0x309 AC2 Voltage L1 / AC2 Voltage L2 / AC2 Voltage L3 / AC2 Frequency */
	data = si.can_read(0x309,8);
	info.ac2_voltage_l1 = _getshort(data,0) / 10.0;
	info.ac2_voltage_l2 = _getshort(data,2) / 10.0;
	info.ac2_voltage_l3 = _getshort(data,4) / 10.0;
	info.ac2_frequency = _getshort(data,6) / 100.0;
//	dprintf(4,"ac2_volts: l1: %.1f, l2: %.1f, l3: %.1f\n",info.ac2_voltage_l1, info.ac2_voltage_l2, info.ac2_voltage_l3);
	info.ac2_voltage = info.ac2_voltage_l1 + info.ac2_voltage_l2;
	dprintf(4,"ac2: voltage: %.1f, frequency: %.1f\n",info.ac2_voltage,info.ac2_frequency);

if (0 == 1) {
	/* 0x30A PVPwrAt / GdCsmpPwrAt / GdFeedPwr */
	data = si.can_read(0x30a,8);
	info.PVPwrAt = _getshort(data,0) / 10.0;
	info.GdCsmpPwrAt = _getshort(data,0) / 10.0;
	info.GdFeedPwr = _getshort(data,0) / 10.0;
	dprintf(4,"PVPwrAt: %.1f\n", info.PVPwrAt);
	dprintf(4,"GdCsmpPwrAt: %.1f\n", info.GdCsmpPwrAt);
	dprintf(4,"GdFeedPwr: %.1f\n", info.GdFeedPwr);
}

	if (info.GdOn != si.grid_connected) {
		si.grid_connected = info.GdOn;
		log_info("Grid %s\n",(si.grid_connected ? "connected" : "disconnected"));
	}
	if (info.GnOn != si.gen_connected) {
		si.gen_connected = info.GnOn;
		log_info("Generator %s\n",(si.gen_connected ? "connected" : "disconnected"));
	}

	/* Sim? */
	if (si.sim) {
		if (si.startup == 1) {
			si.tvolt = si.charge_start_voltage + 4.0;
			si.sim_amps = -50;
		}
		else if (si.charge_mode == 0) info.battery_voltage = (si.tvolt -= 0.8);
		else if (si.charge_mode == 1) info.battery_voltage = (si.tvolt += 2.4);
		else if (si.charge_mode == 2) {
			info.battery_voltage = si.charge_end_voltage;
			if (si.cv_method == 0) {
				si.cv_start_time -= 1800;
			} else if (si.cv_method == 1) {
				si.sim_amps += 8;
				if (si.sim_amps >= 0) si.sim_amps = (si.cv_cutoff - 1.0) * -1;
				if (!si.gen_started && si.bafull) {
					si.sim_amps = 27;
					si.gen_started = 1;
				}
				info.battery_current = si.sim_amps;
			}
		}
	}

	/* Calculate SOC */
	if (!si_isvrange(si.min_voltage)) {
		log_warning("setting min_voltage to %.1f\n", 41.0);
		si.min_voltage = 41.0;
	}
	if (!si_isvrange(si.max_voltage)) {
		log_warning("setting max_voltage to %.1f\n", 58.1);
		si.max_voltage = 58.1;
	}
	dprintf(4,"user_soc: %.1f, battery_voltage: %.1f\n", si.user_soc, info.battery_voltage);
	if (si.user_soc < 0.0) {
		dprintf(1,"battery_voltage: %.1f, min_voltage: %.1f, max_voltage: %.1f\n",
			info.battery_voltage, si.min_voltage, si.max_voltage);
		si.soc = ( ( info.battery_voltage - si.min_voltage) / (si.max_voltage - si.min_voltage) ) * 100.0;
	} else  {
		si.soc = si.user_soc;
	}
	si.soh = 100.0;

	/* Adjust SOC so GdSocEna and GnSocEna dont disconnect until we're done charging */
	dprintf(1,"grid_connected: %d, charge_mode: %d, grid_soc_stop: %.1f, soc: %.1f\n",
		si.grid_connected,si.charge_mode,si.grid_soc_stop,si.soc);
	dprintf(1,"gen_connected: %d, charge_mode: %d, gen_soc_stop: %.1f, soc: %.1f\n",
		si.gen_connected,si.charge_mode,si.gen_soc_stop,si.soc);
	if (si.grid_connected && si.charge_mode && si.grid_soc_stop > 1.0 && si.soc >= si.grid_soc_stop)
		si.soc = si.grid_soc_stop - 1;
	else if (si.gen_connected && si.charge_mode && si.gen_soc_stop > 1.0 && si.soc >= si.gen_soc_stop)
		si.soc = si.gen_soc_stop - 1;
	if (info.battery_voltage != si.last_battery_voltage || si.soc != si.last_soc) {
		printf("%s%sBattery Voltage: %.1fV, SoC: %.1f%%\n",
			(info.Run ? "[Running] " : ""), (si.sim ? "(SIM) " : ""), info.battery_voltage, si.soc);
		si.last_battery_voltage = info.battery_voltage;
		si.last_soc = si.soc;
	}

	return 0;
}
si_read();
exit(0);


/*
Copyright (c) 2022, Stephen P. Shoecraft
All rights reserved.

This source code is licensed under the BSD-style license found in the
LICENSE file in the root directory of this source tree.
*/

function run_init() {
	last_memused = memused();
}

// This script is called once per second to quickly respond to grid/gen changes
function run_main() {

	/* refresh if not in sim */
	if (!sim.enable) data.refresh();

	if (typeof(grid_connected) == "undefined") grid_connected = data.GdOn;
	if (typeof(gen_connected) == "undefined") gen_connected = data.GnOn;

	if (!si_isvrange(si.charge_voltage)) si.charge_voltage = data.battery_voltage;
	if (!si_isvrange(si.charge_voltage)) {
		log_error("si.charge_voltage is out of range\n");
		return 1;
	}

	dprintf(1,"GdOn: %s, grid_connected: %s, GnOn: %s, gen_connected: %s\n", data.GdOn, grid_connected, data.GnOn, gen_connected);
	if ((data.GdOn != grid_connected) || (data.GnOn != gen_connected)) {

		/* 0x351 Battery charge voltage / DC charge current limitation / DC discharge current limitation / discharge voltage */
		dprintf(1,"0x351: charge_voltage: %3.2f, charge_amps: %3.2f, min_voltage: %3.2f, discharge_amps: %3.2f\n",
			si.charge_voltage, si.charge_amps, si.min_voltage, si.discharge_amps);
		if (si.charge_mode) {
			cv = si.charge_voltage;
			if (data.GdOn) {
				ca = si.grid_charge_amps;
			} else if (data.GnOn) {
				ca = si.gen_charge_amps;
			} else {
				cv += 1.0;
				ca = si.std_charge_amps;
			}

			/* Apply modifiers to charge amps */
			dprintf(5,"cv: %.1f\n", cv);
			dprintf(6,"ca(1): %.1f\n", ca);
			ca *= si.charge_amps_temp_modifier;
			dprintf(6,"ca(2): %.1f\n", ca);
			ca *= si.charge_amps_soc_modifier;
			dprintf(5,"ca(3): %.1f\n", ca);
		} else {
			/* make sure no charge goes into batts */
			cv = data.battery_voltage + 0.1;
			ca = 0.0;
		}
		printf("cv: %.1f, ca: %.1f\n", cv, ca);

		var bits = [];
		putshort(bits,0,cv * 10.0);
		putshort(bits,2,ca * 10.0)
		putshort(bits,4,si.discharge_amps * 10.0)
		putshort(bits,6,si.min_voltage * 10.0);
		// use si.can_write so it uses our controls
		si.can_write(0x351,bits);
	}

	grid_connected = data.GdOn;
	gen_connected = data.GnOn;

	include(script_dir+"/../../core/influx.js");
	var r = influx.query("select last(value) from solar");
	if (!r) printf("influx error: %s\n", influx.errmsg);
	influx_dump_results(r);

	if (typeof(last_memused) == "undefined") last_memused = -1;
	if (memused() != last_memused) {
		printf("mem: %d\n", memused());
		last_memused = memused();
	}
}


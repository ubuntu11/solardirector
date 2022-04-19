
/*
Copyright (c) 2022, Stephen P. Shoecraft
All rights reserved.

This source code is licensed under the BSD-style license found in the
LICENSE file in the root directory of this source tree.
*/

function read_main() {

//	printf("==> read_main\n");

//	printf("sim.enabled: %s\n", sim.enable);
	if (sim.enable) run(dirname(script_name)+"/sim.js");

	// Feed check
	if (typeof(feed_check) == "undefined") feed_check = false;
	dprintf(2,"connected: %d, feeding: %d, feed_check: %d\n", smanet.connected, si.feeding, feed_check);
	if (smanet.connected && !si.feeding && !feed_check) {
		var vals = smanet.get(["GdMod","GdManStr"]);
		dprintf(2,"vals.GdMod: %s, GdManStr: %s\n", vals.GdMod, vals.GdManStr);
		if (vals.GdMod == "GridFeed" && vals.GdManStr == "Start") si.feeding = true;
		feed_check = true;
	}

	// Dynamic feed
	dprintf(2,"input_source: %d, dynfeed: %s, feeding: %d, GdOn: %d, charge_mode: %d\n",
		si.input_source, si.dynfeed, si.feeding, data.GdOn, si.charge_mode);
	if (si.input_source != CURRENT_SOURCE_NONE && si.dynfeed && si.feeding && data.GdOn && si.charge_mode) {
		run(script_dir+"/dynfeed.js");
	}

	// If SMANET connected, get current info
	if (smanet.connected && 0 == 1) {
		var vals = smanet.get([ "ExtCur","ExtCurSlv1","ExtCurSlv2","ExtCurSlv3","TotExtCur","InvCur","InvCurSlv1","InvCurSlv2","InvCurSlv3","TotInvCur" ]);
		if (typeof(vals) == "undefined") log_error("unable to get currents: %s\n", smanet.errmsg);
		else influx.write("current",vals);
	}

	// Set charge parms
	run(script_dir+"/charge.js");

	// If the generator is running AND we're charging AND gen_hold_soc is set, set soc to hold value
	dprintf(1,"GnOn: %d, charge_mode: %d, gen_hold_soc: %.1f, soc: %.1f\n",
		data.GnOn, si.charge_mode, si.gen_hold_soc, si.soc);
	if (data.GnOn && si.charge_mode && si.gen_hold_soc > 0 && si.gen_hold_soc < si.soc) {
		dprintf(1,"adjusting soc to: %.1f\n", si.gen_hold_soc);
		si.soc = si.gen_hold_soc;
	}

	// Publish data
	run(script_dir+"/pub.js");

if (0 == 1) {
        if (typeof(last_memused) == "undefined") last_memused = -1;
        if (memused() != last_memused) {
                printf("mem: %d\n", memused());
                last_memused = memused();
        }

        if (typeof(last_sysmemused) == "undefined") last_sysmemused = -1;
        if (sysmemused() != last_sysmemused) {
                printf("sysmem: %d\n", sysmemused());
                last_sysmemused = sysmemused();
        }
}
}

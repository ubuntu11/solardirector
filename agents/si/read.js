
/*
Copyright (c) 2022, Stephen P. Shoecraft
All rights reserved.

This source code is licensed under the BSD-style license found in the
LICENSE file in the root directory of this source tree.
*/

function read_main() {
//	printf("sim.enabled: %s\n", sim.enable);
	if (sim.enable) run(dirname(script_name)+"/sim.js");

	// If SMANET connected, get current and write to db
	if (si.smanet.connected && 0 == 1) {
		var names = [ "ExtCur","ExtCurSlv1","ExtCurSlv2","ExtCurSlv3","TotExtCur","InvCur","InvCurSlv1","InvCurSlv2","InvCurSlv3","TotInvCur" ];
		var vals = si.smanet.get(names);
		var data = [];
		for(i=0; i < vals.length; i++) {
			printf("%-20.20s: %s\n", cur[i], vals[i]);
			data[i] = [cur[i]][vals[i]]
		}
//		si.agent.influx.write("current");
		printf("data: %s\n", data);
	}

}


agent.libdir="../../core";
include(agent.libdir+"/utils.js");
include(agent.libdir+"/inverter.js");
include(agent.libdir+"/charger.js");

printf("===> READING\n");

if (typeof(last_read) == "undefined") var last_read = Date();
printf("last_read: %s\n", last_read);
var cur_read = Date();
printf("cur_read: %s\n", cur_read);
var diff = cur_read - last_read;
printf("diff: %d\n", Math.abs(diff));

var fields = inverter_fields.concat(charger_fields).unique().sort();
//printf("fields: %s\n", fields);

printf("smanet: %s\n", typeof(smanet));
printf("connected: %s\n", smanet.connected);

if (0 == 1) {
if (typeof(channels_loaded) == "undefined") var channels_loaded = false;
if (smanet.connected) {
	if (!channels_loaded) {
		printf("loading channels...\n");
		if (smanet.load_channels(si.smanet_channels_path)) printf("error loading channels\n");
		else channels_loaded = true;
	}
	if (channels_loaded) {
		var gd = smanet.get("GdManStr");
		printf("%s\n", gd);
		if (typeof(gd) == "undefined") abort(0);
	}
}
}

printf("*********************\n");

var data = (typeof(si) == "undefined" ? [] : si.data);

//for(var key in data) {
//	printf("%s: %s\n", key, data[key]);
//}

data.battery_level = si.soc;
j = JSON.stringify(data,fields,4);
printf("j: %s\n", j);
mqtt.pub(SOLARD_TOPIC_ROOT+"/"+agent.name+"/"+SOLARD_FUNC_DATA,j,0);

if (0 == 1) {
        /* Sim? */
        if (si.sim) {
                if (si.startup == 1) {
                        si.tvolt = si.charge_start_voltage + 4.0;
                        si.sim_amps = -50;
                }
                else if (si.charge_mode == 0) si.data.battery_voltage = (si.tvolt -= 0.8);
                else if (si.charge_mode == 1) si.data.battery_voltage = (si.tvolt += 2.4);
                else if (si.charge_mode == 2) {
                        si.data.battery_voltage = si.charge_end_voltage;
                        if (si.cv_method == 0) {
                                si.cv_start_time -= 1800;
                        } else if (si.cv_method == 1) {
                                si.sim_amps += 8;
                                if (si.sim_amps >= 0) si.sim_amps = (si.cv_cutoff - 1.0) * -1;
                                if (!si.gen_started && si.bafull) {
                                        si.sim_amps = 27;
                                        si.gen_started = 1;
                                }
                                si.data.battery_current = si.sim_amps;
                        }
                }
        }
}

last_read = Date();

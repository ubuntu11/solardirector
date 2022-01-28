#!/usr/bin/env sdjs

agent.libdir="../../core";
include(agent.libdir+"/utils.js");
include(agent.libdir+"/inverter.js");
include(agent.libdir+"/charger.js");

//printf("can_connected: %s, smanet_connected: %s\n", si.can_connected, si.smanet_connected);
if (!si.can_connected) {
	dprintf(1,"can_transport: %s, can_target: %s, can_topts: %s\n", si.can_transport, si.can_target, si.can_topts);
	if (si.can_transport.length && si.can_target.length) si.can_init();
}

if (0 == 1) {
if (typeof(last_read) == "undefined") var last_read = Date();
printf("last_read: %s\n", last_read);
var cur_read = Date();
printf("cur_read: %s\n", cur_read);
var diff = cur_read - last_read;
printf("diff: %d\n", Math.abs(diff));
}

var fields = inverter_fields.concat(charger_fields).unique().sort();
//printf("fields: %s\n", fields);

//printf("can: %s\n", typeof(can));
//if (typeof(can) != "undefined") printf("CAN connected: %s\n", can.connected);

//printf("smanet: %s\n", typeof(smanet));
//if (typeof(smanet) != "undefined") printf("SMANET connected: %s\n", smanet.connected);


//for(var key in data) {
//	printf("%s: %s\n", key, data[key]);
//}

function merge(target, source) {
    Object.keys(source).forEach(function (key) {
        if (source[key] && typeof source[key] === 'object') {
            merge(target[key] = target[key] || {}, source[key]);
            return;
        }
        target[key] = source[key];
    });
}

function addit(str,val,text) {
//	printf("val: %d, text: %s\n", val, text);
	if (str.length) str += ",";
str += text + ":" + (val ? "true" : "false");
	return str;
}

function addstat(str,val,text) {
	if (val) str += "[" + text + "]";
	return str;
}
/******************************************************/
//printf("si: %s\n", typeof(si));
if (typeof(si) != "undefined") {
	var data = (typeof(si.data) != "undefined" ? si.data : {});
	var mydata = data;
	mydata.charge_current = si.charge_amps;
	mydata.charge_power = si.charge_voltage * si.charge_amps;
	for(key in si) {
		if (mydata.hasOwnProperty(key)) continue;
		mydata[key] = si[key];
	}
	var status = "";
//	printf("batlev: %f\n", data.battery_level);
	status = addstat(status,si.bms_mode != 0,"bms");
	status = addstat(status,si.can_connected,"can");
	status = addstat(status,si.smanet_connected,"smanet");
	status = addstat(status,si.charge_mode != 0,"charging");
//	status = addstat(status,si.feed != 0,"feeding");
	status = addstat(status,mydata.GdOn,"grid");
	status = addstat(status,mydata.GnOn,"gen");
//	printf("status: %s\n", status);
	printf("%s Battery: Voltage: %.1f, Current: %.1f(%.1f), Level: %.1f\n", status, mydata.battery_voltage,
		mydata.battery_current, mydata.battery_power, mydata.battery_level);
	mydata.status = status;
//	printf("mydata: %s\n", mydata);
	fields.push("status");
	j = JSON.stringify(mydata,fields,4);
//	printf("j: %s\n", j);

	mqtt.pub(SOLARD_TOPIC_ROOT+"/Agents/"+agent.name+"/"+SOLARD_FUNC_DATA,j,0);

	influx.write("test",j);
}

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

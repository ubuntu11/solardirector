
/*
Copyright (c) 2022, Stephen P. Shoecraft
All rights reserved.

This source code is licensed under the BSD-style license found in the
LICENSE file in the root directory of this source tree.
*/

function pub_main() {
	dprintf(1,"can_connected: %s, smanet_connected: %s\n", si.can_connected, si.smanet_connected);
	if (!si.can_connected && !si.smanet_connected) exit(0);

	var libdir = dirname(script_name) + "/../../core";
	include(libdir+"/utils.js");
	include(libdir+"/inverter.js");
	include(libdir+"/charger.js");

	var fields = inverter_fields.concat(charger_fields).unique().sort();
	//printf("fields: %s\n", fields);

	delobj(data);
	var data = si.data;

	function addstat(str,val,text) {
		if (val) str += "[" + text + "]";
		return str;
	}

	var status = "";
	status = addstat(status,si.bms_mode,"bms");
	status = addstat(status,si.can_connected,"can");
	status = addstat(status,si.smanet_connected,"smanet");
	status = addstat(status,data.GdOn,"grid");
	status = addstat(status,data.GnOn,"gen");
	status = addstat(status,si.charge_mode,"charge");
	status = addstat(status,si.feed,"feed");
	//printf("status: %s\n", status);
	fields.push("status");

	var mydata = {};
	//fields: battery_current,battery_level,battery_power,battery_temp,battery_voltage,capabilities,charge_current,charge_mode,charge_power,charge_voltage,input_current,input_frequency,input_power,input_voltage,load,output_current,output_frequency,output_power,output_voltage
	var tab = [
			[ "battery_voltage",	data.battery_voltage ],
			[ "battery_current",	data.battery_current ],
			[ "battery_power",	data.battery_power ],
			[ "battery_temp",	data.battery_temp ],
			[ "battery_level",	data.battery_level ],
			[ "charge_mode",	si.charge_mode ],
			[ "charge_voltage",	si.charge_voltage ],
			[ "charge_current",	si.charge_current ],
			[ "charge_power",	si.charge_voltage * si.charge_current ],
			[ "input_voltage",	data.input_voltage ],
			[ "input_frequency",	data.input_frequency ],
			[ "input_current",	data.input_current ],
			[ "input_power",	data.input_power ],
			[ "output_voltage",	data.output_voltage ],
			[ "output_frequency",	data.output_frequency ],
			[ "output_current",	data.output_current ],
			[ "output_power",	data.output_power ],
			[ "load_power",		data.load_power ],
			[ "status",		status ],
	];
	for(j=0; j < tab.length; j++) mydata[tab[j][0]] = tab[j][1];
	//for(key in mydata) printf("%20.20s: %s\n", key, mydata[key]);
	tab = null;

	var format = 0;
	switch(format) {
	case 0:
		out = sprintf("%s Battery: Voltage: %.1f, Current: %.1f, Level: %.1f",
			status, mydata.battery_voltage, mydata.battery_current, mydata.battery_level);
		break;
	case 1:
		out = sprintf("%s Battery: Voltage: %.1f, Current: %.1f, Power: %.1f, Level: %.1f",
			status, mydata.battery_voltage, mydata.battery_current, mydata.battery_power, mydata.battery_level);
		break;
	case 2:
		out = sprintf("%s Battery: Voltage: %.1f, Power: %.1f, Level: %.1f",
			status, mydata.battery_voltage, mydata.battery_power, mydata.battery_level);
		break;
	}
	if (typeof(last_out) == "undefined") last_out = "";
	if (out != last_out) printf("%s\n",out);
	last_out = out;

	j = JSON.stringify(mydata,fields,4);
	//printf("j: %s\n", j);
	fields = null;

	mqtt.pub(SOLARD_TOPIC_ROOT+"/Agents/"+agent.name+"/"+SOLARD_FUNC_DATA,j,0);

	influx.write("test",j);
}

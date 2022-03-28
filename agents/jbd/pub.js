
function pub_main() {
	include(dirname(script_name)+"/../../core/utils.js")
	include(dirname(script_name)+"/../../core/battery.js")

	// Select temp which is furthest from 25c
	var max_delta = 0.0;
	var max_temp = 0.0;
	for(i=0; i < data.ntemps; i++) {
		t = data.temps[i];
		var v = 0;
		if (t > 25) v = t - 25;
		else if (t < 25) v = 25 - t;
		if (v > max_delta) {
			max_delta = v;
			max_temp = t;
		}
	}
	dprintf(1,"max_delta: %.1f, max_temp: %.1f\n", max_delta, max_temp);
	// Convert to F
	//data.temp = max_temp * (9/5) + 32;
	data.temp = max_temp;

	var fields = battery_fields;
	dprintf(1,"flatten: %s\n", jbd.flatten);
	if (jbd.flatten) {
		for(i=0; i < data.ntemps; i++) {
			f = sprintf("temp_%02d",i);
			data[f] = data.temps[i];
			fields.push(f);
		}
		delete data.ntemps;
		delete data.temps;
		for(i=0; i < data.ncells; i++) {
			f = sprintf("cell_%02d",i);
			data[f] = data.cellvolt[i];
			fields.push(f);
		}
		delete data.ncells;
		delete data.cellvolt;
	} else {
		fields.push("temps");
	}
	data.power = data.voltage * data.current;

	function addstate(str,val,text) {
		if (val) {
			if (str.length) str += ",";
			str += text;
		}
		return str;
	}

	var state = "";
	var statebits = jbd.state;
	dprintf(1,"statebits: %x\n", statebits);
	state = addstate(state,statebits & JBD_STATE_CHARGING,"Charging");
	state = addstate(state,statebits & JBD_STATE_DISCHARGING,"Discharging");
	state = addstate(state,statebits & JBD_STATE_BALANCING,"Balancing");
	dprintf(1,"state: %s\n", state);
	data.state = state;

	j = JSON.stringify(data,fields,4);
	printf("j: %s\n", j);
	jbd.agent.mqtt.pub(jbd.agent.mktopic(SOLARD_FUNC_DATA),j);

	if (typeof(last_power) == "undefined") last_power = 0;
	if (data.power != last_power) log_info("%.1f\n",data.power);
	last_power = data.power;
}

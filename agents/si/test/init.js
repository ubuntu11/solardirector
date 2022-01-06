
//////
// Sunny Island init functions
/////

function getshort(data,index) {
	printf("data[%d]: %d\n", index, data[index]);
	printf("data[%d]: %d\n", index+1, data[index+1]);
	var s = (data[index] & 0xF0) << 8 | data[index+1] & 0x0F;
	return s;
}

function putshort(data,index,num) {
        var hex = sprintf("%04x",num);
        data[index++] = "0x" + hex.slice(2,4);
        data[index++] = "0x" + hex.slice(0,2);
};

function putlong(data,index,num) {
        var hex = sprintf("%08x",num);
	dprintf(1,"hex: %s\n", hex);
        data[index++] = "0x" + hex.slice(6,8);
        data[index++] = "0x" + hex.slice(4,6);
        data[index++] = "0x" + hex.slice(2,4);
        data[index++] = "0x" + hex.slice(0,2);
};

// Dont keep getprop'ing on this (and obj creation)
var info = si.info;

si.charge_amps = 50.0;
printf("%f\n", si.charge_amps);

si.interval = 10;
si.smanet_channels_path="/opt/sd/lib/SI6048UM.dat";

config.filename = "sitest.conf";

// XXX
si.readonly = false;
si_run_completed = false;

agent.libdir="../../core";
printf("===> libdir: %s\n", agent.libdir);
include(agent.libdir+"/init.js");
include(agent.libdir+"/inverter.js");

fields = inverter_fields + charger_fields;

printf("===> %s done\n", script_name);

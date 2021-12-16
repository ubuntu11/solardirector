
function arrayUnique(array) {
    var a = array.concat();
    for(var i=0; i<a.length; ++i) {
        for(var j=i+1; j<a.length; ++j) {
            if(a[i] === a[j])
                a.splice(j--, 1);
        }
    }

    return a;
}

Array.prototype.unique = function() {
    var a = this.concat();
    for(var i=0; i<a.length; ++i) {
        for(var j=i+1; j<a.length; ++j) {
            if(a[i] === a[j])
                a.splice(j--, 1);
        }
    }

    return a;
};

var inverter_fields = [];
var i = 0;
inverter_fields[i++] = "input_voltage";
inverter_fields[i++] = "input_frequency";
inverter_fields[i++] = "input_current";
inverter_fields[i++] = "output_voltage";
inverter_fields[i++] = "output_frequency";
inverter_fields[i++] = "output_current";
inverter_fields[i++] = "capacity";
inverter_fields[i++] = "TotLodPwr";
printf("inverter_fields: %s\n", inverter_fields);

var charger_fields = [];
var i = 0;
charger_fields[i++] = "battery_voltage";
charger_fields[i++] = "battery_current";
charger_fields[i++] = "battery_temp";

var fields = inverter_fields.concat(charger_fields).unique().sort();

dprintf(1,"%s started\n", script_name);
mqtt.pub("SolarD/si/data",JSON.stringify(si,fields,'\t'));
exit(0);

si.last_check = Date();

var format = "%-25.25s %s\n"
printf(format,"Voltage",si.voltage);
printf(format,"Frequency",si.frequency);
printf(format,"Capacity",sprintf("%.1f",si.capacity));
printf(format,"Load",si.TotLodPwr);
printf(format,"Run",si.Run);

var j = JSON.parse("{}");
for (var key in si) {
	printf("key: %s, value: %s\n", key, si[key]);
	j[key] = si[key];
}

var getKeys = function (obj) {
  var keysArr = [];
  for (var key in obj) {
    keysArr.push(key);
  }
  printf(keysArr);
}
getKeys(si);

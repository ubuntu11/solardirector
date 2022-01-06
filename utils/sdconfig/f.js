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
printf("fields: %s\n", fields);


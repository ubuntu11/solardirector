
include(SOLARD_LIBDIR+"/battery.js")

dprintf(1,"battery_fields: %s\n", battery_fields);
 j = JSON.stringify(jbd,battery_fields,4);
mqtt.pub("SolarD/"+jbd.name+"/"+SOLARD_FUNC_DATA,j,0);

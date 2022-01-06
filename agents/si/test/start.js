
printf("**************************************************************\n");

var t = typeof(smanet);
if (typeof(smanet) == "object") {
	var GdManStr = smanet.get("GdManStr");
	dprintf(1,"=====================> GdManStr: %s\n", GdManStr);
}

printf("battery_voltage: %d\n", si.battery_voltage);

//for(key in si) printf("key: %s\n", key);
printf("interval: %d, read_interval: %d, write_interval: %d\n", si.interval, si.read_interval, si.write_interval);

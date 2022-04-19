
function dynfeed_main() {

//	printf("==> dynfeed_main\n");

	var dlevel = 1;
	dprintf(dlevel,"battery_power: %.1f\n", data.battery_power);
	dprintf(dlevel,"ac2_power: %.1f\n", data.ac2_power);
	if (isNaN(data.ac2_power)) return 0;
	var diff = data.ac2_power - data.battery_power;
	dprintf(dlevel,"diff: %f\n", diff);
	nca = diff / data.battery_voltage;
	dprintf(dlevel,"nca: %f\n", nca);
	if (nca < 0) nca = 0;
	if (nca) {
		nca -= (nca * 0.10);
		dprintf(2,"nca: %f\n", nca);
	}
	if (nca < si.min_charge_amps) nca = si.min_charge_amps;
	if (nca > si.max_charge_amps) nca = si.max_charge_amps;
	dprintf(dlevel,"nca: %f\n", nca);
//	if (typeof(last_nca) == "undefined") last_nca = nca;
//	dprintf(dlevel,"last_nca: %f, nca: %f\n", last_nca, nca);
	si.charge_amps = nca;
//	last_nca = nca;
}

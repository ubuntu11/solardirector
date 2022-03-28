
function pub_init() {
	// Build the map
	var objmap = [
		// [6380_40451F00] DC Side -> DC measurements -> Voltage
		[ "6380_40251E00", "input_voltage" ],
		// [6380_40452100] DC Side -> DC measurements -> Current 
		[ "6380_40452100", "input_current" ],
		// [6380_40251E00] DC Side -> DC measurements -> Power
		[ "6380_40251E00", "input_power" ],
		// [6100_00464800] AC Side -> Grid measurements -> Phase voltage -> Phase L1:
		[ "6100_00464800", "output_voltage" ],
		// [6100_00464800] AC Side -> Grid measurements -> Phase voltage -> Phase L2:
		[ "6100_00464900", "output_voltage" ],
		// [6100_00464800] AC Side -> Grid measurements -> Phase voltage -> Phase L3:
		[ "6100_00464A00", "output_voltage" ],
		// [6100_00465700] AC Side -> Grid measurements -> Grid frequency
		[ "6100_00465700", "output_frequency" ],
		// [6100_40465300] AC Side -> Grid measurements -> Phase currents -> Phase L1
		[ "6100_40465300", "output_current" ],
		// [6100_40465400] AC Side -> Grid measurements -> Phase currents -> Phase L2
		[ "6100_40465400", "output_current" ],
		// [6100_40465500] AC Side -> Grid measurements -> Phase currents -> Phase L3
		[ "6100_40465500", "output_current" ],
		// [6100_40263F00] AC Side -> Grid measurements -> Power
		[ "6100_40263F00", "output_power" ],
		// [6400_00260100] AC Side -> Measured values -> Total yield
		[ "6400_00260100", "total_yeild" ],
		// [6400_00262200] AC Side -> Measured values -> Daily yield
		[ "6400_00262200", "daily_yeild" ]
	];
}

function pub_main() {
	printf("publishing...\n");
	return 0;
	sb.agent.config.filename = "sb.json";
	sb.agent.config.write();
}

exit(0);
// This is called once per second

load("charge.js");

function si_write_va() {

	if (si.readonly) return 1;

	/* 0x351 Battery charge voltage / DC charge current limitation / DC discharge current limitation / discharge voltage */
	dprintf(1,"0x351: charge_voltage: %3.2f, charge_amps: %3.2f, min_voltage: %3.2f, discharge_amps: %3.2f\n",
		si.charge_voltage, si.charge_amps, si.min_voltage, si.discharge_amps);
        if (si.charge_mode) {
                cv = si.charge_voltage;
                if (si.info.GdOn) {
                        if (si.force_charge) ca = si.grid_charge_amps;
                        else ca = si.charge_min_amps;
                } else if (si.info.GnRn) {
                        ca = si.gen_charge_amps;
                } else {
                        cv += 1.0;
                        ca = si.charge_amps;
                }

		/* Apply modifiers to charge amps */
		dprintf(5,"cv: %.1f\n", cv);
		dprintf(6,"ca(1): %.1f\n", ca);
		ca *= si.charge_amps_temp_modifier;
		dprintf(6,"ca(2): %.1f\n", ca);
		ca *= si.charge_amps_soc_modifier;
		dprintf(5,"ca(3): %.1f\n", ca);
        } else {
                /* Always keep the CSVP = battery_voltage so it doesnt keep pushing power to the batts */
                /* XXX keeping the CSVP < battery (even - 0.1) will always draw from batt and turn grid off */
                /* XXX need max_charge_amps = never go above */
                /* Add solar output to charge amps? */
                if (si.info.GdOn | si.info.GnRn) cv = si.info.battery_voltage;
                else cv = si.info.battery_voltage + 1.0;
                ca = si.charge_amps;
        }

	if (si.charge_mode) {
		cv = si.charge_voltage;
		if (si.info.GdOn) {
			if (si.charge_from_grid || si.force_charge) ca = si.grid_charge_amps;
			else ca = si.charge_min_amps;
		} else if (si.info.GnOn) {
			ca = si.gen_charge_amps;
		} else {
			cv += 1.0;
			ca = si.std_charge_amps;
		}

		/* Apply modifiers to charge amps */
		dprintf(5,"cv: %.1f\n", cv);
		dprintf(6,"ca(1): %.1f\n", ca);
		ca *= si.charge_amps_temp_modifier;
		dprintf(6,"ca(2): %.1f\n", ca);
		ca *= si.charge_amps_soc_modifier;
		dprintf(5,"ca(3): %.1f\n", ca);
	} else {
		/* make sure no charge goes into batts */
		cv = si.info.battery_voltage - 0.1;
		ca = 0.0;
	}

	var data = [];
	putshort(data,0,cv * 10.0);
	putshort(data,2,ca * 10.0)
	putshort(data,4,si.discharge_amps * 10.0)
	putshort(data,6,si.min_voltage * 10.0);
	return (si.can_write(0x351,data));
}

dprintf(1,"getting relays...\n");
si.get_relays();
si_write_va();

printf("GdOn: %s, grid_connected: %s, GnOn: %s, gen_connected: %s\n", si.info.GdOn, si.grid_connected, si.info.GnOn, si.gen_connected);
if (si.info.GdOn != si.grid_connected) {
	si.grid_connected = si.info.GdOn;
	printf("Grid %s\n",(si.grid_connected ? "connected" : "disconnected"));
}
if (si.info.GnOn != si.gen_connected) {
	si.gen_connected = si.info.GnOn;
	printf("Generator %s\n",(si.gen_connected ? "connected" : "disconnected"));
}

// Flag for read/write
run_completed = true;

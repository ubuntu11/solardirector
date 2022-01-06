
printf("===> WRITE\n");

if (0 == 1) {
        /* Adjust SOC so GdSocEna and GnSocEna dont disconnect until we're done charging */
        dprintf(1,"grid_connected: %d, charge_mode: %d, grid_soc_stop: %.1f, soc: %.1f\n",
                si.grid_connected,si.charge_mode,si.grid_soc_stop,si.soc);
        dprintf(1,"gen_connected: %d, charge_mode: %d, gen_soc_stop: %.1f, soc: %.1f\n",
                si.gen_connected,si.charge_mode,si.gen_soc_stop,si.soc);
        if (si.grid_connected && si.charge_mode && si.grid_soc_stop > 1.0 && si.soc >= si.grid_soc_stop)
                si.soc = si.grid_soc_stop - 1;
        else if (si.gen_connected && si.charge_mode && si.gen_soc_stop > 1.0 && si.soc >= si.gen_soc_stop)
                si.soc = si.gen_soc_stop - 1;
        if (si.data.battery_voltage != si.last_battery_voltage || si.soc != si.last_soc) {
                log_info("%s%sBattery Voltage: %.1fV, SoC: %.1f%%\n",
                        (si.data.Run ? "[Running] " : ""), (si.sim ? "(SIM) " : ""), si.data.battery_voltage, si.soc);
                si.last_battery_voltage = si.data.battery_voltage;
                si.last_soc = si.soc;
        }
}

exit(0);

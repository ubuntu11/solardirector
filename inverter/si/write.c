
#include "si.h"

int si_write(si_session_t *s, void *buf, int buflen) {
	solard_inverter_t *inv = buf;
	uint8_t data[8];

	dprintf(1,"0x351: charge_voltage: %3.2f, charge_amps: %3.2f, discharge_voltage: %3.2f, discharge_amps: %3.2f\n",
		inv->charge_voltage, inv->charge_amps, inv->discharge_voltage, inv->discharge_amps);
	si_putshort(&data[0],(inv->charge_voltage * 10.0));
	si_putshort(&data[2],(inv->charge_amps * 10.0));
	si_putshort(&data[4],(inv->discharge_amps * 10.0));
	si_putshort(&data[6],(inv->discharge_voltage * 10.0));
//	if (s->can->write(s->can_handle,0x351,&data,8) < 0) return 1;

	dprintf(1,"0x355: SOC: %.1f, SOH: %.1f\n", inv->soc, inv->soh);
	si_putshort(&data[0],inv->soc);
	si_putshort(&data[2],inv->soh);
	si_putlong(&data[4],(inv->soc * 100.0));
//	if (s->can->write(s->can_handle,0x355,&data,8) < 0) return 1;

#if 0
	dprintf(1,"0x356: battery_voltage: %3.2f, battery_current: %3.2f, battery_temp: %3.2f\n",
		inv->battery_voltage, inv->battery_current, inv->battery_temp);
	si_putshort(&data[0],inv->battery_voltage * 100.0);
	si_putshort(&data[2],inv->battery_current * 10.0);
	si_putshort(&data[4],inv->battery_temp * 10.0);
//	if (s->can->write(s->can_handle,0x356,&data,8) < 0) return 1;

	/* Alarms/Warnings */
	memset(data,0,sizeof(data));
//	if (s->can->write(s->can_handle,0x35A,&data,8) < 0) return 1;

	/* Events */
	memset(data,0,sizeof(data));
//	if (s->can->write(s->can_handle,0x35B,&data,8)) return 1;

	/* MFG Name */
	memset(data,' ',sizeof(data));
#define MFG_NAME "RSW"
	memcpy(data,MFG_NAME,strlen(MFG_NAME));
//	if (s->can->write(s->can_handle,0x35E,&data,8) < 0) return 1;

	/* 0x35F - Bat-Type / BMS Version / Bat-Capacity / reserved Manufacturer ID */
	si_putshort(&data[0],1);
	/* major.minor.build.revision - encoded as MmBBr 10142 = 1.0.14.2 */
	si_putshort(&data[2],10000);
	si_putshort(&data[4],inv->battery_capacity);
	si_putshort(&data[6],1);
//	if (s->can->write(s->can_handle,0x35F,&data,8) < 0) return 1;
#endif

	return 0;
}

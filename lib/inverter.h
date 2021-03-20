
#ifndef __SOLARD_INVERTER_H
#define __SOLARD_INVERTER_H

#include "agent.h"

typedef struct inverter_session inverter_session_t;

#define INVERTER_NAME_LEN 32

struct solard_inverter {
	float charge_voltage;
	float discharge_voltage;
	float charge_amps;
	float discharge_amps;
	float battery_voltage;		/* Really? I need to comment this? */
	float battery_current;		/* batt power, in amps */
	float battery_temp;		/* batt temp */
	float battery_power;		/* batt power, in watts */
	float battery_capacity;		/* battery capacity, in AH */
	float grid_power;		/* Grid/Gen watts */
	float load_power;		/* loads watts */
	float site_power;		/* pv/wind/caes/chp watts */
	float soc;
	float soh;
	int error;			/* Inverter Error code, 0 if none */
	char errmsg[256];		/* Inverter Error message */
	void *handle;			/* Inverter Handle */
	uint16_t state;			/* Inverter State */
};
typedef struct solard_inverter solard_inverter_t;
typedef struct solard_inverter inverter_t;

/* States */
#define SOLARD_INVERTER_STATE_OPEN 0x01
#define SOLARD_INVERTER_STATE_UPDATED	0x01
#define SOLARD_INVERTER_STATE_RUNNING	0x02
#define SOLARD_INVERTER_STATE_GRID	0x04
#define SOLARD_INVERTER_STATE_GEN	0x08
#define SOLARD_INVERTER_STATE_CHARGING	0x10

extern module_t inverter;

#endif /* __SOLARD_INVERTER_H */

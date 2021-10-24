
/*
Copyright (c) 2021, Stephen P. Shoecraft
All rights reserved.

This source code is licensed under the BSD-style license found in the
LICENSE file in the root directory of this source tree.
*/

#ifndef __SOLARD_INVERTER_H
#define __SOLARD_INVERTER_H

#include "agent.h"

typedef struct inverter_session inverter_session_t;

#define INVERTER_ID_LEN 64
#define INVERTER_NAME_LEN 32

enum INVERTER_TYPE {
	INVERTER_TYPE_PVTODC,		/* PV panels to DC (charge controller) */
	INVERTER_TYPE_PVTOAC,		/* PV panels to AC (sunny boy/etc) */
	INVERTER_TYPE_BATTOAC,		/* Battery to AC (battery inverter) */
	INVERTER_TYPE_ALL,		/* All functions in 1 unit */
};

struct solard_inverter {
	char name[INVERTER_NAME_LEN];
	enum INVERTER_TYPE type;
	float system_voltage;		/* System nominal voltage (48, 400, whatever) */
	float max_voltage;		/* Dont go above this voltage */
	float min_voltage;		/* Dont go below this voltage */
	float charge_start_voltage;	/* Voltage to start charging (empty voltage) */
	float charge_end_voltage;	/* Voltage to end charging (full voltage) */
	int charge_at_max;		/* Charge at max_voltage until battery_voltage >= charge_end_voltage */
	float charge_amps;
	float discharge_amps;
	float battery_voltage;
	float battery_amps;
	float battery_watts;
	float battery_temp;
	float soc;			/* State of Charge */
	float soh;			/* State of Health */
	solard_power_t grid_voltage;	/* Grid/Gen power */
	float grid_frequency;
	solard_power_t grid_amps;
	solard_power_t grid_watts;
	solard_power_t load_voltage;	/* Load power */
	float load_frequency;
	solard_power_t load_amps;
	solard_power_t load_watts;
	solard_power_t site_voltage;	/* On-site power source (PV/Wind/CAES/etc) */
	float site_frequency;
	solard_power_t site_amps;
	solard_power_t site_watts;
	int errcode;			/* Inverter Error code, 0 if none */
	char errmsg[256];		/* Inverter Error message */
	uint16_t state;			/* Inverter State */
	long last_update;
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

void inverter_dump(solard_inverter_t *,int);
int inverter_from_json(solard_inverter_t *, char *);
json_value_t *inverter_to_json(solard_inverter_t *);
int inverter_check_parms(solard_inverter_t *);

#endif /* __SOLARD_INVERTER_H */

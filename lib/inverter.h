
#ifndef __SOLARD_INVERTER_H
#define __SOLARD_INVERTER_H

#include "agent.h"

typedef struct inverter_session inverter_session_t;

#define INVERTER_ID_LEN 64
#define INVERTER_NAME_LEN 32

enum INVERTER_TYPE {
	INVERTER_TYPE_PVTODC,		/* PV panels to DC (charge controller) */
	INVERTER_TYPE_PVTOAC,		/* PV panels to AC (sunny boy/etc) */
	INVERTER_TYPE_DCTOAC,		/* Battery to AC (battery inverter) */
	INVERTER_TYPE_ALL,		/* All functions in 1 unit */
};

struct solard_inverter {
	char id[INVERTER_ID_LEN];
	char name[INVERTER_NAME_LEN];
	enum INVERTER_TYPE type;
	float charge_voltage;
	float charge_max_voltage;	/* inc voltage to this in order to mainint charge amps */
	int charge_at_max;		/* Always charge at max voltage */
	float charge_amps;
	float discharge_voltage;
	float discharge_amps;
	float battery_voltage;		/* Really? I need to comment this? */
	float battery_current;		/* batt power, in amps */
	float battery_power;		/* batt power, in watts */
	float battery_temp;		/* batt temp */
	float soc;			/* State of Charge */
	float soh;			/* State of Health */
	solard_power_t grid_voltage;
	float grid_frequency;
	solard_power_t grid_current;
	solard_power_t grid_watts;
	solard_power_t load_voltage;
	float load_frequency;
	solard_power_t load_current;
	solard_power_t load_watts;
	solard_power_t pv_voltage;
	solard_power_t pv_current;
	solard_power_t pv_watts;
	double yeild;			/* Total PV yeild */
	int errcode;			/* Inverter Error code, 0 if none */
	char errmsg[256];		/* Inverter Error message */
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
void inverter_dump(solard_inverter_t *,int);
int inverter_from_json(solard_inverter_t *, char *);
json_value_t *inverter_to_json(solard_inverter_t *);

#endif /* __SOLARD_INVERTER_H */

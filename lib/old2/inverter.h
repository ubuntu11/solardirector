
#ifndef __SOLARD_INVERTER_H
#define __SOLARD_INVERTER_H

#include "agent.h"

struct solard_inverter {
	solard_agent_t *conf;
	char name[32];			/* Inverter name */
	char uuid[37];			/* Inverter UUID */
	float battery_voltage;		/* Really? I need to comment this? */
	float battery_current;		/* batt power, in amps */
	float battery_power;		/* batt power, in watts */
	float grid_power;		/* Grid/Gen watts */
	float load_power;		/* loads watts */
	float site_power;		/* pv/wind/caes/chp watts */
	int error;			/* Inverter Error code, 0 if none */
	char errmsg[256];		/* Inverter Error message */
	void *handle;			/* Inverter Handle */
	uint16_t state;			/* Inverter State */
	uint16_t capabilities;		/* Capability bits */
};
typedef struct solard_inverter solard_inverter_t;
typedef struct solard_inverter inverter_t;

/* States */
#define SOLARD_INVERTER_STATE_UPDATED	0x01
#define SOLARD_INVERTER_STATE_RUNNING	0x02
#define SOLARD_INVERTER_STATE_GRID	0x04
#define SOLARD_INVERTER_STATE_GEN	0x08
#define SOLARD_INVERTER_STATE_CHARGING	0x10

/* Capabilities */
#define SOLARD_INVERTER_GRID_CONTROL	0x01
#define SOLARD_INVERTER_GEN_CONTROL	0x02
#define SOLARD_INVERTER_POWER_CONTROL	0x04

#if 0
int inverter_add(sdagent_config_t *conf, solard_inverter_t *inv);
int inverter_init(sdagent_config_t *conf);
int inverter_read(solard_inverter_t *inv);
int inverter_write(solard_inverter_t *inv);
#endif
int inverter_get_config(solard_agent_t *conf);

extern module_t inverter;

#endif /* __SOLARD_INVERTER_H */

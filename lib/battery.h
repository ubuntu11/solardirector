
#ifndef __SD_BATTERY_H
#define __SD_BATTERY_H

#include "agent.h"

enum CELL_STATUS {
	CELL_STATUS_OK,
	CELL_STATUS_WARNING,
	CELL_STATUS_ERROR,
	CELL_STATUS_UNDERVOLT,
	CELL_STATUS_OVERVOLT
};

struct battery_cell {
	char *label;
	enum CELL_STATUS status;
	float voltage;
	float resistance;
};
typedef struct battery_cell battery_cell_t;

#define BATTERY_ID_LEN 38
#define BATTERY_NAME_LEN 32
#define BATTERY_MAX_TEMPS 8
#define BATTERY_MAX_CELLS 32

struct solard_battery {
	char uuid[BATTERY_ID_LEN+1];		/* Battery unique ID */
	char name[BATTERY_NAME_LEN+1];	/* Battery name */
	float capacity;			/* Battery pack capacity, in AH */
	float voltage;			/* Pack voltage */
	float current;			/* Pack current */
	int ntemps;			/* Number of temps */
	float temps[BATTERY_MAX_TEMPS];	/* Temp values */
	int ncells;			/* Number of cells, updated by BMS */
//	battery_cell_t cells[BATTERY_MAX_CELLS];		/* Cell info */
	float cellvolt[BATTERY_MAX_CELLS]; /* Per-cell voltages, updated by BMS */
	float cellres[BATTERY_MAX_CELLS]; /* Per-cell resistances, updated by BMS */
	float cell_min;
	float cell_max;
	float cell_diff;
	float cell_avg;
	float cell_total;
	uint32_t balancebits;		/* Balance bitmask */
	int errcode;			/* Battery status, updated by agent */
	char errmsg[256];		/* Error message if status !0, updated by agent */
	uint16_t state;			/* Battery state */
	time_t last_update;		/* Set by client */
};
typedef struct solard_battery solard_battery_t;

/* Battery states */
#define BATTERY_STATE_UPDATED	0x01
#define BATTERY_STATE_CHARGING	0x02
#define BATTERY_STATE_DISCHARGING	0x04
#define BATTERY_STATE_BALANCING	0x08
#define BATTERY_STATE_OPEN		0x10

//solard_battery_t *battery_init(int,char **,opt_proctab_t *opts, module_t *mp);
//int battery_init(solard_battery_t *,char *);

#if 0
int pack_update(solard_battery_t *pp);
int pack_update_all(sdagent_config_t *,int);
int pack_add(sdagent_config_t *conf, char *packname, solard_battery_t *pp);
int pack_init(sdagent_config_t *conf);
int pack_check(sdagent_config_t *conf,solard_battery_t *pp);
int pack_send_mqtt(solard_battery_t *pp);

//int battery_get_config(solard_battery_t *, char *);

char *battery_info(solard_agent_t *);
int battery_update(solard_agent_t *);
//int battery_write(solard_agent_t *);
int battery_config(solard_agent_t *,char *,list);
int battery_control(solard_agent_t *);
int battery_process(solard_agent_t *, solard_message_t *);
int battery_send_status(solard_agent_t *,char *,int,char *);
#endif

extern solard_module_t battery;

#endif

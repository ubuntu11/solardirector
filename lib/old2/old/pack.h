
#ifndef __PACK_H
#define __PACK_H

#include "agent.h"
#include "module.h"

#define PACK_NAME_LEN 32
#define PACK_MAX_TEMPS 8
#define PACK_MAX_CELLS 32

struct battery_cell {
	char *label;
	unsigned char status;
	float voltage;
	float resistance;
};

#define CELL_STATUS_OK 		0x00
#define CELL_STATUS_WARNING 	0x01
#define CELL_STATUS_ERROR 	0x02
#define CELL_STATUS_UNDERVOLT	0x40
#define CELL_STATUS_OVERVOLT	0x80

struct solard_pack {
	sdagent_config_t *conf;		/* Agent back ptr */
	char name[PACK_NAME_LEN];	/* Pack name */
	char uuid[37];			/* UUID */
	char type[32];			/* BMS name */
	char transport[32];		/* Transport name */
	char target[32];		/* Transport target */
	char opts[64];			/* Pack-specific options */
	uint16_t state;			/* Pack state */
	int failed;			/* Update fail count */
	int error;			/* Error code, from BMS */
	char errmsg[256];		/* Error message, updated by BMS */
	float capacity;			/* Battery pack capacity, in AH */
	float voltage;			/* Pack voltage */
	float current;			/* Pack current */
	int status;			/* Pack status, updated by BMS */
	int ntemps;			/* Number of temps */
	float temps[PACK_MAX_TEMPS];	/* Temp values */
	int cells;			/* Number of cells, updated by BMS */
//	battery_cell_t *cells;		/* Cell info */
	float cellvolt[PACK_MAX_CELLS]; /* Per-cell voltages, updated by BMS */
	float cellres[PACK_MAX_CELLS]; /* Per-cell resistances, updated by BMS */
	float cell_min;
	float cell_max;
	float cell_diff;
	float cell_avg;
	uint32_t balancebits;		/* Balance bitmask */
	uint16_t capabilities;		/* BMS Capability Mask */
	void *handle;			/* BMS Handle */
	solard_module_open_t open;	/* BMS Open */
	solard_module_read_t read;	/* BMS Read */
	solard_module_close_t close;	/* BMS Close */
	solard_module_control_t control;	/* BMS Control */
	char timestamp[32];		/* Last update timestamp */
};
typedef struct solard_pack solard_pack_t;

/* Pack states */
#define SOLARD_PACK_STATE_UPDATED	0x01
#define SOLARD_PACK_STATE_CHARGING	0x02
#define SOLARD_PACK_STATE_DISCHARGING	0x04
#define SOLARD_PACK_STATE_BALANCING	0x08

int pack_update(solard_pack_t *pp);
int pack_update_all(sdagent_config_t *,int);
int pack_add(sdagent_config_t *conf, char *packname, solard_pack_t *pp);
int pack_init(sdagent_config_t *conf);
int pack_check(sdagent_config_t *conf,solard_pack_t *pp);
int pack_send_mqtt(sdagent_config_t *conf,solard_pack_t *pp);


#endif

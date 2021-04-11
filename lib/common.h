
#ifndef __SD_COMMON_H
#define __SD_COMMON_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <stdarg.h>
#include <stdint.h>
#include <errno.h>
#include <time.h>

#include "types.h"
#include "opts.h"
#include "cfg.h"
#include "message.h"
#include "json.h"
#include "utils.h"
#include "debug.h"
#include "state.h"
#include "mqtt.h"

#define SOLARD_TRANSPORT_LEN	32
#define SOLARD_TARGET_LEN	64
#define SOLARD_TOPTS_LEN	32
#define SOLARD_TOPIC_SIZE	256	/* Max topic length */

int solard_common_init(int argc,char **argv,opt_proctab_t *add_opts,int start_opts);

/* TOPIC / ROLE / NAME / FUNC */
#define SOLARD_TOPIC_ROOT	"SolarD"

#define SOLARD_ROLE_CONTROLLER	"Controller"
#define SOLARD_ROLE_BATTERY	"Battery"
#define SOLARD_ROLE_INVERTER	"Inverter"
#define SOLARD_ROLE_CONSUMER	"Consumer"

#define SOLARD_FUNC_INFO	"Info"
#define SOLARD_FUNC_DATA	"Data"
#define SOLARD_FUNC_CONFIG	"Config"
#define SOLARD_FUNC_CONTROL	"Control"

#define SOLARD_ACTION_GET	"Get"
#define SOLARD_ACTION_SET	"Set"

#define SOLARD_ID_STATUS	"Status"

struct solard_power {
	union {
		float l1;
		float a;
	};
	union {
		float l2;
		float b;
	};
	union {
		float l3;
		float c;
	};
	float total;
};
typedef struct solard_power solard_power_t;

#endif

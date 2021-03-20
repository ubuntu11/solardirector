
#ifndef __CELLMON_H
#define __CELLMON_H

#include "client.h"

struct cellmon_config {
	solard_client_t *c;
	int cells;
	list packs;
	int state;
};
typedef struct cellmon_config cellmon_config_t;

#define BATTERY_MAX_PACKS 32

void display(cellmon_config_t *);
void web_display(cellmon_config_t *, int);

#endif

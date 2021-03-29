
#ifndef __SOLARD_H
#define __SOLARD_H

#include "client.h"

struct solard_config {
	solard_client_t *c;
	solard_inverter_t *inv;
	list packs;
	list agents;
	int state;
};
typedef struct solard_config solard_config_t;

int solard_pubinfo(solard_config_t *conf);

#endif

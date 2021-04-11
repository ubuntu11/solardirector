
#ifndef __SOLARD_H
#define __SOLARD_H

#include "client.h"

struct solard_config {
	solard_client_t *c;
	solard_inverter_t *inv;
	list inverters;
	list packs;
	list agents;
	int state;
};
typedef struct solard_config solard_config_t;

int solard_pubinfo(solard_config_t *conf);
void getinv(solard_config_t *conf, char *name, char *data);
void getpack(solard_config_t *conf, char *name, char *data);

#endif


#ifndef __SD_CLIENT_H
#define __SD_CLIENT_H

#include "agent.h"

struct solard_client {
	char topic[128];
	char name[64];
	char id[38];
	mqtt_session_t *m;
	pthread_mutex_t lock;
	list messages;
	cfg_info_t *cfg;
	char *temp;
};
typedef struct solard_client solard_client_t;

solard_client_t *client_init(int,char **,opt_proctab_t *,char *,char *);
char *client_get_config(solard_client_t *cp, char *name, int timeout, int direct);
int client_set_config(solard_client_t *cp, char *name, char *value, int timeout);
list client_get_mconfig(solard_client_t *cp, int count, char **names, int timeout);

#endif

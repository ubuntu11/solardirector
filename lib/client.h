
#ifndef __SD_CLIENT_H
#define __SD_CLIENT_H

#include "agent.h"

#define CLIENT_ID_LEN 64
#define CLIENT_NAME_LEN 32

struct solard_client {
	char id[CLIENT_ID_LEN];
	char name[CLIENT_NAME_LEN];
	mqtt_session_t *m;
	pthread_mutex_t lock;
	list messages;
	cfg_info_t *cfg;
	char data[262144];
};
typedef struct solard_client solard_client_t;

solard_client_t *client_init(int,char **,opt_proctab_t *,char *,char *);
char *client_get_config(solard_client_t *cp, char *target, char *param, int timeout, int direct);
list client_get_mconfig(solard_client_t *cp, char *target, int count, char **params, int timeout);
int client_set_config(solard_client_t *cp, char *target, char *param, char *value, int timeout);

#endif

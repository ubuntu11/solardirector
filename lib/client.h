
/*
Copyright (c) 2021, Stephen P. Shoecraft
All rights reserved.

This source code is licensed under the BSD-style license found in the
LICENSE file in the root directory of this source tree.
*/

#ifndef __SD_CLIENT_H
#define __SD_CLIENT_H

#include "agent.h"

struct solard_client {
	char id[SOLARD_ID_LEN];			/* Same as MQTT ClientID */
	char name[SOLARD_NAME_LEN];		/* Client name */
	mqtt_config_t mqtt_config;
	mqtt_session_t *m;
	list messages;
	cfg_info_t *cfg;
	char section_name[CFG_SECTION_NAME_SIZE];
	char data[262144];
};
typedef struct solard_client solard_client_t;

solard_client_t *client_init(int,char **,opt_proctab_t *,char *);
char *client_get_config(solard_client_t *cp, char *op, char *target, char *param, int timeout, int direct);
list client_get_mconfig(solard_client_t *cp, char *op, char *target, int count, char **params, int timeout);
int client_set_config(solard_client_t *cp, char *op, char *target, char *param, char *value, int timeout);

#endif

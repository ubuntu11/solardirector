
/*
Copyright (c) 2021, Stephen P. Shoecraft
All rights reserved.

This source code is licensed under the BSD-style license found in the
LICENSE file in the root directory of this source tree.
*/

#ifndef __SD_CLIENT_H
#define __SD_CLIENT_H

#include "agent.h"

struct solard_client;

struct client_agentinfo {
	struct solard_client *c;
	char id[SOLARD_ID_LEN];
	char name[SOLARD_NAME_LEN];
	char role[SOLARD_ROLE_LEN];
	char target[SOLARD_ROLE_LEN+SOLARD_NAME_LEN+1];
	list aliases;
	list funcs;
	uint16_t state;
	int online;
	json_value_t *info;
	json_value_t *config;
	config_t *cp;
	void *data;
	int status;
	char errmsg[1024];
	list mq;
};
typedef struct client_agentinfo client_agentinfo_t;

#define CLIENT_AGENTINFO_REPLY	0x0001		/* Function called, expecting a reply */
#define CLIENT_AGENTINFO_STATUS	0x0002		/* Status/errmsg set */

struct solard_client {
	char name[SOLARD_NAME_LEN];		/* Client name */
	char section_name[CFG_SECTION_NAME_SIZE]; /* Config section name */
#ifdef MQTT
	mqtt_session_t *m;			/* MQTT session */
	bool config_from_mqtt;
	int mqtt_init;
	list mq;				/* Messages */
#endif
#ifdef INFLUX
	influx_session_t *i;			/* Influx session */
#endif
	config_t *cp;				/* Config */
	list agents;				/* Agents */
#ifdef JS
	JSPropertySpec *props;
	int rtsize;
	int stacksize;
	JSEngine *js;
	jsval config_val;
	jsval mqtt_val;
	jsval influx_val;
#endif
	char script_dir[SOLARD_PATH_MAX];
	char init_script[SOLARD_PATH_MAX];
	char start_script[SOLARD_PATH_MAX];
	char stop_script[SOLARD_PATH_MAX];
};
typedef struct solard_client solard_client_t;

solard_client_t *client_init(int argc,char **argv,char *,opt_proctab_t *client_opts,char *name,config_property_t *props,config_function_t *funcs);

client_agentinfo_t *client_getagentbyname(solard_client_t *c, char *name);
client_agentinfo_t *client_getagentbyid(solard_client_t *c, char *name);
config_function_t *client_getagentfunc(client_agentinfo_t *info, char *name);
int client_callagentfunc(client_agentinfo_t *info, config_function_t *f, int nargs, char **args);
int client_callagentfuncbyname(client_agentinfo_t *info, char *name, int nargs, char **args);
int client_set_config(solard_client_t *cp, char *op, char *target, char *param, char *value, int timeout);
int client_config_init(solard_client_t *c, config_property_t *client_props, config_function_t *client_funcs);
int client_mqtt_init(solard_client_t *c);
int client_matchagent(client_agentinfo_t *info, char *target);
int client_getagentstatus(client_agentinfo_t *info, solard_message_t *msg);
char *client_getagentrole(client_agentinfo_t *info);

#ifdef JS
int client_jsinit(JSEngine *e, void *priv);
#endif

#endif


/*
Copyright (c) 2021, Stephen P. Shoecraft
All rights reserved.

This source code is licensed under the BSD-style license found in the
LICENSE file in the root directory of this source tree.
*/

#ifndef __SOLARD_H
#define __SOLARD_H

#include "agent.h"
#include "client.h"
#include "battery.h"
#include "inverter.h"

struct solard_agentinfo {
	char index[16];				/* Agent index used in cfg */
	char id[SOLARD_ID_LEN];			/* Unique agent ID */
	char agent[SOLARD_NAME_LEN];		/* Agent name */
	char path[256];				/* Full path to agent */
	char role[SOLARD_ROLE_LEN];		/* Agent role */
	char name[SOLARD_NAME_LEN];		/* Agent instance name */
	char transport[SOLARD_TRANSPORT_LEN];
	char target[SOLARD_TARGET_LEN];
	char topts[SOLARD_TOPTS_LEN];
	void *data;				/* Agent instance data (battery/inverter/etc) */
	int managed;				/* Do we manage this agent (start/stop)? */
	uint16_t state;				/* Agent state */
	pid_t pid;
	time_t started;
};
typedef struct solard_agentinfo solard_agentinfo_t;

#define AGENTINFO_STATUS_STARTED 0x01		/* Agent has started */
#define AGENTINFO_STATUS_WARNED 0x02		/* Agent has not reported */
#define AGENTINFO_STATUS_ERROR 0x04		/* Agent is gone */
#define AGENTINFO_STATUS_MASK 0x0F

#define AGENTINFO_NOTIFY_GONE 0x10		/* Agent has not reported since XXX */
#define AGENTINFO_NOTIFY_VOLT 0x20		/* bat/inv volt out of range */
#define AGENTINFO_NOTIFY_TEMP 0x40		/* bat/inv temp out of range */

struct solard_config {
	solard_agent_t *ap;
	solard_client_t *c;
	list inverters;
	list batteries;
	list agents;
	int state;
	int interval;				/* Agent check interval */
	int agent_warning;			/* Warning, in seconds, when agent dosnt respond */
	int agent_error;			/* In seconds, when agent considered lost */
	int agent_notify;			/* In seconds, when monitoring should notify */
	int status;
	char errmsg[128];
	time_t last_check;			/* Last time agents were checked */
	long start;
	char notify_path[256];
#ifdef JS
	JSPropertySpec *props;
#endif
};
typedef struct solard_config solard_config_t;

extern solard_driver_t sd_driver;

int solard_agent_init(int argc, char **argv, opt_proctab_t *sd_opts, solard_config_t *conf);

json_value_t *sd_get_info(void *handle);

int solard_read_config(solard_config_t *conf);
int solard_write_config(solard_config_t *conf);
int solard_send_status(solard_config_t *conf, char *func, char *action, char *id, int status, char *message);
int solard_config(void *, int, ...);
int solard_add_config(solard_config_t *conf, char *label, char *value, char *errmsg);


void getinv(solard_config_t *conf, client_agentinfo_t *ap);
void getpack(solard_config_t *conf, client_agentinfo_t *ap);
//void getinv(solard_config_t *conf, char *name, char *data);
//void getpack(solard_config_t *conf, char *name, char *data);
//void solard_setcfg(cfg_info_t *cfg, char *section_name, solard_agentinfo_t *info);

/* agents */
solard_agentinfo_t *agent_find(solard_config_t *conf, char *name);
int agent_get_role(solard_config_t *conf, solard_agentinfo_t *info);
void add_agent(solard_config_t *conf, char *role, json_value_t *v);
int check_agents(void *);
int agent_stop(solard_config_t *conf, solard_agentinfo_t *info);
void solard_monitor(solard_config_t *conf);

/* agentinfo */
json_value_t *agentinfo_to_json(solard_agentinfo_t *info);
void agentinfo_pub(solard_config_t *conf, solard_agentinfo_t *info);
void agentinfo_getcfg(cfg_info_t *cfg, char *sname, solard_agentinfo_t *info);
void agentinfo_setcfg(cfg_info_t *cfg, char *sname, solard_agentinfo_t *info);
int agentinfo_set(solard_agentinfo_t *, char *, char *);
solard_agentinfo_t *agentinfo_add(solard_config_t *conf, solard_agentinfo_t *info);
int agentinfo_get(solard_config_t *conf, char *);
void agentinfo_newid(solard_agentinfo_t *info);

#ifdef JS
int solard_jsinit(solard_config_t *);
#endif

#endif

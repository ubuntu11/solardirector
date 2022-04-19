
/*
Copyright (c) 2021, Stephen P. Shoecraft
All rights reserved.

This source code is licensed under the BSD-style license found in the
LICENSE file in the root directory of this source tree.
*/

#ifndef __SOLARD_H
#define __SOLARD_H

#include "agent.h"
//#include "client.h"
#include "battery.h"
#include "inverter.h"

struct solard_agentinfo {
	bool managed;
	char name[SOLARD_NAME_LEN];		/* Agent name */
	char path[256];				/* Full path to agent */
	char conf[256];				/* Full path to config file */
	char log[256];
	bool append;
	bool monitor_topic;
	char topic[SOLARD_TOPIC_SIZE];
	int action;				/* What action to take if the agent stops reporting */
	int notify;				/* Notify if the agent stops reporting */
	bool enabled;
	uint16_t state;				/* Agent state */
	pid_t pid;
	time_t started;
	time_t updated;
};
typedef struct solard_agentinfo solard_agentinfo_t;

#define AGENTINFO_STATUS_STARTED 0x01		/* Agent has started */
#define AGENTINFO_STATUS_WARNED 0x02		/* Agent has not reported */
#define AGENTINFO_STATUS_ERROR 0x04		/* Agent is gone */
#define AGENTINFO_STATUS_MASK 0x0F
#define AGENTINFO_NOTIFY_GONE 0x10		/* Agent has not reported since XXX */

struct solard_instance {
	solard_agent_t *ap;
	list names;
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
	int next_num;
};
typedef struct solard_instance solard_instance_t;

extern solard_driver_t sd_driver;

/* Name used in config props */
#define SD_SECTION_NAME "solard"

int solard_agent_init(int argc, char **argv, opt_proctab_t *sd_opts, solard_instance_t *sd);
json_value_t *sd_get_info(void *handle);

/* agents */
int agent_get_config(solard_instance_t *sd, solard_agentinfo_t *info);
int agent_set_config(solard_instance_t *sd, solard_agentinfo_t *info);
int agent_start(solard_instance_t *sd, solard_agentinfo_t *info);
int agent_stop(solard_instance_t *sd, solard_agentinfo_t *info);
void agent_check(solard_instance_t *sd, solard_agentinfo_t *info);

/* config */
int solard_config(void *h, int req, ...);
int solard_read_config(solard_instance_t *sd);
int solard_write_config(solard_instance_t *sd);

#if 0
int solard_send_status(solard_instance_t *sd, char *func, char *action, char *id, int status, char *message);
int solard_add_config(solard_instance_t *sd, char *label, char *value, char *errmsg);

//void getinv(solard_instance_t *sd, client_agentinfo_t *ap);
//void getpack(solard_instance_t *sd, client_agentinfo_t *ap);

#if 0
/* agents */
solard_agentinfo_t *agent_find(solard_instance_t *sd, char *name);
int agent_get_role(solard_instance_t *sd, solard_agentinfo_t *info);
void add_agent(solard_instance_t *sd, char *role, json_value_t *v);
void solard_monitor(solard_instance_t *sd);

/* agentinfo */
json_value_t *agentinfo_to_json(solard_agentinfo_t *info);
void agentinfo_pub(solard_instance_t *sd, solard_agentinfo_t *info);
void agentinfo_getcfg(cfg_info_t *cfg, char *sname, solard_agentinfo_t *info);
void agentinfo_setcfg(cfg_info_t *cfg, char *sname, solard_agentinfo_t *info);
int agentinfo_set(solard_agentinfo_t *, char *, char *);
solard_agentinfo_t *agentinfo_add(solard_instance_t *sd, solard_agentinfo_t *info);
int agentinfo_get(solard_instance_t *sd, char *);
void agentinfo_newid(solard_agentinfo_t *info);
#endif
#endif

#ifdef JS
int solard_jsinit(solard_instance_t *);
#endif

#endif


/*
Copyright (c) 2021, Stephen P. Shoecraft
All rights reserved.

This source code is licensed under the BSD-style license found in the
LICENSE file in the root directory of this source tree.
*/

#ifndef __SD_AGENT_H
#define __SD_AGENT_H

#include "common.h"
#include "driver.h"

//struct solard_agent;
typedef struct solard_agent solard_agent_t;
typedef struct solard_agent agent_t;
//typedef int (*solard_agent_callback_t)(solard_agent_t *);
typedef int (solard_agent_callback_t)(void *);

struct solard_agent {
	char name[SOLARD_NAME_LEN];
	cfg_info_t *cfg;
	mqtt_config_t mqtt_config;
	char section_name[CFG_SECTION_NAME_SIZE];
	mqtt_session_t *m;		/* MQTT Session handle */
	int config_from_mqtt;
	int read_interval;
	int write_interval;
//	list modules;			/* list of loaded modules */
	list mq;			/* incoming message queue */
	list mh;			/* Message handlers */
	uint16_t state;			/* States */
	int pretty;			/* Format json messages for readability (uses more mem) */
	solard_driver_t *role;
	void *role_handle;
	char instance_name[SOLARD_NAME_LEN]; /* Agent instance name */
	void *role_data;		/* Role-specific data */
	json_value_t *info;		/* Info returned by role/driver */
	struct {
		solard_agent_callback_t *func;	/* Called between read and write */
		void *ctx;
	} callback;
};

/* Agent states */
#define SOLARD_AGENT_RUNNING 0x01
#define SOLARD_AGENT_CONFIG_DIRTY 0x10

solard_agent_t *agent_init(int argc, char **argv, opt_proctab_t *agent_opts, solard_driver_t *, solard_driver_t **, solard_driver_t *);
int agent_run(solard_agent_t *ap);
//void agent_mktopic(char *topic, int topicsz, solard_agent_t *ap, char *name, char *func, char *action, char *id);
void agent_mktopic(char *topic, int topicsz, solard_agent_t *ap, char *name, char *func);
int agent_sub(solard_agent_t *ap, char *name, char *func);
int agent_pub(solard_agent_t *ap, char *func, char *action, char *id, char *message, int retain);
//int agent_send_status(solard_agent_t *ap, char *topic, char *name, char *func, char *op, char *clientid, int status, char *message);
int agent_reply(solard_agent_t *ap, char *topic, int status, char *message);
int agent_set_callback(solard_agent_t *, solard_agent_callback_t *, void *);
void agent_add_msghandler(solard_agent_t *, solard_msghandler_t *, void *);

void *agent_get_handle(solard_agent_t *);

#include "config.h"

#endif /* __SD_AGENT_H */

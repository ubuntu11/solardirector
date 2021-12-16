
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
#include "jsengine.h"

//struct solard_agent;
typedef struct solard_agent solard_agent_t;
typedef struct solard_agent agent_t;
typedef int (solard_agent_callback_t)(void *);

struct solard_agent {
	char name[SOLARD_NAME_LEN];
	config_t *cp;
	cfg_info_t *cfg;
	char section_name[CFG_SECTION_NAME_SIZE];
	solard_driver_t *driver;
	void *handle;
	mqtt_session_t *m;		/* MQTT Session handle */
	mqtt_config_t mqtt_config;
	int read_interval;
	int write_interval;
	list mq;			/* incoming message queue */
	list mh;			/* Message handlers */
	uint16_t state;			/* States */
	int pretty;			/* Format json messages for readability (uses more mem) */
	char instance_name[SOLARD_NAME_LEN]; /* Agent instance name */
	JSEngine *js;			/* JavaScript engine */
	char script_dir[256];
	char init_script[64];
	char start_script[64];
	char stop_script[64];
	char read_script[64];
	char write_script[64];
	char run_script[64];
	struct {
		solard_agent_callback_t *func;	/* Called between read and write */
		void *ctx;
	} callback;
#ifdef DEBUG_MEM
	int mem_start;
	int mem_last;
#endif
	char errmsg[128];
	JSPropertySpec *props;
	int mqtt_init;
	int open_before_read;
	int close_after_read;
	int open_before_write;
	int close_after_write;
};

/* Agent states */
#define SOLARD_AGENT_RUNNING 0x01
#define SOLARD_AGENT_CONFIG_DIRTY 0x10

solard_agent_t *agent_init(int argc, char **argv, opt_proctab_t *agent_opts,
		solard_driver_t *, void *, config_property_t *, config_function_t *);
int agent_run(solard_agent_t *ap);
void agent_mktopic(char *topic, int topicsz, solard_agent_t *ap, char *name, char *func);
int agent_sub(solard_agent_t *ap, char *name, char *func);
int agent_pub(solard_agent_t *ap, char *func, char *message, int retain);
int agent_reply(solard_agent_t *ap, char *topic, int status, char *message);
int agent_set_callback(solard_agent_t *, solard_agent_callback_t *, void *);
void agent_add_msghandler(solard_agent_t *, solard_msghandler_t *, void *);

void *agent_get_handle(solard_agent_t *);
int agent_start_script(solard_agent_t *ap, char *name, int bg, int newcx);
#define agent_run_script(a,n) agent_start_script(a,n,0)

#include "config.h"

#endif /* __SD_AGENT_H */

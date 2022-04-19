
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
#ifdef JS
#include "jsengine.h"
#endif

//struct solard_agent;
typedef struct solard_agent solard_agent_t;
typedef struct solard_agent agent_t;
typedef int (solard_agent_callback_t)(void *);

struct solard_agent {
	char name[SOLARD_NAME_LEN];
	config_t *cp;
	char section_name[CFG_SECTION_NAME_SIZE];
	solard_driver_t *driver;
	void *handle;
	json_value_t *info;
#ifdef MQTT
	mqtt_session_t *m;
	bool config_from_mqtt;
	char mqtt_topic[SOLARD_TOPIC_SIZE];
	list mq;			/* incoming message queue */
#endif
#ifdef INFLUX
	influx_session_t *i;
#endif
	int interval;
	uint16_t state;			/* States */
	int pretty;			/* Format json messages for readability (uses more mem) */
	char instance_name[SOLARD_NAME_LEN]; /* Agent instance name */
	char script_dir[SOLARD_PATH_MAX];
	char init_script[SOLARD_PATH_MAX];
	char start_script[SOLARD_PATH_MAX];
	char stop_script[SOLARD_PATH_MAX];
	char read_script[SOLARD_PATH_MAX];
	char write_script[SOLARD_PATH_MAX];
	char run_script[SOLARD_PATH_MAX];
	struct {
		solard_agent_callback_t *func;	/* Called between read and write */
		void *ctx;
	} callback;
	char errmsg[128];
	int mqtt_init;
	int open_before_read;
	int close_after_read;
	int open_before_write;
	int close_after_write;
#ifdef JS
	JSEngine *js;			/* JavaScript engine */
	int rtsize;
	int stksize;
	JSPropertySpec *props;
	jsval config_val;
	jsval mqtt_val;
	jsval influx_val;
	bool initial_gc;
#endif
	void *private;			/* Per-instance private data */
};

/* Agent states */
#define SOLARD_AGENT_RUNNING 0x01
#define SOLARD_AGENT_CONFIG_DIRTY 0x10

solard_agent_t *agent_new(void);
solard_agent_t *agent_init(int, char **, char *, opt_proctab_t *,
		solard_driver_t *, void *, config_property_t *, config_function_t *);
int agent_run(solard_agent_t *ap);
#ifdef MQTT
void agent_mktopic(char *topic, int topicsz, char *name, char *func);
int agent_sub(solard_agent_t *ap, char *name, char *func);
int agent_pub(solard_agent_t *ap, char *func, char *message, int retain);
int agent_pubinfo(solard_agent_t *ap, int disp);
int agent_reply(solard_agent_t *ap, char *topic, int status, char *message);
#endif
int agent_set_callback(solard_agent_t *, solard_agent_callback_t *, void *);
int agent_clear_callback(solard_agent_t *);

#include "config.h"

#ifdef JS
int agent_start_script(solard_agent_t *ap, char *name);
int agent_start_jsfunc(solard_agent_t *ap, char *name, char *func);
int agent_script_exists(solard_agent_t *ap, char *name);
#define agent_run_script(a,n) agent_start_script(a,n,0)
JSObject *jsagent_new(JSContext *cx, JSObject *parent, solard_agent_t *ap);
#endif

#endif /* __SD_AGENT_H */

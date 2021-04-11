
#ifndef __SD_AGENT_H
#define __SD_AGENT_H

#include "common.h"

//struct solard_agent;
typedef struct solard_agent solard_agent_t;
typedef struct solard_agent agent_t;
typedef int (*solard_agent_callback_t)(solard_agent_t *);

#include "module.h"
#include "battery.h"
#include "inverter.h"

#define SOLARD_AGENT_NAME_LEN 32
#define SOLARD_AGENT_TRANSPORT_LEN 8
#define SOLARD_AGENT_TARGET_LEN 64
#define SOLARD_UUID_LEN 38

struct solard_agent {
	char id[SOLARD_UUID_LEN];
	char name[SOLARD_AGENT_NAME_LEN];
	void *cfg;
	char section_name[64];		/* Configfile section name */
	mqtt_session_t *m;		/* MQTT Session handle */
	int read_interval;
	int write_interval;
	list modules;			/* list of loaded modules */
	list mq;			/* incoming message queue */
	uint16_t state;			/* States */
	int pretty;			/* Format json messages for readability (uses more mem) */
	module_t *role;
	void *role_handle;
	void *role_data;		/* Role-specific data */
	json_value_t *info;		/* Info returned by role/driver */
	solard_agent_callback_t cb;	/* Called between read and write */
};

/* Agent states */
#define SOLARD_AGENT_RUNNING 0x01
#define SOLARD_AGENT_CONFIG_DIRTY 0x10

/* Agent configuration request item */
struct solard_agent_config_req {
	char name[64];
	enum DATA_TYPE type;
	union {
		char sval[128];
		double dval;
		unsigned char bval;
	};
};
typedef struct solard_agent_config_req solard_confreq_t;

solard_agent_t *agent_init(int argc, char **argv, opt_proctab_t *agent_opts, solard_module_t *driver);
int agent_run(solard_agent_t *ap);
int agent_send_status(solard_agent_t *ap, char *name, char *func, char *op, char *clientid, int status, char *message);
int agent_set_callback(solard_agent_t *, solard_agent_callback_t);
void *agent_get_handle(solard_agent_t *);

#define agent_get_driver_handle(ap) ap->role->get_handle(ap->role_handle)

#endif /* __SD_AGENT_H */

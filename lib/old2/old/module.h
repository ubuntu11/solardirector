
#ifndef __SOLARD_MODULE_H
#define __SOLARD_MODULE_H

#include "agent.h"

typedef int (*solard_module_open_t)(void *);
typedef int (*solard_module_read_t)(void *,...);
typedef int (*solard_module_write_t)(void *,...);
typedef int (*solard_module_close_t)(void *);
typedef int (*solard_module_control_t)(void *,...);


struct solard_module {
	int type;
	char *name;
	unsigned short capabilities;
	int (*init)(sdagent_config_t *);
	void *(*new)(sdagent_config_t *,...);
	solard_module_open_t open;
	solard_module_read_t read;
	solard_module_write_t write;
	solard_module_close_t close;
	int (*free)(void *);
	int (*shutdown)(void *);
	solard_module_control_t control;
	solard_module_control_t config;
};
typedef struct solard_module solard_module_t;

#define SOLARD_MODULE_CAPABILITY_CONTROL 0x01
#define SOLARD_MODULE_CAPABILITY_CONFIG	0x02

solard_module_t *solard_load_module(sdagent_config_t *conf, char *name, int type);

enum SOLARD_MODTYPE {
        SOLARD_MODTYPE_INVERTER,
        SOLARD_MODTYPE_CELLMON,
        SOLARD_MODTYPE_TRANSPORT,
        SOLARD_MODTYPE_DB,
};

#ifndef EXPORT_API
#define EXPORT_API __attribute__ ((visibility("default")))
#endif 

#endif /* __SOLARD_MODULE_H */

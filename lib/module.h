
#ifndef __SOLARD_MODULE_H
#define __SOLARD_MODULE_H

struct solard_module;
typedef struct solard_module solard_module_t;
typedef struct solard_module module_t;

enum SOLARD_MODTYPE {
        SOLARD_MODTYPE_INVERTER,
        SOLARD_MODTYPE_BATTERY,
        SOLARD_MODTYPE_TRANSPORT,
};

#define SOLARD_MODTYPE_ANY 0xffff

typedef struct solard_agent solard_agent_t;

#include "list.h"
#include "json.h"

typedef int (*solard_module_init_t)(void *,char *);
//typedef void *(*solard_module_new_t)(void *,...);
typedef void *(*solard_module_new_t)(void *,void *,void *);
typedef char *(*solard_module_info_t)(void *);
typedef int (*solard_module_open_t)(void *);
#if 0
typedef int (*solard_module_read_t)(void *,...);
typedef int (*solard_module_write_t)(void *,...);
#endif
typedef int (*solard_module_read_t)(void *,void *,int);
typedef int (*solard_module_write_t)(void *,void *,int);
typedef int (*solard_module_close_t)(void *);
typedef int (*solard_module_free_t)(void *);
typedef int (*solard_module_shutdown_t)(void *);
typedef int (*solard_module_control_t)(void *,char *,char *,json_value_t *);
typedef int (*solard_module_config_t)(void *,char *,char *,list);
typedef void *(*solard_module_get_handle_t)(void *);
typedef void *(*solard_module_get_info_t)(void *);

#include "agent.h"

struct solard_module {
	int type;
	char *name;
	solard_module_init_t init;
	solard_module_new_t new;
	solard_module_info_t info;
	solard_module_open_t open;
	solard_module_read_t read;
	solard_module_write_t write;
	solard_module_close_t close;
	solard_module_free_t free;
	solard_module_shutdown_t shutdown;
	solard_module_control_t control;
	solard_module_config_t config;
	solard_module_get_handle_t get_handle;
	solard_module_get_handle_t get_info;
};

module_t *load_module(list lp, char *name, int type);

#ifndef EXPORT_API
#define EXPORT_API __attribute__ ((visibility("default")))
#endif 

#endif /* __SOLARD_MODULE_H */

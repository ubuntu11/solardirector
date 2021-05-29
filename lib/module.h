
/*
Copyright (c) 2021, Stephen P. Shoecraft
All rights reserved.

This source code is licensed under the BSD-style license found in the
LICENSE file in the root directory of this source tree.
*/

#ifndef __SOLARD_MODULE_H
#define __SOLARD_MODULE_H

struct solard_module;
typedef struct solard_module solard_module_t;

enum SOLARD_MODTYPE {
	SOLARD_MODTYPE_NONE,
        SOLARD_MODTYPE_CONTROLLER,
        SOLARD_MODTYPE_INVERTER,
        SOLARD_MODTYPE_BATTERY,
        SOLARD_MODTYPE_TRANSPORT,
};

#define SOLARD_MODTYPE_ANY 0xffff

typedef struct solard_agent solard_agent_t;

#include "list.h"
#include "json.h"

typedef int (*solard_module_init_t)(void *,char *);
typedef void *(*solard_module_new_t)(void *,void *,void *);
typedef json_value_t *(*solard_module_info_t)(void *);
typedef int (*solard_module_open_t)(void *);
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

solard_module_t *load_module(list lp, char *name, int type);
int load_tp_from_cfg(solard_module_t **mp, void **h, cfg_info_t *cfg, char *section_name);

extern solard_module_t null_transport;
#define SOLARD_DUMMY_MODULE(name) \
solard_module_t name_module = { \
	SOLARD_MODTYPE_NONE, \
	"name", \
	0,		/* Init */ \
	0,		/* New */ \
	0,		/* Info */ \
	0,		/* Open */ \
	0,		/* Read */ \
	0,		/* Write */ \
	0,		/* Close */ \
	0,		/* Free */ \
	0,		/* Shutdown */ \
	0,		/* Config */ \
	0,		/* Control */ \
};

#ifdef EXPORT
#undef EXPORT
#endif
#ifdef __WIN32
#define EXPORT __declspec(dllexport)
#else
#define EXPORT __attribute__ ((visibility("default")))
#endif

#endif /* __SOLARD_MODULE_H */


/*
Copyright (c) 2021, Stephen P. Shoecraft
All rights reserved.

This source code is licensed under the BSD-style license found in the
LICENSE file in the root directory of this source tree.
*/

#ifndef __SOLARD_DRIVER_H
#define __SOLARD_DRIVER_H

//struct solard_driver;
//typedef struct solard_driver solard_driver_t;

enum SOLARD_DRIVER {
	SOLARD_DRIVER_NONE,
        SOLARD_DRIVER_TRANSPORT,
        SOLARD_DRIVER_AGENT,
        SOLARD_DRIVER_ROLE,
};

//typedef struct solard_agent solard_agent_t;

#include "list.h"
#include "json.h"

typedef void *(*solard_driver_new_t)(void *,void *,void *);
typedef int (*solard_driver_open_t)(void *);
typedef int (*solard_driver_read_t)(void *,void *,int);
typedef int (*solard_driver_write_t)(void *,void *,int);
typedef int (*solard_driver_close_t)(void *);
typedef int (*solard_driver_config_t)(void *,int,...);

struct solard_driver {
	int type;
	char *name;
	solard_driver_new_t new;
	solard_driver_open_t open;
	solard_driver_close_t close;
	solard_driver_read_t read;
	solard_driver_write_t write;
	solard_driver_config_t config;
};
typedef struct solard_driver solard_driver_t;

/* Driver configuration requests */
enum SOLARD_CONFIG {
	SOLARD_CONFIG_INIT = 1,		/* Process json message */
	SOLARD_CONFIG_MESSAGE,
	SOLARD_CONFIG_GET_INFO,		/* Process json message */
	SOLARD_CONFIG_GET_DRIVER,		/* Get transport or target */
	SOLARD_CONFIG_SET_DRIVER,
	SOLARD_CONFIG_GET_HANDLE,		/* Get transport handle or topts */
	SOLARD_CONFIG_SET_HANDLE,
	SOLARD_CONFIG_MAX,
};

#define SOLARD_CONFIG_GET_TARGET SOLARD_CONFIG_GET_DRIVER
#define SOLARD_CONFIG_SET_TARGET SOLARD_CONFIG_SET_DRIVER
#define SOLARD_CONFIG_GET_TOPTS SOLARD_CONFIG_GET_HANDLE
#define SOLARD_CONFIG_SET_TOPTS SOLARD_CONFIG_SET_HANDLE

solard_driver_t *find_driver(solard_driver_t **transports, char *name);

#endif /* __SOLARD_DRIVER_H */

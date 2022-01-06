
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

/* Driver type */
enum SOLARD_DRIVER {
	SOLARD_DRIVER_NONE,
        SOLARD_DRIVER_TRANSPORT,
        SOLARD_DRIVER_AGENT,
};

//typedef struct solard_agent solard_agent_t;

#include "list.h"
#include "json.h"

typedef void *(*solard_driver_new_t)(void);
typedef int (*solard_driver_destroy_t)(void *);
typedef int (*solard_driver_open_t)(void *);
typedef int (*solard_driver_read_t)(void *,void *,int);
typedef int (*solard_driver_write_t)(void *,void *,int);
typedef int (*solard_driver_close_t)(void *);
typedef int (*solard_driver_config_t)(void *,int,...);

#define DRIVER_FLAG_OPEN_BEFORE_READ
#define DRIVER_FLAG_CLOSE_AFTER_READ
#define DRIVER_FLAG_OPEN_BEFORE_WRITE
#define DRIVER_FLAG_CLOSE_AFTER_WRITE

struct solard_driver {
	int type;
	char *name;
	void *(*new)(void *,void *,void *);
	solard_driver_destroy_t destroy;
	solard_driver_open_t open;
	solard_driver_close_t close;
	solard_driver_read_t read;
	solard_driver_write_t write;
	solard_driver_config_t config;
};
typedef struct solard_driver solard_driver_t;

/* Driver configuration requests */
enum SOLARD_CONFIG {
	SOLARD_CONFIG_INIT,		/* Init driver */
	SOLARD_CONFIG_GET_INFO,		/* Get driver info */
	SOLARD_CONFIG_GET_DRIVER,	/* Get driver */
	SOLARD_CONFIG_GET_HANDLE,	/* Get handle */
};

#if 0
#define SOLARD_CONFIG_GET_TARGET SOLARD_CONFIG_GET_DRIVER
#define SOLARD_CONFIG_SET_TARGET SOLARD_CONFIG_SET_DRIVER
#define SOLARD_CONFIG_GET_TOPTS SOLARD_CONFIG_GET_HANDLE
#define SOLARD_CONFIG_SET_TOPTS SOLARD_CONFIG_SET_HANDLE
#endif

#define DRIVER_TRANSPORT_SIZE 16
#define DRIVER_TARGET_SIZE 128
#define DRIVER_TOPTS_SIZE 64

solard_driver_t *find_driver(solard_driver_t **transports, char *name);

#endif /* __SOLARD_DRIVER_H */

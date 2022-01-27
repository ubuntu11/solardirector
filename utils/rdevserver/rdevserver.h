
/*
Copyright (c) 2021, Stephen P. Shoecraft
All rights reserved.

This source code is licensed under the BSD-style license found in the
LICENSE file in the root directory of this source tree.
*/

#ifndef __RDEVSERVER_H
#define __RDEVSERVER_H

#include "common.h"
#include "driver.h"
#include "rdev.h"
#include <pthread.h>
#include "can.h"

struct devserver_io {
	char name[RDEV_NAME_LEN];
	char type[RDEV_TYPE_LEN];
	void *handle;
	int (*open)(void *handle);
	int (*read)(void *handle, void *buf, int buflen);
	int (*write)(void *handle, void *buf, int buflen);
	int (*close)(void *handle);
};
typedef struct devserver_io devserver_io_t;

#define DEVSERVER_MAX_UNITS 8

struct devserver_config {
	pthread_mutex_t lock;
	devserver_io_t units[DEVSERVER_MAX_UNITS];
	int count;
};
typedef struct devserver_config devserver_config_t;

#define NFRAMES 2048
#define NBITMAPS (NFRAMES/32)

struct rdev_config {
	cfg_info_t *cfg;
	list modules;
	devserver_config_t ds;
	solard_driver_t *can;
	void *can_handle;
	pthread_t th;
	struct can_frame frames[NFRAMES];
	uint32_t bitmaps[NBITMAPS];
	uint16_t state;
};
typedef struct rdev_config rdev_config_t;

#define RDEV_STATE_RUNNING 0x01
#define RDEV_STATE_OPEN 0x10

int devserver_add_unit(devserver_config_t *, devserver_io_t *);
int devserver(devserver_config_t *,int);
int server(rdev_config_t *,int);

#endif

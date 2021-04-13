
/*
Copyright (c) 2021, Stephen P. Shoecraft
All rights reserved.

This source code is licensed under the BSD-style license found in the
LICENSE file in the root directory of this source tree.
*/

#ifndef __RDEVSERVER_H
#define __RDEVSERVER_H

#include "common.h"
#include "module.h"
#include "devserver.h"
#include <linux/can.h>

struct rdev_config {
	cfg_info_t *cfg;
	list modules;
	devserver_config_t ds;
	solard_module_t *can;
	void *can_handle;
	pthread_t th;
	struct can_frame frames[16];
	uint32_t bitmap;
	uint16_t state;
};
typedef struct rdev_config rdev_config_t;

#define RDEV_STATE_RUNNING 0x01
#define RDEV_STATE_OPEN 0x10

int server(rdev_config_t *,int);

#endif

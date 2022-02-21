
/*
Copyright (c) 2021, Stephen P. Shoecraft
All rights reserved.

This source code is licensed under the BSD-style license found in the
LICENSE file in the root directory of this source tree.
*/

#ifndef __SOLARD_INFLUX_H
#define __SOLARD_INFLUX_H

struct influx_session;
typedef struct influx_session influx_session_t;

#define INFLUX_HOST_SIZE 64
#define INFLUX_DATABASE_SIZE 64
#define INFLUX_USERNAME_SIZE 32
#define INFLUX_PASSWORD_SIZE 32
#define INFLUX_TOPIC_SIZE 128
#define INFLUX_MAX_MESSAGE_SIZE 262144

struct influx_config {
	char host[INFLUX_HOST_SIZE];
	int port;
	char database[INFLUX_DATABASE_SIZE];
	char username[INFLUX_USERNAME_SIZE];
	char password[INFLUX_PASSWORD_SIZE];
};
typedef struct influx_config influx_config_t;

influx_session_t *influx_new(void);

#ifdef JS
#include "jsapi.h"
#include "jsengine.h"
void influx_add_props(config_t *, influx_config_t *, char *, influx_config_t *);
int influx_jsinit(JSEngine *, influx_session_t *);
#endif

#endif

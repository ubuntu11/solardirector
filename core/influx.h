
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

#define INFLUX_ENDPOINT_SIZE 128
#define INFLUX_DATABASE_SIZE 64
#define INFLUX_USERNAME_SIZE 32
#define INFLUX_PASSWORD_SIZE 32

struct influx_config {
	char endpoint[INFLUX_ENDPOINT_SIZE];
	char database[INFLUX_DATABASE_SIZE];
	char username[INFLUX_USERNAME_SIZE];
	char password[INFLUX_PASSWORD_SIZE];
};
typedef struct influx_config influx_config_t;

struct influx_value {
	int type;
	void *data;
	int len;
};
typedef struct influx_value influx_value_t;

struct influx_series {
	char name[128];
	char **columns;
	int column_count;
	influx_value_t **values;
	int value_count;
	jsval name_val;
	jsval columns_val;
	jsval values_val;
	bool convdt;
};
typedef struct influx_series influx_series_t;

int influx_parse_config(influx_config_t *conf, char *influx_info);
influx_session_t *influx_new(influx_config_t *);
char *influx_mkurl(influx_session_t *s, char *action, char *query);
list influx_get_results(influx_session_t *);

int influx_ping(influx_session_t *s);
int influx_health(influx_session_t *s);
int influx_query(influx_session_t *s, char *query);
int influx_write(influx_session_t *s, char *mm, char *field, int data_type, void *ptr, int size);

void influx_add_props(config_t *cp, influx_config_t *gconf, char *name, influx_config_t *conf);

#ifdef JS
#include "jsapi.h"
#include "jsengine.h"
JSObject *jsinflux_new(JSContext *, JSObject *, influx_session_t *);
#endif

#endif

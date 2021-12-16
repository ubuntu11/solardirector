
/*
Copyright (c) 2021, Stephen P. Shoecraft
All rights reserved.

This source code is licensed under the BSD-style license found in the
LICENSE file in the root directory of this source tree.
*/

#ifndef __SOLARD_JSON_H
#define __SOLARD_JSON_H

#include "cfg.h"
#include "types.h"
//#include "parson.h"

struct json_descriptor {
	char *name;
	enum DATA_TYPE type;
	char *scope;
	int nvalues;
	void *values;
	int nlabels;
	char **labels;
	char *units;
	float scale;
	char *format;
};
typedef struct json_descriptor json_descriptor_t;

typedef struct json_value json_value_t;

typedef void (json_ptcallback_t)(char *,void *,int,json_value_t *);

struct json_proctab {
	char *field;
	enum DATA_TYPE type;
	void *ptr;
	int len;
	json_ptcallback_t *cb;
};
typedef struct json_proctab json_proctab_t;

#define JSON_PROCTAB_END { 0, 0, 0, 0, 0 }

struct json_object {
    json_value_t  *wrapping_value;
    char       **names;
    json_value_t **values;
    size_t       count;
    size_t       capacity;
};
typedef struct json_object json_object_t;

struct json_array {
    json_value_t  *wrapping_value;
    json_value_t **items;
    size_t       count;
    size_t       capacity;
};
typedef struct json_array json_array_t;

typedef struct json_string {
    char *chars;
    size_t length;
} JSON_String;

enum json_value_type {
	JSONError   = -1,
	JSONNull    = 1,
	JSONString  = 2,
	JSONNumber  = 3,
	JSONObject  = 4,
	JSONArray   = 5,
	JSONBoolean = 6
};
typedef int json_value_Type;
#define __HAVE_JSON_TYPES 1

/* Type definitions */
union json_value_value {
	JSON_String  string;
	double       number;
	json_object_t *object;
	json_array_t  *array;
	int          boolean;
	int          null;
};
typedef union json_value_value json_value_value_t;

struct json_value {
    struct json_value *parent;
    json_value_Type  type;
    json_value_value_t value;
};
typedef struct json_value JSON_Value;

struct descriptor_scope {
	int type;
	union {
		struct {
			int min;
			int max;
			int step;
		} range;
		int *values;
	};
};

char *json_typestr(int);
json_value_t *json_create_object(void);
json_value_t *json_create_array(void);
int json_destroy(json_value_t *);
json_value_t *json_parse(char *);

int json_add_string(json_value_t *,char *,char *);
int json_add_boolean(json_value_t *,char *,int);
int json_add_number(json_value_t *,char *,double);
int json_add_value(json_value_t *,char *,json_value_t *);
#define json_add_object(v,n,o) json_add_value(v,n,(json_value_t *)o)
int json_add_descriptor(json_value_t *,char *,json_descriptor_t);

char *json_get_string(json_value_t *,char *);
double json_get_number(json_value_t *,char *);
int json_get_boolean(json_value_t *,char *);
json_value_t *json_get_value(json_value_t *,char *);
void json_conv_value(enum DATA_TYPE, void *, int, json_value_t *);

int json_array_add_string(json_value_t *,char *);
int json_array_add_strings(json_value_t *,char *);
int json_array_add_boolean(json_value_t *,int);
int json_array_add_number(json_value_t *,double);
int json_array_add_value(json_value_t *,json_value_t *);
int json_array_add_descriptor(json_value_t *,json_descriptor_t);

int json_tostring(json_value_t *,char *,int,int);
char *json_dumps(json_value_t *,int);
int json_dumps_r(json_value_t *,char *,int);
const char *json_string (const JSON_Value *value);
void json_free_serialized_string(char *string);
#define json_free_string(s) json_free_serialized_string(s)

json_value_t *json_from_tab(json_proctab_t *);
int json_to_tab(json_proctab_t *, json_value_t *);

typedef int (json_ifunc_t)(void *,char *,char *);
int json_iter(char *name, json_value_t *v, json_ifunc_t *func, void *ctx);

int json_object_remove(json_object_t *, char *);

#ifndef TAINT
char *json_object_dotget_string(json_object_t *, char *);
json_array_t *json_object_dotget_array(json_object_t *, char *);
double json_object_dotget_number(json_object_t *, char *);
int json_type(json_value_t *);
#define json_type(v) v->type
#define json_object(v) v->value.object
#define json_array(v) v->value.array
#define json_string(v) v->value.string.chars
#define json_strlen(v) v->value.string.length
#define json_number(v) v->value.number
#define json_bool(v) v->value.boolean
#endif
json_value_t *json_from_cfgtab(cfg_proctab_t *);

#endif

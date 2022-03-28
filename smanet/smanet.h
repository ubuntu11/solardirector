
/*
Copyright (c) 2021, Stephen P. Shoecraft
All rights reserved.

This source code is licensed under the BSD-style license found in the
LICENSE file in the root directory of this source tree.
*/

#ifndef __SMANET_H
#define __SMANET_H

#include "driver.h"
#include "buffer.h"
#include "types.h"
#include <pthread.h>

/* Channel type */
#define CH_ANALOG	0x0001
#define CH_DIGITAL	0x0002
#define CH_COUNTER	0x0004
#define CH_STATUS	0x0008
#define CH_IN		0x0100
#define CH_OUT		0x0200
#define CH_PARA		0x0400
#define CH_SPOT		0x0800
#define CH_MEAN		0x1000
#define CH_TEST		0x2000

/* Channel format */
#define CH_BYTE		0x00
#define CH_SHORT	0x01
#define CH_LONG		0x02
#define CH_FLOAT	0x04
#define CH_DOUBLE	0x05
#define CH_ARRAY	0x08

struct smanet_channel {
	uint16_t id;			/* Also an index into the values array */
	uint8_t index;
	uint16_t mask;
	uint16_t format;
	int type;
	int count;
	uint16_t level;
	char name[17];
	char unit[9];
	/* Values according to class */
	union {
		float gain;		/* Analog & counter */
		char txtlo[17];		/* Digital */
		uint8_t size;		/* Status */
	};
	union {
		float offset;		/* Analog */
		char txthi[17];		/* Digital */
		list strings;		/* Status */
	};
};
typedef struct smanet_channel smanet_channel_t;

//struct __attribute__((__packed__)) smanet_value {
struct smanet_value {
	time_t timestamp;
	int type;
	union {
		uint8_t bval;
		uint16_t wval;
		uint32_t lval;
		float fval;
		double dval;
	};
};
typedef struct smanet_value smanet_value_t;

struct smanet_session {
	buffer_t *b;
//	list channels;
	smanet_channel_t *chans;
	int chancount;
//	smanet_value_t *values;
	smanet_value_t values[1024];
	solard_driver_t *tp;
	void *tp_handle;
	char *transport;
	char *target;
	char *topts;
	bool connected;
	long serial;
	char type[32];
	uint16_t src;
	uint16_t dest;
	int timeouts;
	int commands;
#ifdef SMANET_THREADSAFE
	pthread_mutex_t lock;
#endif
	char errmsg[128];
};
typedef struct smanet_session smanet_session_t;

struct smanet_multreq {
	char *name;
	double value;
	char *text;
};
typedef struct smanet_multreq smanet_multreq_t;

smanet_session_t *smanet_init(char *, char *, char *);
void smanet_destroy(smanet_session_t *s);
int smanet_connect(smanet_session_t *, char *, char *, char *);

int smanet_read_channels(smanet_session_t *s);
int smanet_save_channels(smanet_session_t *s, char *);
int smanet_load_channels(smanet_session_t *s, char *);
smanet_channel_t *smanet_get_channel(smanet_session_t *s, char *);

int smanet_get_multvalues(smanet_session_t *, smanet_multreq_t *, int);
int smanet_get_value(smanet_session_t *, char *, double *, char **);
int smanet_get_chanvalue(smanet_session_t *, smanet_channel_t *, double *, char **);
int smanet_set_value(smanet_session_t *, char *, double, char *);
int smanet_reset_value(smanet_session_t *, char *);

#if 0
//int smanet_get_value(smanet_session_t *, smanet_channel_t *, double *);
int smanet_get_valuebyname(smanet_session_t *, char *, double *);
//int smanet_set_value(smanet_session_t *s, smanet_channel_t *c, double);
int smanet_set_valuebyname(smanet_session_t *s, char *, double);

int smanet_get_option(smanet_session_t *s, smanet_channel_t *, char *, int);
int smanet_get_optionbyname(smanet_session_t *s, char *, char *, int);
int smanet_set_option(smanet_session_t *s, smanet_channel_t *, char *, int);
int smanet_set_optionbyname(smanet_session_t *s, char *, char *);
#endif

void smanet_clear_values(smanet_session_t *);
char *smanet_get_errmsg(smanet_session_t *);
int smanet_lock(smanet_session_t *);
int smanet_unlock(smanet_session_t *);

#ifdef JS
#include "jsengine.h"
JSObject *js_InitSMANETClass(JSContext *cx, JSObject *global);
JSObject *jssmanet_new(JSContext *cx, JSObject *parent, smanet_session_t *s,char *transport, char *target, char *topts);
int smanet_jsinit(JSEngine *js);
#endif

#endif

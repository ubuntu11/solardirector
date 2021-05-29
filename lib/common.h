
/*
Copyright (c) 2021, Stephen P. Shoecraft
All rights reserved.

This source code is licensed under the BSD-style license found in the
LICENSE file in the root directory of this source tree.
*/

#ifndef __SD_COMMON_H
#define __SD_COMMON_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <stdarg.h>
#include <stdint.h>
#include <errno.h>
#include <time.h>

#ifdef __WIN32
struct __attribute__((packed, aligned(1))) can_frame {
	long can_id;
	unsigned char can_dlc;
        unsigned char data[8];
};
#else
#include <linux/can.h>
#endif

#include "types.h"
#include "opts.h"
#include "cfg.h"
#include "message.h"
#include "json.h"
#include "utils.h"
#include "debug.h"
#include "state.h"
#include "mqtt.h"

#define SOLARD_ID_LEN		38
#define SOLARD_NAME_LEN		32
#define SOLARD_ROLE_LEN		16
#define SOLARD_TRANSPORT_LEN	32
#define SOLARD_TARGET_LEN	64
#define SOLARD_TOPTS_LEN	32
#define SOLARD_TOPIC_LEN	128	/* Max topic length */
#define SOLARD_TOPIC_SIZE	256	/* Max topic length */

int solard_common_init(int argc,char **argv,opt_proctab_t *add_opts,int start_opts);

/* TOPIC / ROLE / NAME / FUNC */
#define SOLARD_TOPIC_ROOT	"SolarD"

#define SOLARD_ROLE_CONTROLLER	"Controller"
#define SOLARD_ROLE_STORAGE	"Storage"
#define SOLARD_ROLE_BATTERY	"Battery"
#define SOLARD_ROLE_INVERTER	"Inverter"
#define SOLARD_ROLE_PRODUCER	"Producer"
#define SOLARD_ROLE_CONSUMER	"Consumer"

#define SOLARD_FUNC_STATUS	"Status"
#define SOLARD_FUNC_INFO	"Info"
#define SOLARD_FUNC_DATA	"Data"
#define SOLARD_FUNC_CONFIG	"Config"
#define SOLARD_FUNC_CONTROL	"Control"

#if 0
#define SOLARD_ACTION_GET	"Get"
#define SOLARD_ACTION_SET	"Set"
#define SOLARD_ACTION_ADD	"Add"
#define SOLARD_ACTION_DEL	"Del"
#define SOLARD_ACTION_DAT	"Settings"

#define SOLARD_ID_STATUS	"Status"
#endif

struct solard_power {
	union {
		float l1;
		float a;
	};
	union {
		float l2;
		float b;
	};
	union {
		float l3;
		float c;
	};
	float total;
};
typedef struct solard_power solard_power_t;

extern char SOLARD_BINDIR[], SOLARD_ETCDIR[], SOLARD_LIBDIR[], SOLARD_LOGDIR[];

#if defined(__WIN32) && defined(NEED_EXPORT)
#ifdef DLL_EXPORT
  #define EXPORT __declspec(dllexport)
#else
  #define EXPORT __declspec(dllimport)
#endif
#define DLLCALL __cdecl
#endif

#endif

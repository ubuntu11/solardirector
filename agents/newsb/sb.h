
/*
Copyright (c) 2021, Stephen P. Shoecraft
All rights reserved.

This source code is licensed under the BSD-style license found in the
LICENSE file in the root directory of this source tree.
*/

#ifndef __SUNNYBOY_H
#define __SUNNYBOY_H

#include "agent.h"
#include <curl/curl.h>
#include "sma_strings.h"
#include "sma_object.h"

#define SB_ID_SIZE 32

struct sb_value {
	sma_object_t *o;
	char w[5];
	int type;
	union {
		double d;
		char *s;
		list l;
	};
};
typedef struct sb_value sb_value_t;

#define SB_ENDPOINT_SIZE 32
#define SB_PASSWORD_SIZE 32
#define SB_SESSION_ID_SIZE 32

struct sb_session {
	solard_agent_t *ap;		/* Agent config pointer */
	char endpoint[SB_ENDPOINT_SIZE];
	char password[SB_PASSWORD_SIZE];
	uint16_t state;			/* Pack state */
	CURL *curl;
	char session_id[SB_SESSION_ID_SIZE];
	list data;
	char *login_fields;
	char *read_fields;
	char *parm_fields;
	int errcode;			/* error indicator */
	char errmsg[256];		/* Error message if errcode !0 */
};
typedef struct sb_session sb_session_t;

/* States */
#define SB_STATE_OPEN		0x01

int sb_config(void *h, int req, ...);

extern solard_driver_t sb_driver;

#endif

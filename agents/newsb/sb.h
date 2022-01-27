
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

struct sbval {
	char id[32];
	int type;
	list values;
};
typedef struct sbval sbval_t;

struct sb_data {
	float voltage;
};
typedef struct sb_data sb_data_t;

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
	sb_data_t data;
	char *fields;
	int errcode;			/* error indicator */
	char errmsg[256];		/* Error message if errcode !0 */
};
typedef struct sb_session sb_session_t;

/* States */
#define SB_STATE_OPEN		0x01

int sb_config(void *h, int req, ...);

extern solard_driver_t sb_driver;

#endif


/*
Copyright (c) 2021, Stephen P. Shoecraft
All rights reserved.

This source code is licensed under the BSD-style license found in the
LICENSE file in the root directory of this source tree.
*/

#ifndef __SUNNYBOY_H
#define __SUNNYBOY_H

#include "agent.h"

#define JBD_NAME_LEN 32
#define JBD_MAX_TEMPS 8
#define JBD_MAX_CELLS 32


struct sb_data {
	float voltage;
};
typedef struct sb_data sb_data_t;

struct sb_session {
	solard_agent_t *ap;		/* Agent config pointer */
	char transport[SOLARD_TRANSPORT_LEN];
	char target[SOLARD_TARGET_LEN];
	char topts[SOLARD_TOPTS_LEN];
	solard_driver_t *tp;		/* Our transport */
	void *tp_handle;		/* Our transport handle */
	uint16_t state;			/* Pack state */
	sb_data_t data;
	int errcode;			/* error indicator */
	char errmsg[256];		/* Error message if errcode !0 */
};
typedef struct sb_session sb_session_t;

/* States */
#define SB_STATE_OPEN		0x01

int sb_read(void *, void *, int);
int sb_config(void *h, int req, ...);

#endif

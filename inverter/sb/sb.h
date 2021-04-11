
/*
Copyright (c) 2021, Stephen P. Shoecraft
All rights reserved.

This source code is licensed under the BSD-style license found in the
LICENSE file in the root directory of this source tree.
*/

#ifndef __SD_SB_H
#define __SD_SB_H

#include <stdint.h>
#include "battery.h"
#include "state.h"

struct sb_session {
	solard_agent_t *conf;		/* Our config */
	solard_module_t *tp;		/* Our transport */
	void *tp_handle;		/* Our transport handle */
	uint16_t state;			/* Pack state */
};
typedef struct sb_session sb_session_t;

json_value_t *sb_info(void *);
int sb_read(void *handle,void *,int);
int sb_config(void *,char *,char *,list);
int sb_control(void *, char *, char *, json_value_t *);

#endif

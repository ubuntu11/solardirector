
/*
Copyright (c) 2021, Stephen P. Shoecraft
All rights reserved.

This source code is licensed under the BSD-style license found in the
LICENSE file in the root directory of this source tree.
*/

#include "jk.h"

int jk_read(void *handle, void *buf, int buflen) {
	jk_session_t *s = handle;
//	solard_battery_t *pp;
	int r;

//	pp = buf;

	dprintf(2,"transport: %s\n", s->tp->name);
	r = 1;
#if 0
	if (strncmp(s->tp->name,"can",3)==0) 
		r = jk_can_get_pack(s,pp);
	else
		r = jk_std_get_pack(s,pp);
#endif
	return r;
}

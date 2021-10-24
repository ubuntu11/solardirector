
/*
Copyright (c) 2021, Stephen P. Shoecraft
All rights reserved.

This source code is licensed under the BSD-style license found in the
LICENSE file in the root directory of this source tree.
*/

#include "jk.h"

int jk_config(void *h, int req, ...) {
	jk_session_t *s = h;
	va_list va;
	int r;

	r = 1;
	va_start(va,req);
	dprintf(1,"req: %d\n", req);
	switch(req) {
	case SOLARD_CONFIG_GET_INFO:
		{
			json_value_t **vp = va_arg(va,json_value_t **);
			dprintf(1,"vp: %p\n", vp);
			if (vp) {
				*vp = jk_info(s);
				r = 0;
			}
		}
	}
	dprintf(1,"returning: %d\n", r);
	return r;
}


/*
Copyright (c) 2022, Stephen P. Shoecraft
All rights reserved.

This source code is licensed under the BSD-style license found in the
LICENSE file in the root directory of this source tree.
*/

#include <string.h>
#include "driver.h"
#include "debug.h"

solard_driver_t *find_driver(solard_driver_t **transports, char *name) {
	register int i;

	if (!transports || !name) return 0;

	for(i=0; transports[i]; i++) {
//		dprintf(1,"transports[%d]: %p\n", i, transports[i]);
//		dprintf(1,"name[%d]: %s\n", i, transports[i]->name);
		if (strcmp(transports[i]->name,name)==0) break;
	}

	dprintf(1,"tp: %p\n", transports[i]);
	return transports[i];
}

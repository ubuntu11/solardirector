
/*
Copyright (c) 2021, Stephen P. Shoecraft
All rights reserved.

This source code is licensed under the BSD-style license found in the
LICENSE file in the root directory of this source tree.
*/

#include <dlfcn.h>
#include "agent.h"
#include "module.h"
#include "list.h"

/* List of transports we support */
//static char *tlist[] = { "bt", "can", "can_ip", "ip", "serial" };
//#define TLSIZE (sizeof(tlist)/sizeof(char *))

#if 0
solard_module_t *solard_get_module(solard_config_t *conf, char *name, int type) {
	solard_module_t *mp;

	dprintf(3,"name: %s, type: %d\n", name, type);
	list_reset(conf->modules);
	while((mp = list_get_next(conf->modules)) != 0) {
		dprintf(3,"mp->name: %s, mp->type: %d\n", mp->name, mp->type);
		if (strcmp(mp->name,name)==0 && mp->type == type) {
			dprintf(3,"found.\n");
			return mp;
		}
	}
	dprintf(3,"NOT found.\n");
	return 0;
}
#endif

solard_module_t *load_module(list lp, char *name, int type) {
	char temp[128];
	solard_module_t *mp;

	dprintf(1,"lp: %p, name: %s, type: %d\n", lp, name, type);

	if (lp) {
		list_reset(lp);
		while((mp = list_get_next(lp)) != 0) {
			dprintf(3,"mp->name: %s, mp->type: %d\n", mp->name, mp->type);
			if (strcmp(mp->name,name)==0 && mp->type == type) {
				dprintf(3,"found.\n");
				return mp;
			}
		}
		dprintf(3,"NOT found.\n");
	}

#if 0
	sprintf(temp,"modules/%s.so", p);
	dprintf(3,"temp: %s\n", temp);
	h = dlopen(temp,RTLD_NOW);

	/* If a debug sym is present in the mod, set it to same as ours */
	mod_debug = dlsym(conf->dlsym_handle, "debug");
	if (mod_debug) *mod_debug = debug;
#endif

	/* Get the module symbol */
	sprintf(temp,"%s_module",name);
	mp = dlsym(0, temp);
	dprintf(3,"module: %p\n", mp);
	if (!mp) {
		log_write(LOG_ERROR,"%s\n",dlerror());
//		printf("error: cannot find symbol: %s_module: %s\n",name,dlerror());
		return 0;
	}
	if (type != SOLARD_MODTYPE_ANY && mp->type != type) {
		log_write(LOG_ERROR,"load_module: requested type (%d) does not match module type (%d)\n", type, mp->type);
		return 0;
	}

	/* Call the init function */
	dprintf(3,"init: %p\n", mp->init);
	if (mp->init && mp->init(0,0)) {
		printf("%s init function returned error, aborting.\n",mp->name);
		return 0;
	}

	if (lp) {
		/* Add this module to our list */
		dprintf(3,"adding module: %s\n", mp->name);
		list_add(lp,mp,0);
	}

	return mp;
}

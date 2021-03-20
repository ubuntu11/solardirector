
/*
Copyright (c) 2021, Stephen P. Shoecraft
All rights reserved.

This source code is licensed under the BSD-style license found in the
LICENSE file in the root directory of this source tree.
*/

#define _GNU_SOURCE
#include <dlfcn.h>
#include "solard.h"

//#define STATIC_MODULES 1

#ifdef STATIC_MODULES
extern solard_module_t si_module;
extern solard_module_t bt_module;
extern solard_module_t ip_module;
extern solard_module_t serial_module;
extern solard_module_t can_module;
extern solard_module_t can_ip_module;
#endif

solard_module_t *solard_get_module(sdagent_config_t *conf, char *name, int type) {
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

solard_module_t *solard_load_module(sdagent_config_t *conf, char *name, int type) {
#ifndef STATIC_MODULES
	char temp[128];
#endif
	solard_module_t *mp;

	list_reset(conf->modules);
	while((mp = list_get_next(conf->modules)) != 0) {
		dprintf(3,"mp->name: %s, mp->type: %d\n", mp->name, mp->type);
		if (strcmp(mp->name,name)==0 && mp->type == type) {
			dprintf(3,"found.\n");
			return mp;
		}
	}
	dprintf(3,"NOT found.\n");

#if 0
	sprintf(temp,"modules/%s.so", p);
	dprintf(3,"temp: %s\n", temp);
	h = dlopen(temp,RTLD_NOW);

	/* If a debug sym is present in the mod, set it to same as ours */
	mod_debug = dlsym(conf->dlsym_handle, "debug");
	if (mod_debug) *mod_debug = debug;
#endif

#ifdef STATIC_MODULES
	/* Static config */
	if (strcmp(name,"ip")==0) {
		mp = &ip_module;
#ifdef BLUETOOH
	} else if (strcmp(name,"bt")==0) {
		mp = &bt_module;
#endif
	} else if (strcmp(name,"serial")==0) {
		mp = &serial_module;
	} else if (strcmp(name,"can")==0) {
		mp = &can_module;
	} else if (strcmp(name,"can_ip")==0) {
		mp = &can_ip_module;
#ifdef SI
	} else if (strcmp(name,"si")==0) {
		mp = &si_module;
#endif
	} else {
		return 0;
	}
#else
	/* Get the module symbol */
	sprintf(temp,"%s_module",name);
//	mp = dlsym(conf->dlsym_handle, temp);
	mp = dlsym(RTLD_DEFAULT, temp);
	dprintf(3,"module: %p\n", mp);
	if (!mp) {
		printf("error: cannot find symbol: %s_module: %s\n",name,dlerror());
		return 0;
	}
#endif

	/* Call the init function */
	dprintf(3,"init: %p\n", mp->init);
	if (mp->init && mp->init(conf)) {
		printf("%s init function returned error, aborting.\n",mp->name);
		return 0;
	}

	/* Add this module to our list */
	dprintf(3,"adding module: %s\n", mp->name);
	list_add(conf->modules,mp,0);

	return mp;
}

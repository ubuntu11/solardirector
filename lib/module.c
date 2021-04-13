
/*
Copyright (c) 2021, Stephen P. Shoecraft
All rights reserved.

This source code is licensed under the BSD-style license found in the
LICENSE file in the root directory of this source tree.
*/

#include "agent.h"
#include "module.h"
#include "list.h"

#ifdef __WIN32
#include <windows.h>
#else
#include <dlfcn.h>
#endif

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
#ifdef __WIN32
	HMODULE mod;
//	FARPROC proc;
#endif

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

	/* Get the module */
	sprintf(temp,"%s_module",name);
#ifdef __WIN32
	mod = GetModuleHandle(0);
	dprintf(1,"mod: %p\n", mod);
	mp = (solard_module_t *) GetProcAddress(mod, temp);
#else
	mp = dlsym(0, temp);
#endif
	dprintf(3,"module: %p\n", mp);
	if (!mp) {
#ifdef __WIN32
		log_write(LOG_ERROR,"GetProcAddress %s\n",temp);
#else
		log_write(LOG_ERROR,"%s\n",dlerror());
#endif
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

int load_tp_from_cfg(solard_module_t **mp, void **h, cfg_info_t *cfg, char *section_name) {
	char transport[SOLARD_TRANSPORT_LEN],target[SOLARD_TARGET_LEN],topts[SOLARD_TOPTS_LEN];
	cfg_proctab_t tab[] = {
		{ section_name,"transport","Transport",DATA_TYPE_STRING,&transport,sizeof(transport),"" },
		{ section_name,"target","Transport address/interface/device", DATA_TYPE_STRING,&target,sizeof(target),"" },
		{ section_name,"topts","Transport specific options",DATA_TYPE_STRING,&topts,sizeof(topts),"" },
		CFG_PROCTAB_END
	};
	cfg_get_tab(cfg,tab);
	if (debug) cfg_disp_tab(tab,0,0);
	if (!strlen(transport) || !strlen(target) || !strlen(topts)) return 1;
	*mp = load_module(0,transport,SOLARD_MODTYPE_TRANSPORT);
	*h = (*mp)->new(0,target,topts);
	if (!*h) return 1;
	return 0;
}

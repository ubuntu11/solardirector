
/*
Copyright (c) 2021, Stephen P. Shoecraft
All rights reserved.

This source code is licensed under the BSD-style license found in the
LICENSE file in the root directory of this source tree.
*/

#include "solard.h"
#include "uuid.h"

#define INFO_TAB(SN) \
	{ SN, "agent_name", 0, DATA_TYPE_STRING,&info->agent,sizeof(info->agent), 0 }, \
	{ SN, "agent_path", 0, DATA_TYPE_STRING,&info->path,sizeof(info->path), 0 }, \
	{ SN, "agent_role", 0, DATA_TYPE_STRING,&info->role,sizeof(info->role), 0 }, \
	{ SN, "name", 0, DATA_TYPE_STRING,&info->name,sizeof(info->name), 0 }, \
	{ SN, "transport", 0, DATA_TYPE_STRING,&info->transport,sizeof(info->transport), 0 }, \
	{ SN, "target", 0, DATA_TYPE_STRING,&info->target,sizeof(info->target), 0 }, \
	{ SN, "topts", 0, DATA_TYPE_STRING,&info->topts,sizeof(info->topts), 0 }, \
	{ SN, "managed", 0, DATA_TYPE_LOGICAL,&info->managed,sizeof(info->managed),"true" }, \
	CFG_PROCTAB_END

void agentinfo_dump(solard_agentinfo_t *info) {
	cfg_proctab_t info_tab[] = { INFO_TAB(0) }, *t;
	char temp[128];

	for(t=info_tab; t->keyword; t++) {
		conv_type(DATA_TYPE_STRING,&temp,sizeof(temp)-1,t->type,t->dest,t->dlen);
		dprintf(1,"%s: %s\n", t->keyword, temp);
	}
}

void agentinfo_pub(solard_config_t *conf, solard_agentinfo_t *info) {
	cfg_proctab_t info_tab[] = { INFO_TAB(0) }, *t;
	char entry[1024],temp[128],temp2[256];
	int er;

	*entry = 0;
	er = sizeof(entry)-1;
	for(t=info_tab; t->keyword; t++) {
		conv_type(DATA_TYPE_STRING,&temp,sizeof(temp)-1,t->type,t->dest,t->dlen);
		if (!strlen(temp)) continue;
		snprintf(temp2,sizeof(temp2)-1,"%s=%s",t->keyword,temp);
		dprintf(1,"temp2: %s\n", temp2);
		if (strlen(entry)) {
			strncat(entry,",",er);
			er--;
		}
		strncat(entry,temp2,er);
		er -= strlen(temp2);
	}
	dprintf(1,"entry: %s\n", entry);
	agent_pub(conf->ap,SOLARD_FUNC_CONFIG,"Settings",0,entry,1);
}

void agentinfo_getcfg(cfg_info_t *cfg, char *sname, solard_agentinfo_t *info) {
	cfg_proctab_t info_tab[] = { INFO_TAB(sname) };

	dprintf(1,"getting tab...\n");
	cfg_get_tab(cfg,info_tab);
	if (debug) cfg_disp_tab(info_tab,"info",0);
	*info->id = 0;
	strncat(info->id,sname,sizeof(info->id)-1);
}

void agentinfo_setcfg(cfg_info_t *cfg, char *section_name, solard_agentinfo_t *info) {
	cfg_proctab_t info_tab[] = { INFO_TAB(section_name) };

	dprintf(1,"setting tab...\n");
	cfg_set_tab(cfg,info_tab,0);
	if (debug) cfg_disp_tab(info_tab,"info",0);
}

int agentinfo_set(solard_agentinfo_t *info, char *key, char *value) {
	cfg_proctab_t info_tab[] = { INFO_TAB(0) }, *t;
	int r;

	r = 1;
	dprintf(5,"key: %s, value: %s\n", key, value);
	for(t=info_tab; t->keyword; t++) {
		dprintf(5,"keyword: %s\n", t->keyword);
		if (strcmp(t->keyword,key) == 0) {
			dprintf(5,"found!\n");
			conv_type(t->type,t->dest,t->dlen,DATA_TYPE_STRING,value,strlen(value));
			r = 0;
			break;
		}
	}
	dprintf(5,"returning: %d\n", r);
	return r;
}

int agentinfo_add(solard_config_t *conf, solard_agentinfo_t *info) {
	solard_agentinfo_t *ap;
	char *agent_path,*agent_role;

	agentinfo_dump(info);

	dprintf(1,"info: agent: %s, role: %s, name: %s, transport: %s, target: %s\n", info->agent, info->role, info->name, info->transport, info->target);
	if (!strlen(info->agent) || !strlen(info->transport) || !strlen(info->target)) {
		/* invalid */
		dprintf(1,"agent is not valid, not adding\n");
		return 1;
	}

	agent_path = agent_role = 0;

	list_reset(conf->agents);
	while ((ap = list_get_next(conf->agents)) != 0) {
//		dprintf(1,"ap->name: %s\n", ap->name);
		if (strcmp(ap->agent,info->agent) == 0) {
			/* Save path & role for later */
			if (!agent_path) agent_path = ap->path;
			if (!agent_role) agent_role = ap->role;
			if (strcmp(ap->name,info->name) == 0) {
				/* name exists */
				strcpy(conf->errmsg,"name already exists");
				dprintf(1,"%s\n",conf->errmsg);
				return 1;
			}
		}
	}
	if (strlen(info->path) == 0 && agent_path != 0) strcpy(info->path, agent_path);

	/* We need the role */
	if (!strlen(info->role)) {
		if (agent_role) {
			dprintf(1,"setting role: %s\n", agent_role);
			strcpy(info->role, agent_role);
		} else {
			dprintf(1,"getting role...\n");
			if (agent_get_role(conf, info)) return 1;
		}
	}
	list_add(conf->agents, info, sizeof(*info));

	/* Now add to the role base lists */
	if (strcmp(info->role,SOLARD_ROLE_BATTERY)==0) {
		solard_battery_t newbat;

		memset(&newbat,0,sizeof(newbat));
		strcpy(newbat.name,info->name);
		time(&newbat.last_update);
		list_add(conf->batteries,&newbat,sizeof(newbat));
	} else if (strcmp(info->role,SOLARD_ROLE_INVERTER)==0) {
		solard_inverter_t newinv;

		memset(&newinv,0,sizeof(newinv));
		strcpy(newinv.name,info->name);
		time(&newinv.last_update);
		list_add(conf->inverters,&newinv,sizeof(newinv));
	}

	dprintf(1,"added!\n");
	return 0;
}

int agentinfo_get(solard_config_t *conf, char *entry) {
	solard_agentinfo_t newinfo,*info = &newinfo;
	cfg_proctab_t info_tab[] = { INFO_TAB(0) }, *t;
	char key[64],*str, *p;
	list lp;

	conv_type(DATA_TYPE_LIST,&lp,0,DATA_TYPE_STRING,entry,strlen(entry));
	dprintf(5,"lp: %p\n", lp);
	if (!lp) return 1;
	dprintf(5,"count: %d\n", list_count(lp));

	memset(&newinfo,0,sizeof(newinfo));
	list_reset(lp);
	while((str = list_get_next(lp)) != 0) {
		p = strchr(str,'=');
		if (!p) continue;
		*p++ = 0;
		strncpy(key,str,sizeof(key)-1);
		dprintf(6,"key: %s, value: %s\n", key, p);
		for(t=info_tab; t->keyword; t++) {
		      dprintf(6,"keyword: %s\n", t->keyword);
			if (strcmp(t->keyword,key) == 0) {
			      dprintf(6,"found!\n");
				conv_type(t->type,t->dest,t->dlen,DATA_TYPE_STRING,p,strlen(p));
				break;
			}
		}
	}
	agentinfo_add(conf,info);
	return 0;
}

void agentinfo_newid(solard_agentinfo_t *info) {
	uint8_t uuid[16];

	/* Create a new agent */
	dprintf(4,"gen'ing UUID...\n");
	uuid_generate_random(uuid);
	my_uuid_unparse(uuid, info->id);
	dprintf(4,"new id: %s\n",info->id);
}


/*
Copyright (c) 2021, Stephen P. Shoecraft
All rights reserved.

This source code is licensed under the BSD-style license found in the
LICENSE file in the root directory of this source tree.
*/

#include "solard.h"
#include "uuid.h"
#include <sys/types.h>
#include <sys/wait.h>
#include <sys/stat.h>
#include <sys/signal.h>
#include <unistd.h>

int agent_start(solard_config_t *conf, solard_agentinfo_t *info) {
	char logfile[256],*args[32];
	int i;

	if (!conf->c->cfg && solard_write_config(conf)) return 1;

	log_info("Starting agent: %s/%s\n", info->role, info->name);

	/* Do we have a config file? */
	/* logfile */
	sprintf(logfile,"%s/%s.log",SOLARD_LOGDIR,info->name);
	dprintf(5,"logfile: %s\n", logfile);

	/* Exec the agent */
	i = 0;
	args[i++] = info->path;
	args[i++] = "-c";
	args[i++] = conf->c->cfg->filename;
	args[i++] = "-s";
	args[i++] = info->id;
	args[i++] = "-l";
	args[i++] = logfile;
	args[i++] = 0;
	info->pid = solard_exec(info->path,args,0,0);
	if (info->pid < 0) {
		log_write(LOG_SYSERR,"agent_start: exec");
		return 1;
	}
	info->state = 0;
	time(&info->started);
	return 0;
}

void agent_warning(solard_config_t *conf, solard_agentinfo_t *info, int num) {
	solard_set_state(info,AGENTINFO_STATUS_WARNED);
	log_write(LOG_WARNING,"%s/%s has not reported in %d seconds\n",info->role,info->name,num);
}

void agent_error(solard_config_t *conf, solard_agentinfo_t *info, int secs) {
	solard_set_state(info,AGENTINFO_STATUS_ERROR);
	if (!info->managed || info->pid < 1) {
		log_write(LOG_ERROR,"%s/%s has not reported in %d seconds, considered lost\n",
			info->role,info->name,secs);
		return;
	}
	log_write(LOG_ERROR,"%s/%s has not reported in %d seconds, killing\n",info->role,info->name,secs);
	kill(info->pid,SIGTERM);
}

static time_t get_updated(solard_config_t *conf, solard_agentinfo_t *info) {
	time_t last_update;

	dprintf(5,"name: %s\n", info->name);

	last_update = 0;
	if (strcmp(info->role,SOLARD_ROLE_BATTERY)==0) {
		solard_battery_t *bp;

		list_reset(conf->batteries);
		while((bp = list_get_next(conf->batteries)) != 0) {
			dprintf(7,"bp->name: %s\n", bp->name);
			if (strcmp(bp->name,info->name) == 0) {
				dprintf(7,"found!\n");
				last_update = bp->last_update;
				break;
			}
		}
	} else if (strcmp(info->role,SOLARD_ROLE_INVERTER)==0) {
	}
	dprintf(5,"returning: %ld\n",last_update);
	return last_update;
}

int check_agents(solard_config_t *conf) {
	solard_agentinfo_t *info;
	time_t cur,diff,last;

	dprintf(1,"checking agents... count: %d\n", list_count(conf->agents));
	list_reset(conf->agents);
	while((info = list_get_next(conf->agents)) != 0) {
		time(&cur);
		dprintf(1,"name: %s\n",info->name);
		if (info->managed) {
			/* Give it a little bit to start up */
			if ((cur - info->started) < 31) continue;
			if (info->pid < 1) {
				if (agent_start(conf,info))
					log_write(LOG_ERROR,"unable to start agent!");
				continue;
			} else {
				int status;

				/* Check if the process exited */
				dprintf(5,"pid: %d\n", info->pid);
				if (waitpid(info->pid, &status, WNOHANG) != 0) {

					/* Get exit status */
					dprintf(5,"WIFEXITED: %d\n", WIFEXITED(status));
					if (WIFEXITED(status)) dprintf(1,"WEXITSTATUS: %d\n", WEXITSTATUS(status));
					status = (WIFEXITED(status) ? WEXITSTATUS(status) : 1);
					dprintf(5,"status: %d\n", status);

					/* XXX if process exits normally do we restart?? */
					/* XXX Number of restart attempts in a specific time */

					/* Re-start agent */
					if (agent_start(conf,info))
						log_write(LOG_ERROR,"unable to re-start agent!");
					continue;
				}
			}
		}
		last = get_updated(conf,info);
		dprintf(5,"last: %d\n", (int)last);
		if (last < 0) continue;
		diff = cur - last;
		dprintf(5,"diff: %d\n", (int)diff);
//		dprintf(1,"agent_error: %d, agent_warning: %d\n", conf->agent_error, conf->agent_warning);
//		dprintf(1,"AGENTINFO_STATUS_ERROR: %d\n", solard_check_state(info,AGENTINFO_STATUS_ERROR));
//		dprintf(1,"AGENTINFO_STATUS_WARNED: %d\n", solard_check_state(info,AGENTINFO_STATUS_WARNED));
//		dprintf(1,"last_action: %d\n",cur - info->last_action);
		if (conf->agent_error && diff >= conf->agent_error && !solard_check_state(info,AGENTINFO_STATUS_ERROR))
			agent_error(conf,info,diff);
		else if (conf->agent_warning && diff >= conf->agent_warning && !solard_check_state(info,AGENTINFO_STATUS_WARNED))
			agent_warning(conf,info,diff);
	}
	return 0;
}

int agent_get_role(solard_config_t *conf, solard_agentinfo_t *info) {
	char temp[128],configfile[256],logfile[256],*args[32], *output, *p;
	cfg_info_t *cfg;
	json_value_t *v;
	int i,r,status;
	struct stat sb;
	FILE *fp;
	cfg_section_t *section;

	/* 1st get the path to the agent if we dont have one */
	dprintf(1,"path: %s\n", info->path);
	if (!strlen(info->path) && solard_get_path(info->agent, info->path, sizeof(info->path)-1)) return 1;

	/* Get the tmpdir */
	tmpdir(temp,sizeof(temp)-1);

	/* Create a temp configfile */
	sprintf(configfile,"%s/%s.conf",temp,info->id);
	dprintf(1,"configfile: %s\n", configfile);
	cfg = cfg_create(configfile);
	if (!cfg) {
		log_write(LOG_SYSERR,"agent_get_role: cfg_create(%s)",configfile);
		return 1;
	}
	agentinfo_setcfg(cfg,info->name,info);
	cfg_write(cfg);

	/* Use a temp logfile */
	sprintf(logfile,"%s/%s.log",temp,info->id);
	dprintf(1,"logfile: %s\n", logfile);

	/* Exec the agent with the -I option and capture the output */
	i = 0;
	args[i++] = info->path;
	args[i++] = "-c";
	args[i++] = configfile;
	args[i++] = "-n";
	args[i++] = info->name;
//	args[i++] = "-d";
//	args[i++] = "2";
	args[i++] = "-I";
	args[i++] = 0;

	r = 1;
	output = 0;
	status = solard_exec(info->path,args,logfile,1);
	dprintf(1,"status: %d\n", status);
	if (status != 0) goto agent_get_role_error;

	/* Get the logfile size */
	if (stat(logfile,&sb) < 0) {
		log_write(LOG_SYSERR,"agent_get_role: stat(%s)",logfile);
		goto agent_get_role_error;
	}
	dprintf(1,"sb.st_size: %d\n", sb.st_size);

	/* Alloc buffer and read log into */
	output = malloc(sb.st_size+1);
	if (!output) {
		log_write(LOG_SYSERR,"agent_get_role: malloc(%d)",sb.st_size);
		goto agent_get_role_error;
	}
	dprintf(1,"output: %p\n", output);

	fp = fopen(logfile,"r");
	if (!fp) {
		log_write(LOG_SYSERR,"agent_get_role: fopen(%s)",logfile);
		goto agent_get_role_error;
	}
	fread(output,1,sb.st_size,fp);
	output[sb.st_size] = 0;
	fclose(fp);

	/* Find the start of the json data */
	dprintf(1,"output: %s\n", output);
	p = strchr(output, '{');
	if (!p) {
		log_write(LOG_ERROR,"agent_get_role: invalid json data");
		goto agent_get_role_error;
	}
	v = json_parse(p);
	if (!v) {
		log_write(LOG_ERROR,"agent_get_role: invalid json data");
		goto agent_get_role_error;
	}
	p = json_get_string(v, "agent_role");
	if (!p) {
		log_write(LOG_ERROR,"agent_get_role: agent_role not found in json data");
		goto agent_get_role_error;
	}
	dprintf(1,"p: %s\n", p);
	*info->role = 0;
	strncat(info->role,p,sizeof(info->role)-1);

	/* If we have this in our conf already, update it */
	section = cfg_get_section(conf->c->cfg,info->id);
	if (section) agentinfo_setcfg(conf->c->cfg,info->id,info);

	r = 0;
agent_get_role_error:
	unlink(configfile);
	unlink(logfile);
	if (output) free(output);
	return r;
}

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

int agentinfo_add(solard_config_t *conf, solard_agentinfo_t *info) {
	solard_agentinfo_t *ap;
	char *agent_path,*agent_role;

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
				dprintf(1,"name exists, not adding...\n");
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
	}

	dprintf(1,"added!\n");
	return 0;
}

void add_agent(solard_config_t *conf, char *role, json_value_t *v) {
	solard_agentinfo_t info;
	uint8_t uuid[16];
	json_proctab_t info_tab[] = {
		{ "agent_name",DATA_TYPE_STRING,&info.agent,sizeof(info.agent)-1,0 },
		{ "agent_path",DATA_TYPE_STRING,&info.path,sizeof(info.path)-1,0 },
		{ "agent_role",DATA_TYPE_STRING,&info.role,sizeof(info.role)-1,0 },
		{ "name",DATA_TYPE_STRING,&info.name,sizeof(info.name)-1,0 },
		{ "transport",DATA_TYPE_STRING,&info.transport,sizeof(info.transport)-1,0 },
		{ "target",DATA_TYPE_STRING,&info.target,sizeof(info.target)-1,0 },
		{ "topts",DATA_TYPE_STRING,&info.topts,sizeof(info.topts)-1,0 },
		{ "managed",DATA_TYPE_BOOL,&info.managed,0,0 },
		JSON_PROCTAB_END
	};

	/* Get the info */
	memset(&info,0,sizeof(info));
	info.managed = 1;
	json_to_tab(info_tab,v);

	/* Create a new agent */
	dprintf(1,"gen'ing UUID...\n");
	uuid_generate_random(uuid);
	my_uuid_unparse(uuid, info.id);
	dprintf(4,"new id: %s\n",info.id);
	if (agentinfo_add(conf, &info)) return;
	solard_write_config(conf);
}

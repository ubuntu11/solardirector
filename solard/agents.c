
/*
Copyright (c) 2021, Stephen P. Shoecraft
All rights reserved.

This source code is licensed under the BSD-style license found in the
LICENSE file in the root directory of this source tree.
*/

#include "solard.h"
#include <sys/types.h>
#include <sys/stat.h>

#ifdef __WIN32
#define STATFUNC _stat
#else
#define STATFUNC stat
#endif

int agent_start(solard_config_t *conf, solard_agentinfo_t *info) {
	char prog[256],logfile[256],*args[32];
	int i;

	if (!conf->c->cfg && solard_write_config(conf)) return 1;

	log_info("Starting agent: %s/%s\n", info->role, info->name);

	/* Do we have a config file? */
	/* logfile */
	sprintf(logfile,"%s/%s.log",SOLARD_LOGDIR,info->name);
	dprintf(5,"logfile: %s\n", logfile);

	/* Exec the agent */
	i = 0;
	strncpy(prog,info->path,sizeof(prog)-1);
#ifdef __WIN32
//	strcat(prog,".exe");
#endif
	args[i++] = prog;
	if (conf->c->cfg->filename) {
		args[i++] = "-c";
		args[i++] = conf->c->cfg->filename;
		args[i++] = "-s";
		args[i++] = info->id;
	} else {
		char temp[128];

		sprintf(temp,"%s,%s",info->transport,info->target);
		if (strlen(info->topts)) {
			strcat(temp,",");
			strcat(temp,info->topts);
		}
		args[i++] = "-t";
		args[i++] = temp;
		args[i++] = "-n";
		args[i++] = info->name;
		args[i++] = "-m";
		args[i++] = conf->c->mqtt_config.host;
	}
	args[i++] = "-l";
	args[i++] = logfile;
	args[i++] = 0;
	info->pid = solard_exec(info->path,args,logfile,0);
	dprintf(1,"pid: %d\n", info->pid);
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
	solard_kill(info->pid);
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
		solard_inverter_t *inv;

		list_reset(conf->inverters);
		while((inv = list_get_next(conf->inverters)) != 0) {
			dprintf(7,"inv->name: %s\n", inv->name);
			if (strcmp(inv->name,info->name) == 0) {
				dprintf(7,"found!\n");
				last_update = inv->last_update;
				break;
			}
		}
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
				if (solard_checkpid(info->pid, &status)) {
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
	struct STATFUNC sb;
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
	fixpath(configfile,sizeof(configfile)-1);
	cfg = cfg_create(configfile);
	if (!cfg) {
		log_write(LOG_SYSERR,"agent_get_role: cfg_create(%s)",configfile);
		return 1;
	}
	agentinfo_setcfg(cfg,info->name,info);
	dprintf(1,"writing config...\n");
	cfg_write(cfg);

	/* Use a temp logfile */
	sprintf(logfile,"%s/%s.log",temp,info->id);
	dprintf(1,"logfile: %s\n", logfile);
	fixpath(logfile,sizeof(logfile)-1);

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
	if (status != 0) {
		sprintf(configfile,"type %s",logfile);
		system(configfile);
		goto agent_get_role_error;
	}

	/* Get the logfile size */
	if (STATFUNC(logfile,&sb) < 0) {
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

#if 0
void add_agent(solard_config_t *conf, char *role, json_value_t *v) {
	solard_agentinfo_t info;
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

	agentinfo_newid(&info);
	if (agentinfo_add(conf, &info)) return;
	solard_write_config(conf);
}
#endif

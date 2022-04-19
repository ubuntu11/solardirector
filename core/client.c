
/*
Copyright (c) 2021, Stephen P. Shoecraft
All rights reserved.

This source code is licensed under the BSD-style license found in the
LICENSE file in the root directory of this source tree.
*/

#define DEBUG_CLIENT 1
#define dlevel 4

#ifdef DEBUG
#undef DEBUG
#define DEBUG DEBUG_CLIENT
#endif
#include "debug.h"

#include <pthread.h>
#include "client.h"
#include "libgen.h"

#define DEBUG_STARTUP 0

#if DEBUG_STARTUP
#define _ST_DEBUG LOG_DEBUG
#else
#define _ST_DEBUG 0
#endif

client_agentinfo_t *client_getagentbyname(solard_client_t *c, char *name) {
	client_agentinfo_t *info;

	dprintf(dlevel,"name: %s\n", name);

	list_reset(c->agents);
	while((info = list_get_next(c->agents)) != 0) {
		dprintf(dlevel,"info->name: %s\n", info->name);
		if (strcmp(info->name,name) == 0) {
			dprintf(dlevel,"found\n");
			return info;
		}
	}
	dprintf(dlevel,"NOT found\n");
	return 0;
}

client_agentinfo_t *client_getagentbyid(solard_client_t *c, char *id) {
	client_agentinfo_t *info;

	dprintf(dlevel,"id: %s\n", id);

	list_reset(c->agents);
	while((info = list_get_next(c->agents)) != 0) {
		dprintf(dlevel,"info->id: %s\n", info->id);
		if (strcmp(info->id,id) == 0) {
			dprintf(dlevel,"found\n");
			return info;
		}
	}
	dprintf(dlevel,"NOT found\n");
	return 0;
}

client_agentinfo_t *client_getagentbyrole(solard_client_t *c, char *role) {
	client_agentinfo_t *info;

	dprintf(dlevel,"role: %s\n", role);

	list_reset(c->agents);
	while((info = list_get_next(c->agents)) != 0) {
		dprintf(dlevel,"info->role: %s\n", info->role);
		if (strcmp(info->role,role) == 0) {
			dprintf(dlevel,"found\n");
			return info;
		}
	}
	dprintf(dlevel,"NOT found\n");
	return 0;
}

char *client_getagentrole(client_agentinfo_t *info) {
	register char *p;

	dprintf(dlevel,"name: %s\n", info->name);

	if (*info->role) p = info->role;
	else if (info->info) p = json_object_dotget_string(json_value_get_object(info->info),"agent_role");
	else p = 0;
	dprintf(dlevel,"role: %s\n", p);
	return p;
}

int client_matchagent(client_agentinfo_t *info, char *target) {
	char temp[SOLARD_ROLE_LEN+SOLARD_NAME_LEN+2];
	char role[SOLARD_ROLE_LEN];
	char name[SOLARD_NAME_LEN];
	register char *p;

	dprintf(dlevel,"target: %s\n", target);

	if (strcasecmp(target,"all") == 0) return 1;

	strncpy(temp,target,sizeof(temp)-1);
	*role = *name = 0;
	p = strchr(temp,'/');
	if (p) {
		*p = 0;
		strncpy(role,temp,sizeof(role)-1);
		dprintf(dlevel,"role: %s, info->role: %s\n", role, info->role);
		if (*role && *info->role && strcmp(info->role,role) != 0) {
			dprintf(dlevel,"role NOT matched\n");
			return 0;
		}
		strncpy(name,p+1,sizeof(name)-1);
	} else {
		strncpy(name,temp,sizeof(name)-1);
	}
	dprintf(dlevel,"name: %s, info->name: %s\n", name, info->name);
	if (*name && *info->name && strcmp(info->name,name) == 0) {
		dprintf(dlevel,"name matched\n");
		return 1;
	}

	dprintf(dlevel,"checking aliases...\n");
	list_reset(info->aliases);
	while((p = list_get_next(info->aliases)) != 0) {
		dprintf(dlevel,"alias: %s\n", p);
		if (strcmp(p,name) == 0) {
			dprintf(dlevel,"alias found\n");
			return 1;
		}
	}
	dprintf(dlevel,"alias NOT found\n");
	dprintf(dlevel,"NO MATCH\n");
	return 0;
}

#ifdef MQTT
static void parse_funcs(client_agentinfo_t *info) {
	json_object_t *o;
	json_array_t *a;
	config_function_t newfunc;
	char *p;
	int i;

	/* Functions is an array of objects */
	/* each object has name and args */
	a = json_object_get_array(json_value_get_object(info->info), "functions");
	dprintf(dlevel,"a: %p\n", a);
	if (!a) return;
	for(i=0; i < a->count; i++) {
		if (json_value_get_type(a->items[i]) != JSON_TYPE_OBJECT) continue;
		o = json_value_get_object(a->items[i]);
		p = json_object_get_string(o,"name");
		newfunc.name = malloc(strlen(p)+1);
		if (!newfunc.name) {
			log_syserror("parse_funcs: malloc(%d)",strlen(p)+1);
			return;
		}
		strcpy(newfunc.name,p);
		newfunc.nargs = json_object_get_number(o,"nargs");
		dprintf(dlevel,"adding: %s\n", newfunc.name);
		list_add(info->funcs,&newfunc,sizeof(newfunc));
	}
}

static void add_alias(client_agentinfo_t *info, char *name) {
	register char *p;

	if (!name) return;
	dprintf(dlevel+1,"name: %s\n", name);

	list_reset(info->aliases);
	while((p = list_get_next(info->aliases)) != 0) {
		if (strcmp(p,name)==0) {
			dprintf(dlevel+1,"NOT adding\n");
			return;
		}
	}
	dprintf(dlevel+1,"adding: %s\n",name);
	list_add(info->aliases,name,strlen(name)+1);
}

static int process_info(client_agentinfo_t *info, char *data) {
	register char *p;

	/* parse the info */
//	printf("data: %s\n", data);
	if (info->info) json_destroy_value(info->info);
	info->info = json_parse(data);
	if (info->info) {
		if (json_value_get_type(info->info) != JSON_TYPE_OBJECT) {
			log_error("invalid info data from: %s\n", info->name);
			return 1;
		}
		if (!strlen(info->role)) {
			p = json_object_get_string(json_value_get_object(info->info),"agent_role");
			if (p) strncpy(info->role,p,sizeof(info->role)-1);
			sprintf(info->target,"%s/%s",info->role,info->name);
		}
		p = json_object_get_string(json_value_get_object(info->info),"device_aliases");
		dprintf(dlevel,"aliasses: %p\n", info->aliases);
		if (p) conv_type(DATA_TYPE_STRING_LIST,info->aliases,0,DATA_TYPE_STRING,p,strlen(p));
		add_alias(info,json_object_get_string(json_value_get_object(info->info),"agent_name"));
		add_alias(info,json_object_get_string(json_value_get_object(info->info),"agent_role"));
		add_alias(info,json_object_get_string(json_value_get_object(info->info),"device_model"));
#if 0
		/* XXX could be stale - cant depend on this */
		/* If the AP doesnt have the id, grab it from info */
		if (!strlen(info->id)) {
			str = json_object_get_string(json_value_get_object(info->info),"agent_id");
			if (str) strncpy(info->id,str,sizeof(info->id)-1);
		}
#endif
		/* Parse the funcs */
		parse_funcs(info);
	} else {
		log_error("unable to parse info from: %s\n", info->name);
		return 1;
	}

	return 0;
}

static int process_config(client_agentinfo_t *info, char *data) {
	json_value_t *v;

//	printf("data: %s\n", data);
	v = json_parse(data);
	if (!v) return 1;
	config_parse_json(info->c->cp, v);
	json_destroy_value(v);
#if 1
	if (info->config) json_destroy_value(info->config);
	info->config = json_parse(data);
	if (info->config) {
		if (json_value_get_type(info->config) != JSON_TYPE_OBJECT) {
			log_error("invalid config data from: %s\n", info->target);
			json_destroy_value(info->config);
			info->config = 0;
			return 1;
		}
	} else {
		log_error("unable to parse config from: %s\n", info->target);
		return 1;
	}
#endif

	return 0;
}

#if 0
static int process_data(client_agentinfo_t *info, char *data) {
//	solard_battery_t *bp;
//	solard_inverter_t *inv;
	json_value_t *v;

	/* We only handle inverters and batteries */
//	printf("data: %s\n", data);
	v = json_parse(data);
	if (v) {
		if (json_value_get_type(info->config) != JSON_TYPE_OBJECT) {
			log_error("invalid config data from: %s\n", info->target);
			json_destroy_value(v);
			return 1;
		} else {
			log_error("unable to parse config from: %s\n", info->target);
			json_destroy_value(v);
			return 1;
		}
	}

	return 0;
}
#endif
#endif

#if 0
static void agent_dump(client_agentinfo_t *info, int level) {
	char aliases[128], funcs[128];
#define SFORMAT "%15.15s: %s\n"
#define UFORMAT "%15.15s: %04x\n"
#define IFORMAT "%15.15s: %d\n"

	dprintf(level,SFORMAT,"id",info->id);
	dprintf(level,SFORMAT,"role",info->role);
	dprintf(level,SFORMAT,"name",info->name);
	dprintf(level,SFORMAT,"target",info->target);
	dprintf(level,SFORMAT,"status",info->online ? "online" : "offline");
	if (list_count(info->aliases)) {
		dprintf(dlevel,"aliases: %p\n", info->aliases)
		conv_type(DATA_TYPE_STRING,aliases,sizeof(aliases),DATA_TYPE_STRING_LIST,info->aliases,list_count(info->aliases));
		dprintf(level,SFORMAT,"aliases",aliases);
	}
	if (list_count(info->funcs)) {
		config_function_t *f;
		int i;
		register char *p;

		i = 0;
		p = funcs;
		list_reset(info->funcs);
		while((f = list_get_next(info->funcs)) != 0) {
			if (i++) p += sprintf(p,", ");
			p += sprintf(p,"%s(%d)",f->name,f->nargs);
		}
		dprintf(level,SFORMAT,"functions",funcs);
	}
	dprintf(level,UFORMAT,"state",info->state);
	if (solard_check_bit(info->state,CLIENT_AGENTINFO_STATUS)) {
		dprintf(level,IFORMAT,"status",info->status);
		dprintf(level,SFORMAT,"errmsg",info->errmsg);
	}
}
#endif

int client_getagentstatus(client_agentinfo_t *info, solard_message_t *msg) {
	json_value_t *v;
	char *errmsg;

	dprintf(dlevel,"getting status...\n");
	v = json_parse(msg->data);
	dprintf(dlevel,"v: %p\n", v);
	if (v && json_value_get_type(v) != JSON_TYPE_OBJECT) return 1;
	info->status = json_object_get_number(json_value_get_object(v),"status");
	errmsg = json_object_get_string(json_value_get_object(v),"message");
	if (!errmsg) return 1;
	strcpy(info->errmsg,errmsg);
	dprintf(dlevel,"status: %d, errmsg: %s\n", info->status, info->errmsg);
	solard_set_bit(info->state,CLIENT_AGENTINFO_STATUS);
	return 0;
}

#ifdef MQTT
static void client_getmsg(void *ctx, char *topic, char *message, int msglen, char *replyto) {
	solard_client_t *c = ctx;
	solard_message_t newmsg;
	client_agentinfo_t newinfo,*info;

	if (solard_getmsg(&newmsg,topic,message,msglen,replyto)) return;
	dprintf(dlevel,"newmsg: topic: %s, id: %s, len: %d, replyto: %s\n", newmsg.topic, newmsg.id, (int)strlen(newmsg.data), newmsg.replyto);
	info = 0;
	if (strlen(newmsg.replyto)) info = client_getagentbyid(c,newmsg.replyto);
	dprintf(dlevel,"info: %p\n", info);
	if (!info) info = client_getagentbyname(c,newmsg.id);
	dprintf(dlevel,"info: %p\n", info);
	if (!info) {
		char *p;

		if (strcmp(basename(newmsg.topic),SOLARD_FUNC_STATUS) == 0) return;
		memset(&newinfo,0,sizeof(newinfo));
		newinfo.c = c;
		newinfo.aliases = list_create();
		newinfo.funcs = list_create();
		newinfo.mq = list_create();

		strncpy(newinfo.id,newmsg.replyto,sizeof(newinfo.id)-1);
		strncpy(newinfo.target,newmsg.id,sizeof(newinfo.target)-1);
		dprintf(dlevel,"newinfo.target: %s\n", newinfo.target);
		p = strchr(newinfo.target,'/');
		if (p) {
			*p = 0;
			strncpy(newinfo.role,newinfo.target,sizeof(newinfo.role)-1);
			strncpy(newinfo.name,p+1,sizeof(newinfo.name)-1);
			sprintf(newinfo.target,"%s/%s",newinfo.role,newinfo.name);
		} else {
			strncpy(newinfo.name,newinfo.target,sizeof(newinfo.name)-1);
		}
		dprintf(dlevel,"newinfo: id: %s, role: %s, name: %s\n", newinfo.id, newinfo.role, newinfo.name);
		info = list_add(c->agents,&newinfo,sizeof(newinfo));
	}
	if (strlen(newmsg.replyto) && !strlen(info->id)) strncpy(info->id,newmsg.replyto,sizeof(info->id)-1);
	if (strcmp(newmsg.func,SOLARD_FUNC_STATUS)==0) {
		if (strcmp(newmsg.data,"Online")==0) info->online = 1;
		else info->online = 0;
	} else if (strcmp(newmsg.func,SOLARD_FUNC_INFO)==0) {
		process_info(info,newmsg.data);
	} else if (strcmp(newmsg.func,SOLARD_FUNC_CONFIG)==0) {
		process_config(info,newmsg.data);
#if 0
	} else if (strcmp(newmsg.func,SOLARD_FUNC_DATA)==0) {
		process_data(info,newmsg.data);
#endif
	} else {
		/* We did not handle the message */
		list_add(info->mq,&newmsg,sizeof(newmsg));
	}
//	agent_dump(info,1);
	return;
}
#endif

config_function_t *client_getagentfunc(client_agentinfo_t *info, char *name) {
	config_function_t *f;

	dprintf(dlevel,"name: %s\n", name);

	list_reset(info->funcs);
	while((f = list_get_next(info->funcs)) != 0) {
		dprintf(dlevel,"f->name: %s\n", f->name);
		if (strcmp(f->name,name)==0) {
			dprintf(dlevel,"found!\n");
			return f;
		}
	}
	dprintf(dlevel,"NOT found!\n");
	return 0;
}

static json_value_t *client_mkargs(int nargs,char **args,config_function_t *f) {
	json_object_t *o;
	json_array_t *a;
	int i;

	/* nargs = 1: Object with function name and array of arguments */
	/* { "func": [ "arg", "argn", ... ] } */
	/* nargs > 1: Object with function name and array of argument arrays */
	/* { "func": [ [ "arg", "argn", ... ], [ "arg", "argn" ], ... ] } */
	o = json_create_object();
	a = json_create_array();
	dprintf(dlevel,"f->nargs: %d\n", f->nargs);
	if (f->nargs == 1) {
		for(i=0; i < nargs; i++) json_array_add_string(a,args[i]);
	} else if (f->nargs > 1) {
		json_array_t *aa;
		register int j;

		i = 0;
		while(i < nargs) {
			aa = json_create_array();
			for(j=0; j < f->nargs; j++) json_array_add_string(aa,args[i++]);
			json_array_add_array(a,aa);
		}
	}
	json_object_set_array(o,f->name,a);
	return json_object_get_value(o);
}

int client_callagentfunc(client_agentinfo_t *info, config_function_t *f, int nargs, char **args) {
#ifdef MQTT
	char topic[SOLARD_TOPIC_LEN],*p;
#endif
	json_value_t *v;
	char *j;

	dprintf(dlevel,"nargs: %d, f->nargs: %d\n", nargs, f->nargs);

	/* Args passed must be divisible by # of args the function takes */
	if (f->nargs && (nargs % f->nargs) != 0) {
		solard_set_bit(info->state,CLIENT_AGENTINFO_STATUS);
		info->status = 1;
		sprintf(info->errmsg, "%s: function %s requires %d arguments but %d given",
			info->target, f->name, f->nargs, nargs);
		return 1;
	}

	/* create the args according to function */
	v = client_mkargs(nargs,args,f);
	j = json_dumps(v,0);
	if (!j) {
		solard_set_bit(info->state,CLIENT_AGENTINFO_STATUS);
		info->status = 1;
		sprintf(info->errmsg, "solard_callagentfunc: memory allocation error: %s",strerror(errno));
		return 1;
	}
	json_destroy_value(v);

#ifdef MQTT
	/* send the req */
	p = topic;
	p += sprintf(p,"%s/%s",SOLARD_TOPIC_ROOT,SOLARD_TOPIC_AGENTS);
//	if (strlen(info->role)) p += sprintf(p,"/%s", info->role);
	p += sprintf(p,"/%s",info->name);
	dprintf(dlevel,"m: %p\n",info->c->m);
	if (mqtt_pub(info->c->m,topic,j,1,0)) {
		free(j);
		log_error("mqtt_pub");
		return 1;
	}
	dprintf(dlevel,"done!\n");
#endif
	free(j);
	solard_set_bit(info->state,CLIENT_AGENTINFO_REPLY);
	solard_clear_bit(info->state,CLIENT_AGENTINFO_STATUS);
	return 0;
}

int client_callagentfuncbyname(client_agentinfo_t *info, char *name, int nargs, char **args) {
	config_function_t *f;

	f = client_getagentfunc(info,name);
	if (!f) return 1;
	return client_callagentfunc(info, f, nargs, args);
}

#if 0
static char *find_entry(solard_client_t *cp, char *name) {
	solard_message_t *dp;
	char *p;

	dprintf(dlevel,"name: %s\n", name);

	p = 0;
	list_reset(cp->mq);
	while((dp = list_get_next(cp->mq)) != 0) {
		dprintf(dlevel,"dp: param %s, data: %s\n", dp->param, dp->data);
		if (strcmp(dp->param,name)==0) {
			p = dp->data;
			break;
		}
	}
	dprintf(dlevel,"returning: %s\n", p);
	return p;
}

static int client_get_status(solard_client_t *cp, char *action, char *param, int timeout) {
	solard_message_t *msg;
	int retries, r, found;

	dprintf(dlevel,"action: %s, param: %s\n", action, param);

	r = 1;
	retries=timeout;
	found = 0;
	while(retries--) {
		dprintf(dlevel,"count: %d\n", list_count(cp->mq));
		list_reset(cp->mq);
		while((msg = list_get_next(cp->mq)) != 0) {
			if (msg->type != SOLARD_MESSAGE_STATUS) continue;
			if (strcmp(msg->action,action)==0) {
				dprintf(dlevel,"match.\n");
				list_delete(cp->mq,msg);
				found = 1;
				r = 0;
				break;
			}
		}
		if (!found) sleep(1);
		else break;
	}
	if (retries < 0) r = -1;
	dprintf(dlevel,"r: %d\n", r);
	return r;
}

char *check(solard_client_t *cp, char *param, int wait) {
	char *p;
	int retries;

	/* See if we have it first */
	retries=1;
	while(retries--) {
		p = find_entry(cp,param);
		dprintf(dlevel,"p: %s\n", p);
		if (p) return strcpy(cp->data,p);
		if (wait) sleep(1);
	}
	return 0;
}

char *client_get_config(solard_client_t *cp, char *op, char *target, char *param, int timeout, int direct) {
	char topic[SOLARD_TOPIC_SIZE], *p;
	json_value_t *a;

	dprintf(dlevel,"op: %s, target: %s, param: %s, timeout: %d, direct: %d\n", op, target, param, timeout, direct);

	if (!direct && (p = check(cp, param, 0)) != 0) return p;

	a = json_create_array();
	json_array_add_string(a,param);
	json_tostring(a,cp->data,sizeof(cp->data)-1,0);
	json_destroy(a);

	/* Request it */
	sprintf(topic,"%s/%s/%s/%s/%s",SOLARD_TOPIC_ROOT,target,SOLARD_FUNC_CONFIG,op,cp->id);
	dprintf(dlevel,"topic: %s\n",topic);
	mqtt_pub(cp->m,topic,cp->data,0);

	/* Get status message */
	client_get_status(cp, op, param, timeout);

	/* Get value message */
	dprintf(dlevel,"finding...\n");
	p = find_entry(cp, param);

        /* Clear our request and the status reply */
        mqtt_pub(cp->m,topic,0,0);
        sprintf(topic,"%s/%s/%s/%s/Status/%s",SOLARD_TOPIC_ROOT,target,SOLARD_FUNC_CONFIG,op,cp->id);
        mqtt_pub(cp->m,topic,0,0);

	dprintf(dlevel,"returning: %s\n", p);
	return p;
}

list client_get_mconfig(solard_client_t *cp, char *op, char *target, int count, char **params, int timeout) {
	char topic[SOLARD_TOPIC_SIZE], *p;
	json_value_t *a;
	int i;
	list results;

	dprintf(dlevel,"op: %s, target: %s, count: %d, params: %p, timeout: %d\n", op, target, count, params, timeout);

	results = list_create();

	a = json_create_array();

	/* Only fetch the ones we dont have */
	for(i=0; i < count; i++) {
		dprintf(dlevel,"params[%d]: %s\n", i, params[i]);
		if ((p = check(cp, params[i],0)) == 0) json_array_add_string(a,params[i]);
	}
	dprintf(dlevel,"a->count: %d\n",(int)a->value.array->count);
	if (a->value.array->count) {
		json_tostring(a,cp->data,sizeof(cp->data)-1,0);
		dprintf(dlevel,"data: %s\n", cp->data);

		/* Request */
		sprintf(topic,"%s/%s/%s/%s/%s",SOLARD_TOPIC_ROOT,target,SOLARD_FUNC_CONFIG,op,cp->id);
		dprintf(dlevel,"topic: %s\n",topic);
		mqtt_pub(cp->m,topic,cp->data,0);

		/* Get status message */
		client_get_status(cp, op, 0, timeout);

		/* Clear our request and the status reply */
		mqtt_pub(cp->m,topic,0,0);
		sprintf(topic,"%s/%s/%s/%s/Status/%s",SOLARD_TOPIC_ROOT,target,SOLARD_FUNC_CONFIG,op,cp->id);
		mqtt_pub(cp->m,topic,0,0);
	}

	/* Get value message */
	dprintf(dlevel,"finding...\n");
	for(i=0; i < count; i++) {
		if ((p = find_entry(cp, params[i])) != 0) list_add(results,p,strlen(p)+1);
	}

	json_destroy(a);
	dprintf(dlevel,"returning results\n");
	return results;
}

int client_set_config(solard_client_t *cp, char *op, char *target, char *param, char *value, int timeout) {
	char topic[SOLARD_TOPIC_SIZE];
	json_value_t *o,*v;

	dprintf(dlevel,"op: %s, target: %s, param: %s, value: %s\n", op, target, param, value);

	o = json_create_object();
	dprintf(dlevel,"value: %c\n", *value);
	if (*value == '{') {
		v = json_parse(value);
		if (!v) {
			log_write(LOG_ERROR,"invalid json string: %s\n", value);
			return 1;
		}
		json_add_value(o,param,v);
	} else {
		json_add_string(o,param,value);
	}
	json_tostring(o,cp->data,sizeof(cp->data)-1,0);
	json_destroy(o);

	/* Request it */
	sprintf(topic,"%s/%s/%s/%s/%s",SOLARD_TOPIC_ROOT,target,SOLARD_FUNC_CONFIG,op,cp->id);
	dprintf(dlevel,"topic: %s\n",topic);
	mqtt_pub(cp->m,topic,cp->data,0);

	/* Get status message */
	client_get_status(cp, op, param, timeout);

	/* Clear our request and the status reply */
	mqtt_pub(cp->m,topic,0,0);
	sprintf(topic,"%s/%s/%s/%s/Status/%s",SOLARD_TOPIC_ROOT,target,SOLARD_FUNC_CONFIG,op,cp->id);
	mqtt_pub(cp->m,topic,0,0);
	return 0;
}
#endif

void client_mktopic(char *topic, int topicsz, char *name, char *func) {
	register char *p;

	dprintf(dlevel,"name: %s, func: %s\n", name, func);

	p = topic;
	p += snprintf(p,topicsz-strlen(topic),"%s/%s",SOLARD_TOPIC_ROOT,SOLARD_TOPIC_CLIENTS);
	if (name) p += snprintf(p,topicsz-strlen(topic),"/%s",name);
	if (func) p += snprintf(p,topicsz-strlen(topic),"/%s",func);
}

static int client_startup(solard_client_t *c, char *configfile, char *mqtt_info, char *influx_info,
		config_property_t *prog_props, config_function_t *prog_funcs) {
	config_property_t client_props[] = {
		{ "name", DATA_TYPE_STRING, c->name, sizeof(c->name)-1, 0 },
//		{ "rtsize", DATA_TYPE_INT, &c->rtsize, 0, 0, CONFIG_FLAG_READONLY },
//		{ "stacksize", DATA_TYPE_INT, &c->stacksize, 0, 0, CONFIG_FLAG_READONLY },
		{ "script_dir", DATA_TYPE_STRING, c->script_dir, sizeof(c->script_dir)-1, 0 },
		{ "init_script", DATA_TYPE_STRING, c->init_script, sizeof(c->init_script)-1, "init.js", 0 },
		{ "start_script", DATA_TYPE_STRING, c->start_script, sizeof(c->start_script)-1, "start.js", 0 },
		{ "stop_script", DATA_TYPE_STRING, c->stop_script, sizeof(c->stop_script)-1, "stop.js", 0 },
		{0}
	};
	config_property_t *props;
	config_function_t *funcs;
	config_function_t client_funcs[] = {
		{ 0 }
	};

        props = config_combine_props(prog_props,client_props);
        funcs = config_combine_funcs(prog_funcs,client_funcs);

        /* Call common startup */
	if (solard_common_startup(&c->cp, c->section_name, configfile, props, funcs
#ifdef MQTT
		,&c->m, 0, client_getmsg, c, mqtt_info, c->config_from_mqtt
#endif
#ifdef INFLUX
		,&c->i, influx_info
#endif
#ifdef JS
		,&c->js, c->rtsize, c->stacksize, (js_outputfunc_t *)log_info
#endif
	)) return 1;

#ifdef JS
	/* Set script_dir if empty */
	dprintf(dlevel,"script_dir(%d): %s\n", strlen(c->script_dir), c->script_dir);
	if (!strlen(c->script_dir)) {
		sprintf(c->script_dir,"%s/clients/%s",SOLARD_LIBDIR,c->name);
		fixpath(c->script_dir,sizeof(c->script_dir));
		dprintf(dlevel,"NEW script_dir(%d): %s\n", strlen(c->script_dir), c->script_dir);
	}
#endif

        config_add_props(c->cp, c->name, client_props, 0);
	config_add_props(c->cp, "client", client_props, CONFIG_FLAG_NOSAVE | CONFIG_FLAG_NOID);
	return 0;
}

solard_client_t *client_init(int argc,char **argv,char *version,opt_proctab_t *client_opts,char *Cname,
			config_property_t *props,config_function_t *funcs) {
	solard_client_t *c;
	char mqtt_info[200],influx_info[200];
	char configfile[256];
	char name[64],sname[CFG_SECTION_NAME_SIZE];
#ifdef MQTT
	int config_from_mqtt;
#endif
#ifdef JS
	char jsexec[4096];
	int rtsize,stksize;
	char script[256];
#endif
	opt_proctab_t std_opts[] = {
		/* Spec, dest, type len, reqd, default val, have */
#ifdef MQTT
		{ "-m::|mqtt host,clientid[,user[,pass]]",&mqtt_info,DATA_TYPE_STRING,sizeof(mqtt_info)-1,0,"" },
		{ "-M|get config from mqtt",&config_from_mqtt,DATA_TYPE_LOGICAL,0,0,"N" },
#endif
		{ "-c:%|config file",&configfile,DATA_TYPE_STRING,sizeof(configfile)-1,0,"" },
		{ "-s::|config section name",&sname,DATA_TYPE_STRING,sizeof(sname)-1,0,"" },
		{ "-n::|client name",&name,DATA_TYPE_STRING,sizeof(name)-1,0,"" },
#ifdef INFLUX
		{ "-i::|influx host[,user[,pass]]",&influx_info,DATA_TYPE_STRING,sizeof(influx_info)-1,0,"" },
#endif
#ifdef JS
		{ "-e:%|exectute javascript",&jsexec,DATA_TYPE_STRING,sizeof(jsexec)-1,0,"" },
		{ "-R:#|javascript runtime size",&rtsize,DATA_TYPE_INT,0,0,"67108864" },
		{ "-S:#|javascript stack size",&stksize,DATA_TYPE_INT,0,0,"1048576" },
		{ "-X::|execute JS script and exit",&script,DATA_TYPE_STRING,sizeof(script)-1,0,"" },
#endif
		OPTS_END
	}, *opts;
	int logopts = LOG_INFO|LOG_WARNING|LOG_ERROR|LOG_SYSERR|_ST_DEBUG;

	opts = opt_addopts(std_opts,client_opts);
	if (!opts) return 0;
	*configfile = 0;
#ifdef MQTT
	*mqtt_info = 0;
#endif
#ifdef INFLUX
	*influx_info = 0;
#endif
#ifdef JS
	*script = *jsexec = 0;
#endif
	dprintf(dlevel,"argv: %p\n", argv);
	if (argv && solard_common_init(argc,argv,version,opts,logopts)) return 0;

	dprintf(dlevel,"creating session...\n");
	c = calloc(1,sizeof(*c));
	if (!c) {
		log_syserror("client_init: malloc(%d)",sizeof(*c));
		return 0;
	}
	if (strlen(name)) {
		strncpy(c->name,name,sizeof(c->name)-1);
	} else if (Cname) {
		strncpy(c->name,Cname,sizeof(c->name)-1);
	} else {
		strncpy(c->name,"client",sizeof(c->name)-1);
	}
#ifdef MQTT
	c->config_from_mqtt = config_from_mqtt;
	c->mq = list_create();
#endif
	c->agents = list_create();

	dprintf(dlevel,"argc: %d, argv: %p, opts: %p, name: %s\n", argc, argv, opts, name);

	if (client_startup(c,configfile,mqtt_info,influx_info,props,funcs)) goto client_init_error;

#ifdef JS
	/* Execute any command-line javascript code */
	dprintf(dlevel,"jsexec(%d): %s\n", strlen(jsexec), jsexec);
	if (strlen(jsexec)) {
		if (JS_EngineExecString(c->js, jsexec))
			log_warning("error executing js expression\n");
	}

	/* Start the init script */
	snprintf(jsexec,sizeof(jsexec)-1,"%s/%s",c->script_dir,c->init_script);
	if (access(jsexec,0)) JS_EngineExec(c->js,jsexec,0,0);

	/* if specified, Execute javascript file then exit */
	dprintf(dlevel,"script: %s\n", script);
	if (strlen(script)) {
		JS_EngineExec(c->js,script,0,0);
		free(c);
		return 0;
	}
#endif

#ifdef MQTT
	/* Sub to the agents */
	sprintf(mqtt_info,"%s/%s/+/#",SOLARD_TOPIC_ROOT,SOLARD_TOPIC_AGENTS);
	dprintf(dlevel,"subscribing to: %s\n", mqtt_info);
	mqtt_sub(c->m,mqtt_info);

	/* Sleep a moment to ingest any messages */
	usleep(50000);
#endif

	return c;
client_init_error:
	free(c);
	return 0;
}

#ifdef JS
enum CLIENT_PROPERTY_ID {
	CLIENT_PROPERTY_ID_CONFIG=1024,
	CLIENT_PROPERTY_ID_MQTT,
	CLIENT_PROPERTY_ID_INFLUX
};

static JSBool client_getprop(JSContext *cx, JSObject *obj, jsval id, jsval *rval) {
	solard_client_t *c;

	c = JS_GetPrivate(cx,obj);
	if (!c) return JS_FALSE;
	return js_config_common_getprop(cx, obj, id, rval, c->cp, c->props);
}

static JSBool client_setprop(JSContext *cx, JSObject *obj, jsval id, jsval *vp) {
	solard_client_t *c;

	c = JS_GetPrivate(cx,obj);
	if (!c) return JS_FALSE;
	return js_config_common_setprop(cx, obj, id, vp, c->cp, c->props);
}

static JSClass client_class = {
	"SDClient",		/* Name */
	JSCLASS_HAS_PRIVATE,	/* Flags */
	JS_PropertyStub,	/* addProperty */
	JS_PropertyStub,	/* delProperty */
	client_getprop,		/* getProperty */
	client_setprop,		/* setProperty */
	JS_EnumerateStub,	/* enumerate */
	JS_ResolveStub,		/* resolve */
	JS_ConvertStub,		/* convert */
	JS_FinalizeStub,	/* finalize */
	JSCLASS_NO_OPTIONAL_MEMBERS
};

static JSBool jsclient_mktopic(JSContext *cx, uintN argc, jsval *vp) {
//	solard_client_t *c;
	char topic[SOLARD_TOPIC_SIZE], *name, *func;
	int topicsz = SOLARD_TOPIC_SIZE;
	JSObject *obj;
	register char *p;

	obj = JS_THIS_OBJECT(cx, vp);
	if (!obj) return JS_FALSE;
//	c = JS_GetPrivate(cx, obj);

	if (argc != 1) {
		JS_ReportError(cx,"mktopic requires 1 arguments (func:string)\n");
		return JS_FALSE;
	}

        func = 0;
        if (!JS_ConvertArguments(cx, argc, JS_ARGV(cx,vp), "s s", &name, &func)) return JS_FALSE;
	dprintf(dlevel,"func: %s\n", func);

//	client_mktopic(topic,sizeof(topic)-1,c,c->instance_name,func);

//	dprintf(dlevel,"name: %s, func: %s\n", name, func);

	p = topic;
	p += snprintf(p,topicsz-strlen(topic),"%s/%s/%s/%s",SOLARD_TOPIC_ROOT,SOLARD_TOPIC_AGENTS,name,func);
	p += snprintf(p,topicsz-strlen(topic),"/%s",name);
	p += snprintf(p,topicsz-strlen(topic),"/%s",func);
	*vp = STRING_TO_JSVAL(JS_NewStringCopyZ(cx,topic));
	return JS_TRUE;
}

#if 0
JSObject *JSClient(JSContext *cx, void *priv);

static JSBool client_ctor(JSContext *cx, JSObject *obj, uintN argc, jsval *argv, jsval *rval) {
	solard_client_t *c;
	char *name;
	JSObject *newobj;

	dprintf(dlevel,"argc: %d\n", argc);
	if (argc != 1) {
		JS_ReportError(cx, "client requires 1 argument (name:string)");
		return JS_FALSE;
	}

	name = 0;
	if (!JS_ConvertArguments(cx, argc, argv, "s", &name)) return JS_FALSE;
	dprintf(dlevel,"name: %s\n", name);

	c = client_init(0,0,0,0,name,0,0);
	dprintf(dlevel,"c: %p\n", c);
	if (!c) {
		JS_ReportError(cx, "client_init returned null\n");
		return JS_FALSE;
	}

	newobj = JSClient(cx,c);
	*rval = OBJECT_TO_JSVAL(newobj);

	return JS_TRUE;
}

JSObject *jsclient_new(JSContext *cx, void *tp, void *handle, char *transport, char *target, char *topts, int *con) {
#if 0
	solard_client_t *p;
	JSObject *newobj;

	p = calloc(sizeof(*p),1);
	if (!p) {
		JS_ReportError(cx,"jsclient_new: error allocating memory");
		return 0;
	}
	p->tp = tp;
	p->tp_handle = handle;
	p->transport = transport;
	p->target = target;
	p->topts = topts;
	p->connected = con;

	newobj = js_InitCANClass(cx,JS_GetGlobalObject(cx));
	JS_SetPrivate(cx,newobj,p);
	return newobj;
#endif
	return 0;
}
#endif

int jsclient_init(JSContext *cx, JSObject *parent, void *priv) {
	JSPropertySpec client_props[] = {
		{ "config", CLIENT_PROPERTY_ID_CONFIG, JSPROP_ENUMERATE },
		{ "mqtt", CLIENT_PROPERTY_ID_MQTT, JSPROP_ENUMERATE },
		{ "influx", CLIENT_PROPERTY_ID_INFLUX, JSPROP_ENUMERATE },
		{ 0 }
	};
	JSFunctionSpec client_funcs[] = {
		JS_FN("mktopic",jsclient_mktopic,1,1,0),
		{ 0 }
	};
	JSObject *obj;
	solard_client_t *c = priv;
	JSPropertySpec *props;

	props = 0;
	if (c && c->cp && !c->props) {
		c->props = config_to_props(c->cp, c->section_name, client_props);
		if (!c->props) {
			log_error("unable to create props: %s\n", config_get_errmsg(c->cp));
			return 1;
		}
		props = c->props;
	}
	dprintf(dlevel,"client props: %p\n", props);

	dprintf(dlevel,"creating %s object\n",client_class.name);
	obj = JS_InitClass(cx, parent, 0, &client_class, 0, 0, props, client_funcs, 0, 0);
	if (!obj) {
		JS_ReportError(cx,"unable to initialize %s class", client_class.name);
		return 1;
	}
	JS_SetPrivate(cx,obj,c);

	/* Create our child objects */
	c->config_val = OBJECT_TO_JSVAL(jsconfig_new(cx,obj,c->cp));
	c->mqtt_val = OBJECT_TO_JSVAL(jsmqtt_new(cx,obj,c->m));
	c->influx_val = OBJECT_TO_JSVAL(js_influx_new(cx,obj,c->i));

	dprintf(dlevel,"done!\n");
	return 0;
}

int client_jsinit(JSEngine *e, void *priv) {
	return JS_EngineAddInitFunc(e, "client", jsclient_init, priv);
}
#endif

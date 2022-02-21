
/*
Copyright (c) 2021, Stephen P. Shoecraft
All rights reserved.

This source code is licensed under the BSD-style license found in the
LICENSE file in the root directory of this source tree.
*/

#define dlevel 1

#include <pthread.h>
#include "client.h"

#define DEBUG_STARTUP 0

#if DEBUG_STARTUP
#define _ST_DEBUG LOG_DEBUG
#else
#define _ST_DEBUG 0
#endif

typedef int (pfunc_t)(void *ctx, solard_message_t *msg);

client_agentinfo_t *client_getagentbyname(solard_client_t *c, char *name) {
	client_agentinfo_t *ap;

	dprintf(4,"name: %s\n", name);

	list_reset(c->agents);
	while((ap = list_get_next(c->agents)) != 0) {
		dprintf(5,"ap->name: %s\n", ap->name);
		if (strcmp(ap->name,name) == 0) {
			dprintf(4,"found\n");
			return ap;
		}
	}
	dprintf(4,"NOT found\n");
	return 0;
}

client_agentinfo_t *client_getagentbyid(solard_client_t *c, char *id) {
	client_agentinfo_t *ap;

	dprintf(4,"id: %s\n", id);

	list_reset(c->agents);
	while((ap = list_get_next(c->agents)) != 0) {
		dprintf(6,"ap->id: %s\n", ap->id);
		if (strcmp(ap->id,id) == 0) {
			dprintf(4,"found\n");
			return ap;
		}
	}
	dprintf(4,"NOT found\n");
	return 0;
}

client_agentinfo_t *client_getagentbyrole(solard_client_t *c, char *role) {
	client_agentinfo_t *ap;

	dprintf(4,"role: %s\n", role);

	list_reset(c->agents);
	while((ap = list_get_next(c->agents)) != 0) {
		dprintf(5,"ap->role: %s\n", ap->role);
		if (strcmp(ap->role,role) == 0) {
			dprintf(4,"found\n");
			return ap;
		}
	}
	dprintf(4,"NOT found\n");
	return 0;
}

char *client_getagentrole(client_agentinfo_t *ap) {
	register char *p;

	dprintf(6,"name: %s\n", ap->name);

	if (*ap->role) p = ap->role;
	else if (ap->info) p = json_object_dotget_string(json_value_get_object(ap->info),"agent_role");
	else p = 0;
	dprintf(6,"role: %s\n", p);
	return p;
}

int client_matchagent(client_agentinfo_t *ap, char *target) {
	char temp[SOLARD_ROLE_LEN+SOLARD_NAME_LEN+2];
	char role[SOLARD_ROLE_LEN];
	char name[SOLARD_NAME_LEN];
	register char *p;

	dprintf(1,"target: %s\n", target);

	strncpy(temp,target,sizeof(temp)-1);
	*role = *name = 0;
	p = strchr(temp,'/');
	if (p) {
		*p = 0;
		strncpy(role,temp,sizeof(role)-1);
		dprintf(1,"role: %s, ap->role: %s\n", role, ap->role);
		if (*role && *ap->role && strcmp(ap->role,role) != 0) {
			dprintf(1,"role NOT matched\n");
			return 0;
		}
		strncpy(name,p+1,sizeof(name)-1);
	} else {
		strncpy(name,temp,sizeof(name)-1);
	}
	dprintf(1,"name: %s, ap->name: %s\n", name, ap->name);
	if (*name && *ap->name && strcmp(ap->name,name) == 0) {
		dprintf(1,"name matched\n");
		return 1;
	}

	list_reset(ap->aliases);
	while((p = list_get_next(ap->aliases)) != 0) {
		dprintf(1,"p: %s\n", p);
		if (strcmp(p,name) == 0) {
			dprintf(1,"alias found\n");
			return 1;
		}
	}
	dprintf(1,"alias NOT found\n");
	return 0;
}

static void parse_funcs(client_agentinfo_t *ap) {
	json_object_t *o;
	json_array_t *a;
	config_function_t newfunc;
	char *p;
	int i;

	/* Functions is an array of objects */
	/* each object has name and args */
	a = json_object_get_array(json_value_get_object(ap->info), "functions");
	dprintf(4,"a: %p\n", a);
	if (!a) return;
	for(i=0; i < a->count; i++) {
		if (json_value_get_type(a->items[i]) != JSON_TYPE_OBJECT) continue;
		o = json_value_get_object(a->items[i]);
		p = json_object_get_string(o,"name");
		newfunc.name = malloc(strlen(p)+1);
		if (!newfunc.name) {
			log_syserror("parse_funcs: malloc(%d)",strlen(p)+1);
			exit(1);
		}
		strcpy(newfunc.name,p);
		newfunc.nargs = json_object_get_number(o,"nargs");
		dprintf(4,"adding: %s\n", newfunc.name);
		list_add(ap->funcs,&newfunc,sizeof(newfunc));
	}
}

static void add_alias(list lp, char *name) {
	register char *p;

	if (!name) return;
	dprintf(1,"name: %s\n", name);

	list_reset(lp);
	while((p = list_get_next(lp)) != 0) {
		if (strcmp(p,name)==0) {
			dprintf(1,"NOT adding\n");
			return;
		}
	}
	dprintf(1,"adding: %s\n",name);
	list_add(lp,name,strlen(name)+1);
}

static int process_info(client_agentinfo_t *ap, char *data) {
	register char *p;

	/* parse the info */
//	printf("data: %s\n", data);
	if (ap->info) json_destroy_value(ap->info);
	ap->info = json_parse(data);
	if (ap->info) {
		if (json_value_get_type(ap->info) != JSON_TYPE_OBJECT) {
			log_error("invalid info data from: %s\n", ap->name);
			return 1;
		}
		if (!strlen(ap->role)) {
			p = json_object_get_string(json_value_get_object(ap->info),"agent_role");
			if (p) strncpy(ap->role,p,sizeof(ap->role)-1);
			sprintf(ap->target,"%s/%s",ap->role,ap->name);
		}
		p = json_object_get_string(json_value_get_object(ap->info),"device_aliases");
		dprintf(1,"aliasses: %p\n", ap->aliases);
		if (p) conv_type(DATA_TYPE_STRING_LIST,ap->aliases,0,DATA_TYPE_STRING,p,strlen(p));
		add_alias(ap->aliases,json_object_get_string(json_value_get_object(ap->info),"agent_name"));
		add_alias(ap->aliases,json_object_get_string(json_value_get_object(ap->info),"agent_role"));
		add_alias(ap->aliases,json_object_get_string(json_value_get_object(ap->info),"device_model"));
#if 0
		/* XXX could be stale - cant depend on this */
		/* If the AP doesnt have the id, grab it from info */
		if (!strlen(ap->id)) {
			str = json_object_get_string(json_value_get_object(ap->info),"agent_id");
			if (str) strncpy(ap->id,str,sizeof(ap->id)-1);
		}
#endif
		/* Parse the funcs */
		parse_funcs(ap);
	} else {
		log_error("unable to parse info from: %s\n", ap->name);
		return 1;
	}

	return 0;
}

static int process_config(client_agentinfo_t *ap, char *data) {
//	printf("data: %s\n", data);
	if (ap->config) json_destroy_value(ap->config);
	ap->config = json_parse(data);
	if (ap->config) {
		if (json_value_get_type(ap->config) != JSON_TYPE_OBJECT) {
			log_error("invalid config data from: %s\n", ap->target);
			json_destroy_value(ap->config);
			ap->config = 0;
			return 1;
		}
	} else {
		log_error("unable to parse config from: %s\n", ap->target);
		return 1;
	}

	return 0;
}

#if 0
static int process_data(client_agentinfo_t *ap, char *data) {
//	solard_battery_t *bp;
//	solard_inverter_t *inv;
	json_value_t *v;

	/* We only handle inverters and batteries */
//	printf("data: %s\n", data);
	v = json_parse(data);
	if (v) {
		if (json_value_get_type(ap->config) != JSON_TYPE_OBJECT) {
			log_error("invalid config data from: %s\n", ap->target);
			json_destroy_value(v);
			return 1;
		} else {
			log_error("unable to parse config from: %s\n", ap->target);
			json_destroy_value(v);
			return 1;
		}
	}

	return 0;
}
#endif

#if 0
static void agentinfo_dump(client_agentinfo_t *ap, int level) {
	char aliases[128], funcs[128];
#define SFORMAT "%15.15s: %s\n"
#define UFORMAT "%15.15s: %04x\n"
#define IFORMAT "%15.15s: %d\n"

	dprintf(level,SFORMAT,"id",ap->id);
	dprintf(level,SFORMAT,"role",ap->role);
	dprintf(level,SFORMAT,"name",ap->name);
	dprintf(level,SFORMAT,"target",ap->target);
	dprintf(level,SFORMAT,"status",ap->online ? "online" : "offline");
	if (list_count(ap->aliases)) {
		dprintf(1,"aliases: %p\n", ap->aliases)
		conv_type(DATA_TYPE_STRING,aliases,sizeof(aliases),DATA_TYPE_STRING_LIST,ap->aliases,list_count(ap->aliases));
		dprintf(level,SFORMAT,"aliases",aliases);
	}
	if (list_count(ap->funcs)) {
		config_function_t *f;
		int i;
		register char *p;

		i = 0;
		p = funcs;
		list_reset(ap->funcs);
		while((f = list_get_next(ap->funcs)) != 0) {
			if (i++) p += sprintf(p,", ");
			p += sprintf(p,"%s(%d)",f->name,f->nargs);
		}
		dprintf(level,SFORMAT,"functions",funcs);
	}
	dprintf(level,UFORMAT,"state",ap->state);
	if (solard_check_bit(ap->state,CLIENT_AGENTINFO_STATUS)) {
		dprintf(level,IFORMAT,"status",ap->status);
		dprintf(level,SFORMAT,"errmsg",ap->errmsg);
	}
}
#endif

int client_getagentstatus(client_agentinfo_t *ap, solard_message_t *msg) {
	json_value_t *v;
	char *errmsg;

	v = json_parse(msg->data);
	dprintf(1,"v: %p\n", v);
	if (v && json_value_get_type(v) != JSON_TYPE_OBJECT) return 1;
	ap->status = json_object_get_number(json_value_get_object(v),"status");
	errmsg = json_object_get_string(json_value_get_object(v),"message");
	if (!errmsg) return 1;
	strcpy(ap->errmsg,errmsg);
	dprintf(1,"status: %d, errmsg: %s\n", ap->status, ap->errmsg);
	solard_set_bit(ap->state,CLIENT_AGENTINFO_STATUS);
	return 0;
}

static void client_getmsg(void *ctx, char *topic, char *message, int msglen, char *replyto) {
	solard_client_t *c = ctx;
	solard_message_t newmsg;
	client_agentinfo_t newagent,*ap;

	if (solard_getmsg(&newmsg,topic,message,msglen,replyto)) return;
	dprintf(1,"newmsg: topic: %s, id: %s, len: %d, replyto: %s\n", newmsg.topic, newmsg.id, (int)strlen(newmsg.data), newmsg.replyto);
	ap = 0;
	if (strlen(newmsg.replyto)) ap = client_getagentbyid(c,newmsg.replyto);
	if (!ap) ap = client_getagentbyname(c,newmsg.id);
	if (!ap) {
		register char *p;

		memset(&newagent,0,sizeof(newagent));
		newagent.c = c;
		strncpy(newagent.id,newmsg.replyto,sizeof(newagent.id)-1);
		strncpy(newagent.target,newmsg.id,sizeof(newagent.target)-1);
//		printf("newagent: target: %s\n", newagent.target);
		p = strchr(newagent.target,'/');
		if (p) {
			*p = 0;
			strncpy(newagent.role,newagent.target,sizeof(newagent.role)-1);
			strncpy(newagent.name,p+1,sizeof(newagent.name)-1);
			sprintf(newagent.target,"%s/%s",newagent.role,newagent.name);
		} else {
			strncpy(newagent.name,newagent.target,sizeof(newagent.name)-1);
		}
//		printf("newagent: id: %s, role: %s, name: %s\n", newagent.id, newagent.role, newagent.name);
		newagent.aliases = list_create();
		newagent.funcs = list_create();
		newagent.mq = list_create();
		ap = list_add(c->agents,&newagent,sizeof(newagent));
	}
	if (strlen(newmsg.replyto) && !strlen(ap->id)) strncpy(ap->id,newmsg.replyto,sizeof(ap->id)-1);
	if (strcmp(newmsg.func,SOLARD_FUNC_STATUS)==0) {
		if (strcmp(newmsg.data,"Online")==0) ap->online = 1;
	} else if (strcmp(newmsg.func,SOLARD_FUNC_INFO)==0) {
		process_info(ap,newmsg.data);
	} else if (strcmp(newmsg.func,SOLARD_FUNC_CONFIG)==0) {
		process_config(ap,newmsg.data);
#if 0
	} else if (strcmp(newmsg.func,SOLARD_FUNC_DATA)==0) {
		process_data(ap,newmsg.data);
#endif
	} else {
		/* We did not handle the message */
		list_add(ap->mq,&newmsg,sizeof(newmsg));
	}
//	agentinfo_dump(ap,1);
	return;
}

config_function_t *client_getagentfunc(client_agentinfo_t *ap, char *name) {
	config_function_t *f;

	dprintf(1,"name: %s\n", name);

	list_reset(ap->funcs);
	while((f = list_get_next(ap->funcs)) != 0) {
		dprintf(1,"f->name: %s\n", f->name);
		if (strcmp(f->name,name)==0) {
			dprintf(1,"found!\n");
			return f;
		}
	}
	dprintf(1,"NOT found!\n");
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
	dprintf(1,"f->nargs: %d\n", f->nargs);
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

int client_callagentfunc(client_agentinfo_t *ap, config_function_t *f, int nargs, char **args) {
	char topic[SOLARD_TOPIC_LEN],*p;
	json_value_t *v;
	char *j;

	dprintf(1,"nargs: %d, f->nargs: %d\n", nargs, f->nargs);

	/* Args passed must be divisible by # of args the function takes */
	if (f->nargs && (nargs % f->nargs) != 0) {
		solard_set_bit(ap->state,CLIENT_AGENTINFO_STATUS);
		ap->status = 1;
		sprintf(ap->errmsg, "%s: function %s requires %d arguments but %d given\n",
			ap->target, f->name, f->nargs, nargs);
		return 1;
	}

	/* create the args according to function */
	v = client_mkargs(nargs,args,f);
	j = json_dumps(v,0);
	if (!j) {
		solard_set_bit(ap->state,CLIENT_AGENTINFO_STATUS);
		ap->status = 1;
		sprintf(ap->errmsg, "solard_callagentfunc: memory allocation error: %s",strerror(errno));
		return 1;
	}
	json_destroy_value(v);

	/* send the req */
	p = topic;
	p += sprintf(p,"%s/%s",SOLARD_TOPIC_ROOT,SOLARD_TOPIC_AGENTS);
//	if (strlen(ap->role)) p += sprintf(p,"/%s", ap->role);
	p += sprintf(p,"/%s",ap->name);
	dprintf(1,"m: %p\n",ap->c->m);
	if (mqtt_pub(ap->c->m,topic,j,1,0)) {
		free(j);
		log_error("mqtt_pub");
		return 1;
	}
	dprintf(1,"done!\n");
	free(j);
	solard_set_bit(ap->state,CLIENT_AGENTINFO_REPLY);
	solard_clear_bit(ap->state,CLIENT_AGENTINFO_STATUS);
	return 0;
}

int client_callagentfuncbyname(client_agentinfo_t *ap, char *name, int nargs, char **args) {
	config_function_t *f;

	f = client_getagentfunc(ap,name);
	if (!f) return 1;
	return client_callagentfunc(ap, f, nargs, args);
}

#if 0
static char *find_entry(solard_client_t *cp, char *name) {
	solard_message_t *dp;
	char *p;

	dprintf(7,"name: %s\n", name);

	p = 0;
	list_reset(cp->mq);
	while((dp = list_get_next(cp->mq)) != 0) {
		dprintf(7,"dp: param %s, data: %s\n", dp->param, dp->data);
		if (strcmp(dp->param,name)==0) {
			p = dp->data;
			break;
		}
	}
	dprintf(7,"returning: %s\n", p);
	return p;
}

static int client_get_status(solard_client_t *cp, char *action, char *param, int timeout) {
	solard_message_t *msg;
	int retries, r, found;

	dprintf(1,"action: %s, param: %s\n", action, param);

	r = 1;
	retries=timeout;
	found = 0;
	while(retries--) {
		dprintf(5,"count: %d\n", list_count(cp->mq));
		list_reset(cp->mq);
		while((msg = list_get_next(cp->mq)) != 0) {
			if (msg->type != SOLARD_MESSAGE_STATUS) continue;
			if (strcmp(msg->action,action)==0) {
				dprintf(1,"match.\n");
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
	dprintf(1,"r: %d\n", r);
	return r;
}

char *check(solard_client_t *cp, char *param, int wait) {
	char *p;
	int retries;

	/* See if we have it first */
	retries=1;
	while(retries--) {
		p = find_entry(cp,param);
		dprintf(7,"p: %s\n", p);
		if (p) return strcpy(cp->data,p);
		if (wait) sleep(1);
	}
	return 0;
}

char *client_get_config(solard_client_t *cp, char *op, char *target, char *param, int timeout, int direct) {
	char topic[SOLARD_TOPIC_SIZE], *p;
	json_value_t *a;

	dprintf(1,"op: %s, target: %s, param: %s, timeout: %d, direct: %d\n", op, target, param, timeout, direct);

	if (!direct && (p = check(cp, param, 0)) != 0) return p;

	a = json_create_array();
	json_array_add_string(a,param);
	json_tostring(a,cp->data,sizeof(cp->data)-1,0);
	json_destroy(a);

	/* Request it */
	sprintf(topic,"%s/%s/%s/%s/%s",SOLARD_TOPIC_ROOT,target,SOLARD_FUNC_CONFIG,op,cp->id);
	dprintf(1,"topic: %s\n",topic);
	mqtt_pub(cp->m,topic,cp->data,0);

	/* Get status message */
	client_get_status(cp, op, param, timeout);

	/* Get value message */
	dprintf(1,"finding...\n");
	p = find_entry(cp, param);

        /* Clear our request and the status reply */
        mqtt_pub(cp->m,topic,0,0);
        sprintf(topic,"%s/%s/%s/%s/Status/%s",SOLARD_TOPIC_ROOT,target,SOLARD_FUNC_CONFIG,op,cp->id);
        mqtt_pub(cp->m,topic,0,0);

	dprintf(1,"returning: %s\n", p);
	return p;
}

list client_get_mconfig(solard_client_t *cp, char *op, char *target, int count, char **params, int timeout) {
	char topic[SOLARD_TOPIC_SIZE], *p;
	json_value_t *a;
	int i;
	list results;

	dprintf(1,"op: %s, target: %s, count: %d, params: %p, timeout: %d\n", op, target, count, params, timeout);

	results = list_create();

	a = json_create_array();

	/* Only fetch the ones we dont have */
	for(i=0; i < count; i++) {
		dprintf(1,"params[%d]: %s\n", i, params[i]);
		if ((p = check(cp, params[i],0)) == 0) json_array_add_string(a,params[i]);
	}
	dprintf(1,"a->count: %d\n",(int)a->value.array->count);
	if (a->value.array->count) {
		json_tostring(a,cp->data,sizeof(cp->data)-1,0);
		dprintf(1,"data: %s\n", cp->data);

		/* Request */
		sprintf(topic,"%s/%s/%s/%s/%s",SOLARD_TOPIC_ROOT,target,SOLARD_FUNC_CONFIG,op,cp->id);
		dprintf(1,"topic: %s\n",topic);
		mqtt_pub(cp->m,topic,cp->data,0);

		/* Get status message */
		client_get_status(cp, op, 0, timeout);

		/* Clear our request and the status reply */
		mqtt_pub(cp->m,topic,0,0);
		sprintf(topic,"%s/%s/%s/%s/Status/%s",SOLARD_TOPIC_ROOT,target,SOLARD_FUNC_CONFIG,op,cp->id);
		mqtt_pub(cp->m,topic,0,0);
	}

	/* Get value message */
	dprintf(1,"finding...\n");
	for(i=0; i < count; i++) {
		if ((p = find_entry(cp, params[i])) != 0) list_add(results,p,strlen(p)+1);
	}

	json_destroy(a);
	dprintf(1,"returning results\n");
	return results;
}

int client_set_config(solard_client_t *cp, char *op, char *target, char *param, char *value, int timeout) {
	char topic[SOLARD_TOPIC_SIZE];
	json_value_t *o,*v;

	dprintf(1,"op: %s, target: %s, param: %s, value: %s\n", op, target, param, value);

	o = json_create_object();
	dprintf(1,"value: %c\n", *value);
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
	dprintf(1,"topic: %s\n",topic);
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

solard_client_t *client_init(int argc,char **argv,opt_proctab_t *client_opts,char *Cname,
			config_property_t *props,config_function_t *funcs) {
	solard_client_t *c;
	char mqtt_info[200],influx_info[200];
	char configfile[256];
	char name[64],sname[CFG_SECTION_NAME_SIZE];
	int config_from_mqtt;
#ifdef JS
	char jsexec[4096];
	int rtsize;
	char script[256];
#endif
	opt_proctab_t std_opts[] = {
		/* Spec, dest, type len, reqd, default val, have */
		{ "-m::|mqtt host,clientid[,user[,pass]]",&mqtt_info,DATA_TYPE_STRING,sizeof(mqtt_info)-1,0,"" },
		{ "-M|get config from mqtt",&config_from_mqtt,DATA_TYPE_LOGICAL,0,0,"N" },
		{ "-c:%|config file",&configfile,DATA_TYPE_STRING,sizeof(configfile)-1,0,"" },
		{ "-s::|config section name",&sname,DATA_TYPE_STRING,sizeof(sname)-1,0,"" },
		{ "-n::|agent name",&name,DATA_TYPE_STRING,sizeof(name)-1,0,"" },
		{ "-i::|influx host[,user[,pass]]",&influx_info,DATA_TYPE_STRING,sizeof(influx_info)-1,0,"" },
#ifdef JS
		{ "-e:%|exectute javascript",&jsexec,DATA_TYPE_STRING,sizeof(jsexec)-1,0,"" },
		{ "-R:#|javascript runtime size",&rtsize,DATA_TYPE_INT,0,0,"67108864" },
		{ "-X::|execute JS script and exit",&script,DATA_TYPE_STRING,sizeof(script)-1,0,"" },
#endif
		OPTS_END
	}, *opts;
	int logopts = LOG_INFO|LOG_WARNING|LOG_ERROR|LOG_SYSERR|_ST_DEBUG;
//	time_t start,now;

	opts = opt_addopts(std_opts,client_opts);
	if (!opts) return 0;
	*mqtt_info = *configfile = 0;
	dprintf(1,"argv: %p\n", argv);
	if (argv && solard_common_init(argc,argv,opts,logopts)) return 0;

	dprintf(1,"creating session...\n");
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
	c->mq = list_create();
	c->agents = list_create();

	dprintf(1,"argc: %d, argv: %p, opts: %p, name: %s\n", argc, argv, opts, name);

	/* call the common startup */
	if (solard_common_startup(&c->cp, sname, configfile, props, funcs, &c->m, client_getmsg, c, mqtt_info, &c->mc, config_from_mqtt, &c->i, influx_info, &c->ic, &c->js, rtsize, (js_outputfunc_t *)log_info)) goto client_init_error;

	{
		config_property_t client_props[] = {
			{ "name", DATA_TYPE_STRING, c->name, sizeof(c->name)-1, 0 },
#ifdef JS
			{ "script_dir", DATA_TYPE_STRING, c->script_dir, sizeof(c->script_dir)-1, 0 },
			{ "start_script", DATA_TYPE_STRING, c->start_script, sizeof(c->start_script)-1, "start.js", 0 },
			{ "stop_script", DATA_TYPE_STRING, c->stop_script, sizeof(c->stop_script)-1, "stop.js", 0 },
#endif
			{0}
		};
	        config_add_props(c->cp, c->name, client_props, 0);
		config_add_props(c->cp, "client", client_props, CONFIG_FLAG_NOSAVE | CONFIG_FLAG_NOID);
	}

#ifdef JS
	/* Execute any command-line javascript code */
	dprintf(dlevel,"jsexec(%d): %s\n", strlen(jsexec), jsexec);
	if (strlen(jsexec)) {
		if (JS_EngineExecString(c->js, jsexec))
			log_warning("error executing js expression\n");
	}

	/* Start the init script */
	snprintf(jsexec,sizeof(jsexec)-1,"%s/%s",c->script_dir,c->init_script);
	if (access(jsexec,0)) JS_EngineExec(c->js,jsexec,0);

	/* if specified, Execute javascript file then exit */
	dprintf(dlevel,"script: %s\n", script);
	if (strlen(script)) {
		JS_EngineExec(c->js,script,0);
		free(c);
		return 0;
	}
#endif

#if 0
	/* Create MQTT session */
	c->m = mqtt_new(client_getmsg, c);
	if (!c->m) {
		log_syserror("unable to create MQTT session\n");
		goto client_init_error;
	}
	dprintf(dlevel,"c->m: %p\n", c->m);

	/* Create InfluxDB session */
	c->i = influx_new();
	if (!c->i) {
		log_syserror("unable to create InfluxDB session\n");
		goto client_init_error;
	}
	dprintf(dlevel,"c->i: %p\n", c->i);

	/* Init the config */
	strncpy(c->section_name,strlen(sname) ? sname : name,CFG_SECTION_NAME_SIZE-1);
	dprintf(1,"section_name: %s\n", c->section_name);
	c->cp = config_init(c->section_name, props, funcs);
	if (!c->cp) return 0;
	common_add_props(c->cp, c->section_name);
	mqtt_add_props(c->cp, &c->gmc, c->section_name, &c->lmc);
	influx_add_props(c->cp, &c->gic, c->section_name, &c->lic);

	if (config_from_mqtt) {
		/* If mqtt info specified on command line, parse it */
		dprintf(dlevel,"mqtt_info: %s\n", mqtt_info);
		if (strlen(mqtt_info)) mqtt_parse_config(&c->lmc,mqtt_info);

		/* init mqtt */
		if (mqtt_init(c->m, &c->lmc)) goto client_init_error;
		c->mqtt_init = 1;

		/* read the config */
//		if (config_from_mqtt(c->cp, topic, c->m)) goto client_init_error;

	} else if (strlen(configfile)) {
		int fmt;
		char *p;

		/* Try to determine format */
		p = strrchr(configfile,'.');
		if (p && (strcasecmp(p,".json") == 0)) fmt = CONFIG_FILE_FORMAT_JSON;
		else fmt = CONFIG_FILE_FORMAT_INI;

		dprintf(dlevel,"reading configfile...\n");
		if (config_read(c->cp,configfile,fmt)) {
			log_error(config_get_errmsg(c->cp));
			goto client_init_error;
		}
	}
	config_dump(c->cp);

	dprintf(1,"name: %s\n", name);
	if (strlen(name)) strncpy(c->name,name,sizeof(c->name)-1);

	/* If MQTT not init, do it now */
	if (!c->mqtt_init) {
		dprintf(1,"mqtt_info: %s\n", mqtt_info);
		if (strlen(mqtt_info)) mqtt_parse_config(&c->lmc, mqtt_info);

		if (mqtt_init(c->m, &c->lmc)) goto client_init_error;
	}

	/* Subscribe to our clientid */
	sprintf(mqtt_info,"%s/%s/%s",SOLARD_TOPIC_ROOT,SOLARD_TOPIC_CLIENTS,c->lmc.clientid);
	mqtt_sub(c->m,mqtt_info);

	/* Sub to the agents */
	sprintf(mqtt_info,"%s/%s/+/#",SOLARD_TOPIC_ROOT,SOLARD_TOPIC_AGENTS);
	dprintf(1,"subscribing to: %s\n", mqtt_info);
	mqtt_sub(c->m,mqtt_info);

#ifdef JS
	/* Init js scripting */
	c->js = JS_EngineInit(rtsize, (js_outputfunc_t *)log_info);
	common_jsinit(c->js);
	config_jsinit(c->js, c->cp);
	client_jsinit(c->js, c);
	mqtt_jsinit(c->js, c->m);
	influx_jsinit(c->js, c->i);
#endif
#endif

	/* Sub to the agents */
	sprintf(mqtt_info,"%s/%s/+/#",SOLARD_TOPIC_ROOT,SOLARD_TOPIC_AGENTS);
	dprintf(1,"subscribing to: %s\n", mqtt_info);
	mqtt_sub(c->m,mqtt_info);

	/* Sleep a moment to ingest any messages */
	usleep(50000);

	return c;
client_init_error:
	free(c);
	return 0;
}

#ifdef JS
static JSBool client_getprop(JSContext *cx, JSObject *obj, jsval id, jsval *rval) {
	solard_client_t *c;

	c = JS_GetPrivate(cx,obj);
	if (!c) return JS_FALSE;
	return config_jsgetprop(cx, obj, id, rval, c->cp, c->props);
}

static JSBool client_setprop(JSContext *cx, JSObject *obj, jsval id, jsval *vp) {
	solard_client_t *c;

	c = JS_GetPrivate(cx,obj);
	if (!c) return JS_FALSE;
	return config_jssetprop(cx, obj, id, vp, c->cp, c->props);
}

static JSClass client_class = {
	"client",		/* Name */
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

static JSBool client_jsmktopic(JSContext *cx, uintN argc, jsval *vp) {
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

JSObject *JSClient(JSContext *cx, void *priv);

static JSBool client_ctor(JSContext *cx, JSObject *obj, uintN argc, jsval *argv, jsval *rval) {
	solard_client_t *c;
	char *name;
	JSObject *newobj;

	dprintf(0,"argc: %d\n", argc);
	if (argc != 1) {
		JS_ReportError(cx, "client requires 1 argument (name:string)");
		return JS_FALSE;
	}

	name = 0;
	if (!JS_ConvertArguments(cx, argc, argv, "s", &name)) return JS_FALSE;
	dprintf(0,"name: %s\n", name);

	c = client_init(0,0,0,name,0,0);
	dprintf(0,"c: %p\n", c);
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

JSObject *JSClient(JSContext *cx, void *priv) {
	JSFunctionSpec client_funcs[] = {
		JS_FN("mktopic",client_jsmktopic,1,1,0),
//		JS_FN("run",client_jsrun,1,1,0),
		{ 0 }
	};
	JSObject *obj;
	solard_client_t *c = priv;
	JSPropertySpec *props;

	props = 0;
	if (c && c->cp && !c->props) {
		c->props = config_to_props(c->cp, c->section_name, 0);
		if (!c->props) {
			log_error("unable to create props: %s\n", config_get_errmsg(c->cp));
			return 0;
		}
		props = c->props;
	}

	dprintf(dlevel,"creating %s object\n",client_class.name);
	obj = JS_InitClass(cx, JS_GetGlobalObject(cx), 0, &client_class, client_ctor, 1, props, client_funcs, 0, 0);
	if (!obj) {
		JS_ReportError(cx,"unable to initialize %s class", client_class.name);
		return 0;
	}
	JS_SetPrivate(cx,obj,priv);
	dprintf(dlevel,"done!\n");
	return obj;
}

int client_jsinit(JSEngine *e, void *priv) {
	return JS_EngineAddInitFunc(e, "client", JSClient, priv);
}
#endif

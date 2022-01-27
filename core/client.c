
/*
Copyright (c) 2021, Stephen P. Shoecraft
All rights reserved.

This source code is licensed under the BSD-style license found in the
LICENSE file in the root directory of this source tree.
*/

#include <pthread.h>
#include "client.h"
#include "uuid.h"

#define DEBUG_STARTUP 1

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
		dprintf(5,"ap->id: %s\n", ap->id);
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
			return 1;
	}
	} else {
		log_error("unable to parse config from: %s\n", ap->target);
		return 1;
	}

	return 0;
}

void agentinfo_dump(client_agentinfo_t *ap, int level) {
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
//	printf("newmsg: topic: %s, id: %s, len: %d, replyto: %s\n", newmsg.topic, newmsg.id, (int)strlen(newmsg.data), newmsg.replyto);
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
		ap = list_add(c->agents,&newagent,sizeof(newagent));
	}
	if (strlen(newmsg.replyto) && !strlen(ap->id)) strncpy(ap->id,newmsg.replyto,sizeof(ap->id)-1);
	if (strcmp(newmsg.func,SOLARD_FUNC_STATUS)==0) {
		if (strcmp(newmsg.data,"Online")==0) ap->online = 1;
	} else if (strcmp(newmsg.func,SOLARD_FUNC_CONFIG)==0) {
		process_config(ap,newmsg.data);
	} else if (strcmp(newmsg.func,SOLARD_FUNC_INFO)==0) {
		process_info(ap,newmsg.data);
	} else {
		/* We did not handle the message */
		list_add(c->mq,&newmsg,sizeof(newmsg));
	}
//	agentinfo_dump(ap,1);
	return;
}

#if 0
static int ping_resp(void *ctx, solard_message_t *msg) {
	client_agentinfo_t *ap = ctx;
	char *p;
	json_value_t *v;

	dprintf(1,"%s\n", msg->data);
	v = json_parse(msg->data);
	p = json_object_dotget_string(json_value_get_object(v),"Pong.ID");
	if (p) {
		strcpy(ap->id, p);
		p = json_object_dotget_string(json_value_get_object(v),"Pong.Target");
		if (p) {
			strcpy(ap->target,p);
			dprintf(1,"pong: %s\n", p);
//			ap->pong = 1;
		}
	}
	json_destroy_value(v);
	return 0;
}
#endif

#if 0
#if 1
static int agent_request(solard_client_t *s, client_agentinfo_t *ap, char *message, int timeout, list agents) {
	char topic[SOLARD_TOPIC_LEN];
	solard_message_t *msg;

	sprintf(topic,"%s/%s",SOLARD_TOPIC_ROOT,ap->target);
	dprintf(1,"topic: %s\n",topic);
	mqtt_pub(s->m,topic,message,1,0);

	/* If timeout, wait for a response */
	ap->have_status = 0;
	if (timeout > 0) {
		while(timeout--) {
			printf("===> have_status: %d\n", ap->have_status);
			process_messages(s, agents, get_status);
			printf("===> have_status: %d\n", ap->have_status);
			if (ap->have_status) break;
#if 0
			dprintf(1,"count: %d\n", list_count(s->mq));
			list_reset(s->mq);
			while((msg = list_get_next(s->mq)) != 0) {
//				dprintf(1,"msg->id: %s, ap->id: %s\n", msg->id, ap->id);
				/* It must be a message TO me FROM the agent */
				dprintf(1,"msg->id: %s, clientid: %s, replyto: %s, ap->id: %s\n",
					msg->id, s->mqtt_config.clientid, msg->replyto, ap->id);
				if (strcmp(msg->id,s->mqtt_config.clientid) == 0 && strcmp(msg->replyto,ap->id) == 0) {
					break;
				}
			}
			if (!found) sleep(1);
			else break;
#endif
			sleep(1);
		}
	}
#if 0
	if (found) {
		printf("%s: %s\n", ap->target, errmsg);
	} else {
		printf("%s: no reply\n", ap->target);
	}
#endif
	return 0;
}
#endif
#endif

#if 0
int client_ping_agent(solard_client_t *s, client_agentinfo_t *ap) {
	char topic[128];
	client_agentinfo_t *ap;
	int all_pong;
	time_t start,now;

	/* Send a ping req */
	sprintf(topic,"%s/%s/%s",SOLARD_TOPIC_ROOT,target,SOLARD_FUNC_PING);
	dprintf(1,"topic: %s\n", topic);
	mqtt_pub(s->m,topic,s->mqtt_config.clientid,1,0);
	all_pong = 0;
	time(&start);
	while(1) {
		dprintf(1,"processing..\n");
		process_messages(s,agents,get_resp);
		all_pong = 1;
		dprintf(1,"count: %d\n", list_count(agents));
		if (list_count(agents)) {
			list_reset(agents);
			while((ap = list_get_next(agents)) != 0) {
				dprintf(1,"id: %s, pong: %d\n", ap->id, ap->pong);
				if (!ap->pong)
					all_pong = 0;
			}
			if (all_pong) break;
		}
		time(&now);
		if ((now - start) > 10)  {
			printf("timeout\n");
			break;
		}
		sleep(1);
	}
	if (all_pong) {
	}
	return 0;
}
#endif

#if 0
static int get_info(solard_client_t *s, char *target,list agents) {
	config_function_t f = { "get_info", 0, 0, 0 };
	json_value_t *v;
	char *m;
	client_agentinfo_t newagent;
	int r;

	/* Call the get info function */
	v = catargs(0,0,&f);
	m = json_dumps(v,0);
	memset(&newagent,0,sizeof(newagent));
	strncpy(newagent.target,target,sizeof(newagent.target)-1);
	r = agent_request(s,&newagent,m,10,agents);
	json_destroy_value(v);
	free(m);
	return r;
}
#endif

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
	dprintf(0,"f->nargs: %d\n", f->nargs);
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
	dprintf(0,"==> PUB: %s, j: %s\n", topic, j);
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

int client_config_init(solard_client_t *c, config_property_t *client_props, config_function_t *client_funcs) {
	config_property_t common_props[] = {
		{ "bindir", DATA_TYPE_STRING, SOLARD_BINDIR, sizeof(SOLARD_BINDIR)-1, 0 },
		{ "etcdir", DATA_TYPE_STRING, SOLARD_ETCDIR, sizeof(SOLARD_ETCDIR)-1, 0 },
		{ "libdir", DATA_TYPE_STRING, SOLARD_LIBDIR, sizeof(SOLARD_LIBDIR)-1, 0 },
		{ "logdir", DATA_TYPE_STRING, SOLARD_LOGDIR, sizeof(SOLARD_LOGDIR)-1, 0 },
		{ "debug", DATA_TYPE_INT, &debug, 0, 0, 0,
			"range", 3, (int []){ 0, 99, 1 }, 1, (char *[]) { "debug level" }, 0, 1, 0 },
		{ 0 }
	};
	config_property_t mqtt_props[] = {
		{ "host", DATA_TYPE_STRING, c->mqtt_config.host, sizeof(c->mqtt_config.host)-1, "localhost", 0,
                        0, 0, 0, 1, (char *[]){ "MQTT host" }, 0, 1, 0 },
		{ "port", DATA_TYPE_INT, &c->mqtt_config.port, 0, "1883", 0,
			0, 0, 0, 1, (char *[]){ "MQTT port" }, 0, 1, 0 },
		{ "clientid", DATA_TYPE_STRING, c->mqtt_config.clientid, sizeof(c->mqtt_config.clientid)-1, "", 0,
                        0, 0, 0, 1, (char *[]){ "MQTT clientid" }, 0, 1, 0 },
		{ "username", DATA_TYPE_STRING, c->mqtt_config.username, sizeof(c->mqtt_config.username)-1, "", 0,
                        0, 0, 0, 1, (char *[]){ "MQTT username" }, 0, 1, 0 },
		{ "password", DATA_TYPE_STRING, c->mqtt_config.password, sizeof(c->mqtt_config.password)-1, "", 0,
                        0, 0, 0, 1, (char *[]){ "MQTT password" }, 0, 1, 0 },
		{ 0 }
	};
	config_property_t influx_props[] = {
		{ "host", DATA_TYPE_STRING, c->influx_config.host, sizeof(c->influx_config.host)-1, "localhost", 0,
                        0, 0, 0, 1, (char *[]){ "InfluxDB host" }, 0, 1, 0 },
		{ "port", DATA_TYPE_INT, &c->influx_config.port, 0, "8086", 0,
			0, 0, 0, 1, (char *[]){ "InfluxDB port" }, 0, 1, 0 },
		{ "database", DATA_TYPE_STRING, c->influx_config.database, sizeof(c->influx_config.database)-1, "", 0,
                        0, 0, 0, 1, (char *[]){ "InfluxDB database" }, 0, 1, 0 },
		{ "username", DATA_TYPE_STRING, c->influx_config.username, sizeof(c->influx_config.username)-1, "", 0,
                        0, 0, 0, 1, (char *[]){ "InfluxDB username" }, 0, 1, 0 },
		{ "password", DATA_TYPE_STRING, c->influx_config.password, sizeof(c->influx_config.password)-1, "", 0,
                        0, 0, 0, 1, (char *[]){ "InfluxDB password" }, 0, 1, 0 },
		{ 0 }
	};

	c->cp = config_init(c->name, client_props, client_funcs);
	if (!c->cp) return 1;
	dprintf(1,"adding common...\n");
	config_add_props(c->cp, c->name, common_props, 0);
	dprintf(1,"adding client...\n");
	config_add_props(c->cp, c->name, client_props, 0);
	config_add_funcs(c->cp, client_funcs);
	dprintf(1,"adding mqtt...\n");
	config_add_props(c->cp, "mqtt", mqtt_props, CONFIG_FLAG_NOID);
	dprintf(1,"adding influx...\n");
	config_add_props(c->cp, "influx", influx_props, CONFIG_FLAG_NOID);

	/* Add the mqtt props to the instance config */
	{
		char *names[] = { "mqtt_host", "mqtt_port", "mqtt_clientid", "mqtt_username", "mqtt_password", 0 };
		config_section_t *s;
		config_property_t *p;
		int i;

		s = config_get_section(c->cp, c->name);
		if (!s) {
			log_error("%s section does not exist?!?\n", c->name);
			return 1;
		}
		for(i=0; names[i]; i++) {
			p = config_section_dup_property(s, &mqtt_props[i], names[i]);
			if (!p) continue;
			dprintf(1,"p->name: %s\n",p->name);
			config_section_add_property(c->cp, s, p, CONFIG_FLAG_NOID);
		}
	}

	dprintf(1,"done!\n");
	return 0;
}

int client_mqtt_init(solard_client_t *c) {

//	if (c->mqtt_init) return 0;

	dprintf(1,"host: %s, clientid: %s, user: %s, pass: %s\n",
		c->mqtt_config.host, c->mqtt_config.clientid, c->mqtt_config.username, c->mqtt_config.password);
	if (!strlen(c->mqtt_config.host)) {
		log_write(LOG_WARNING,"mqtt host not specified, using localhost\n");
		strcpy(c->mqtt_config.host,"localhost");
	}

	/* Create a unique ID for MQTT */
	if (!strlen(c->mqtt_config.clientid)) {
		uint8_t uuid[16];

		dprintf(1,"gen'ing MQTT ClientID...\n");
		uuid_generate_random(uuid);
		my_uuid_unparse(uuid, c->mqtt_config.clientid);
		dprintf(4,"clientid: %s\n",c->mqtt_config.clientid);
	}

	/* Create LWT topic */
//	client_mktopic(c->mqtt_config.lwt_topic,sizeof(c->mqtt_config.lwt_topic)-1,ap,c->name,"Status");

	/* Create a new client */
	if (mqtt_newclient(c->m, &c->mqtt_config)) goto client_mqtt_init_error;

	/* Connect to the server */
	if (mqtt_connect(c->m,20)) goto client_mqtt_init_error;

//	c->mqtt_init = 0;
	return 0;
client_mqtt_init_error:
	return 1;
}

solard_client_t *client_init(int argc,char **argv,opt_proctab_t *client_opts,char *name,config_property_t *props,config_function_t *funcs) {
	solard_client_t *c;
	char mqtt_info[200];
	char configfile[256];
	char sname[CFG_SECTION_NAME_SIZE];
	opt_proctab_t std_opts[] = {
		/* Spec, dest, type len, reqd, default val, have */
		{ "-m::|mqtt host:port,clientid[,user[,pass]]",&mqtt_info,DATA_TYPE_STRING,sizeof(mqtt_info)-1,0,"" },
		{ "-c:%|config file",&configfile,DATA_TYPE_STRING,sizeof(configfile)-1,0,"" },
		{ "-n::|config section name",&sname,DATA_TYPE_STRING,sizeof(sname)-1,0,"" },
		OPTS_END
	}, *opts;
	int logopts = LOG_INFO|LOG_WARNING|LOG_ERROR|LOG_SYSERR|_ST_DEBUG;
//	time_t start,now;

	opts = opt_addopts(std_opts,client_opts);
	if (!opts) return 0;
	*mqtt_info = *configfile = 0;
	if (solard_common_init(argc,argv,opts,logopts)) return 0;

	dprintf(1,"creating session...\n");
	c = calloc(1,sizeof(*c));
	if (!c) {
		log_syserror("client_init: malloc(%d)",sizeof(*c));
		return 0;
	}
	strncpy(c->name,name,sizeof(c->name)-1);
//	pthread_mutex_init(&c->lock, 0);
	c->mq = list_create();
	c->agents = list_create();

	dprintf(1,"argc: %d, argv: %p, opts: %p, name: %s\n", argc, argv, opts, name);

	strncpy(c->section_name,strlen(sname) ? sname : name,CFG_SECTION_NAME_SIZE-1);
	dprintf(1,"section_name: %s\n", c->section_name);
	dprintf(1,"configfile: %s\n", configfile);
	if (strlen(configfile)) {
		cfg_proctab_t agent_tab[] = {
//			{ c->section_name, "id", "Client ID", DATA_TYPE_STRING,&c->name,sizeof(c->name), id },
			CFG_PROCTAB_END
		};
		c->cfg = cfg_read(configfile);
		if (c->cfg) {
			struct mqtt_config *conf = &c->mqtt_config;

			cfg_get_tab(c->cfg,agent_tab);
			if (debug) cfg_disp_tab(agent_tab,"agent",0);
			struct cfg_proctab tab[] = {
				{ "mqtt", "broker", "Broker URL", DATA_TYPE_STRING,&c->mqtt_config.host,sizeof(conf->host), 0 },
				{ "mqtt", "clientid", "Client ID", DATA_TYPE_STRING,&conf->clientid,sizeof(conf->clientid), 0 },
				{ "mqtt", "username", "Broker username", DATA_TYPE_STRING,&conf->username,sizeof(conf->username), 0 },
				{ "mqtt", "password", "Broker password", DATA_TYPE_STRING,&conf->password,sizeof(conf->password), 0 },
				CFG_PROCTAB_END
			};

			cfg_get_tab(c->cfg,tab);
#ifdef DEBUG
			if (debug) cfg_disp_tab(tab,"MQTT",0);
#endif
		}
	} else {
		dprintf(1,"mqtt_info: %s\n", mqtt_info);
		if (!strlen(mqtt_info)) strcpy(mqtt_info,"localhost");
		strncpy(c->mqtt_config.host,strele(0,",",mqtt_info),sizeof(c->mqtt_config.host)-1);
		strncpy(c->mqtt_config.clientid,strele(1,",",mqtt_info),sizeof(c->mqtt_config.clientid)-1);
		strncpy(c->mqtt_config.username,strele(2,",",mqtt_info),sizeof(c->mqtt_config.username)-1);
		strncpy(c->mqtt_config.password,strele(3,",",mqtt_info),sizeof(c->mqtt_config.password)-1);
		dprintf(1,"host: %s, clientid: %s, user: %s, pass: %s\n", c->mqtt_config.host, c->mqtt_config.clientid, c->mqtt_config.username, c->mqtt_config.password);
	}
	dprintf(1,"configfile: %s\n", configfile);

	/* MQTT Init */
	if (!strlen(c->mqtt_config.host)) {
		log_write(LOG_WARNING,"No MQTT host specified, using localhost");
		strcpy(c->mqtt_config.host,"localhost");
	}
	/* MQTT requires a unique ClientID for connections */
	if (!strlen(c->mqtt_config.clientid)) {
		uint8_t uuid[16];

		dprintf(1,"gen'ing MQTT ClientID...\n");
		uuid_generate_random(uuid);
		my_uuid_unparse(uuid, c->mqtt_config.clientid);
		dprintf(4,"NEW clientid: %s\n",c->mqtt_config.clientid);
	}
	c->m = mqtt_new(client_getmsg,c);
	if (!c->m) goto client_init_error;
        if (mqtt_newclient(c->m, &c->mqtt_config)) goto client_init_error;
	if (mqtt_connect(c->m,20)) goto client_init_error;

	/* Subscribe to our clientid */
	sprintf(mqtt_info,"%s/%s/%s",SOLARD_TOPIC_ROOT,SOLARD_TOPIC_CLIENTS,c->mqtt_config.clientid);
	mqtt_sub(c->m,mqtt_info);

	/* Sub to the agents */
	sprintf(mqtt_info,"%s/%s/+/#",SOLARD_TOPIC_ROOT,SOLARD_TOPIC_AGENTS);
	dprintf(1,"subscribing to: %s\n", mqtt_info);
	mqtt_sub(c->m,mqtt_info);

	printf("sleeping...\n");
	usleep(50000);
	printf("done...\n");

	return c;
client_init_error:
	free(c);
	return 0;
}

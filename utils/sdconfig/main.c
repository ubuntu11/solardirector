
/*
Copyright (c) 2021, Stephen P. Shoecraft
All rights reserved.

This source code is licensed under the BSD-style license found in the
LICENSE file in the root directory of this source tree.
*/

#include "sdconfig.h"

#define TESTING 0

static int agent_request(solard_client_t *s, client_agent_info_t *ap, char *message, int timeout) {
	char topic[SOLARD_TOPIC_LEN];
	solard_message_t *msg;
	json_value_t *v;
	int status,found,count;
	char *errmsg;

	dprintf(1,"===> message:\n%s\n", message);

	sprintf(topic,"%s/%s",SOLARD_TOPIC_ROOT,ap->id);
	dprintf(1,"topic: %s\n",topic);
	mqtt_pub(s->m,topic,message,1,0);

#if 0
	count = 0;
	while(1) {
		/* Wait for message from that ID */
		msg = solard_message_wait_id(s->mq, ap->id, 1);
		dprintf(1,"msg: %p\n", msg);
		if (!msg) {
			log_error("timeout waiting for reply from %s\n", ap->target);
			return 1;
		}
		solard_message_dump(msg,1);
		v = json_parse(msg->data);
		str = json_object_get_string(json_value_get_object(v),"status");
		if (str) {
					if (v && json_value_get_type(v) == JSON_TYPE_OBJECT) {
						status = json_object_get_number(json_value_get_object(v),"status");
						errmsg = json_object_get_string(json_value_get_object(v),"message");
						dprintf(1,"status: %d, errmsg: %s\n", status, errmsg);
						found = 1;
						break;
					}
		count++;
	}
#else
	/* If timeout, wait for a response */
	status = 1;
	errmsg = "error";
	found = 0;
	if (timeout > 0) {
		while(timeout--) {
			dprintf(1,"count: %d\n", list_count(s->mq));
			list_reset(s->mq);
			while((msg = list_get_next(s->mq)) != 0) {
				/* If the message role is my ID, its a reply */
//				solard_message_dump(msg,1);
				dprintf(1,"msg->id: %s, clientid: %s\n", msg->id, s->mqtt_config.clientid);
				if (strcmp(msg->id,s->mqtt_config.clientid) == 0) {
					dprintf(1,"===> response: %s\n", msg->data);
					v = json_parse(msg->data);
					dprintf(1,"v: %p\n", v);
					if (v && json_value_get_type(v) == JSON_TYPE_OBJECT) {
						status = json_object_get_number(json_value_get_object(v),"status");
						errmsg = json_object_get_string(json_value_get_object(v),"message");
						dprintf(1,"status: %d, errmsg: %s\n", status, errmsg);
						found = 1;
						list_delete(s->mq,msg);
						break;
					}
				}
			}
			if (!found) sleep(1);
			else break;
		}
	}
	if (found) {
		printf("%s: %s\n", ap->target, errmsg);
	} else {
		printf("%s: no reply\n", ap->target);
	}
#endif
	return 0;
}

static json_value_t *catargs(int argc,char *argv[],config_function_t *f) {
	json_object_t *o;
	json_array_t *a;
	int i;

	/* nargs = 1: Object with function name and array of arguments */
	/* { "func": [ "arg", "argn", ... ] } */
	/* nargs = 2: Object with function name and array of objects with key:value pairs */
	/* { "func": [ { "key": "value" }, { "keyn": "valuen" }, ... ] } */
	/* nargs > 2: Object with function name and array of argument arrays */
	/* { "func": [ [ "arg", "argn", ... ], [ "arg", "argn" ], ... ] } */
	o = json_create_object();
	a = json_create_array();
	if (f->nargs == 1) {
		for(i=0; i < argc; i++) json_array_add_string(a,argv[i]);
	} else if (f->nargs == 2) {
		json_object_t *oo;

		i = 0;
		while(i < argc) {
			oo = json_create_object();
			json_object_set_string(oo,argv[i],argv[i+1]);
			json_array_add_object(a,oo);
			i += 2;
		}
	} else if (f->nargs > 2) {
		printf("notimpl\n");
	}
	json_object_set_array(o,f->name,a);
	return json_object_get_value(o);
}

void parse_funcs(client_agent_info_t *ap) {
	json_object_t *o;
	json_array_t *a;
	config_function_t newfunc;
	char *p;
	int i;

	/* Functions is an array of objects */
	/* each object has name and args */
	a = json_object_get_array(json_value_get_object(ap->info), "functions");
	dprintf(1,"a: %p\n", a);
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
		dprintf(1,"adding: %s\n", newfunc.name);
		list_add(ap->funcs,&newfunc,sizeof(newfunc));
	}
}

config_function_t *get_func(client_agent_info_t *ap, char *name) {
	config_function_t *f;

	if (ap->funcs) {
		list_reset(ap->funcs);
		while((f = list_get_next(ap->funcs)) != 0) {
			if (strcmp(f->name,name)==0)
				return f;
		}
	}
	return 0;
}

int ping_agent(client_agent_info_t *ap) {
	return 0;
}

void usage() {
	printf("usage: sdconfig <target> <func> <item> [<value>]\n");
	exit(1);
}

int main(int argc,char **argv) {
	char message[8192];
	char topic[128],*p;
	char target[SOLARD_NAME_LEN];
	char func[SOLARD_FUNC_LEN];
	solard_client_t *s;
	solard_message_t *msg;
	int timeout,read_flag,list_flag,no_flag,retries;
	opt_proctab_t opts[] = {
                /* Spec, dest, type len, reqd, default val, have */
		{ "-t:#|wait time",&timeout,DATA_TYPE_INT,0,0,"10" },
		{ "-r|ignore persistant config",&read_flag,DATA_TYPE_BOOL,0,0,"0" },
		{ "-n|do not ping agent",&no_flag,DATA_TYPE_BOOL,0,0,"0" },
		{ "-l|list parameters",&list_flag,DATA_TYPE_BOOL,0,0,"0" },
		{ ":target|agent name",&target,DATA_TYPE_STRING,sizeof(target)-1,1,0 },
		{ ":func|agent func",&func,DATA_TYPE_STRING,sizeof(func)-1,0,0 },
		OPTS_END
	};
	list agents;
	json_value_t *v;
	client_agent_info_t newagent,*ap;
	time_t start,now;
	config_function_t *f;
#if TESTING
	char *args[] = { "sdconfig", "-d", "6", "-l", "Battery/jbd" };
	argc = (sizeof(args)/sizeof(char *));
	argv = args;
#endif

	debug = 4;
	log_open("sd",0,0xffff);

	s = client_init(argc,argv,opts,"sdconfig");
	if (!s) return 1;

	dprintf(1,"target: %s, func: %s, %d\n", target, func);
	if (!strlen(func) && !list_flag) {
		log_error("either list flag or func must be specified\n");
		return 1;
	}

        argc -= optind;
        argv += optind;
        optind = 0;

	agents = list_create();
	if (!agents) {
		log_syserror("list_create");
		return 1;
	}

	if (!no_flag) {
	/* Send a "ping" request */
	dprintf(1,"sending ping...\n");
	sprintf(topic,"%s/%s/Ping",SOLARD_TOPIC_ROOT,target);
	mqtt_pub(s->m,topic,s->mqtt_config.clientid,1,0);

	/* Get the replie(s) */
	time(&start);
	while(1) {
		list_reset(s->mq);
		while((msg = list_get_next(s->mq)) != 0) {
//			solard_message_dump(msg,1);
			dprintf(1,"id: %s, clientid: %s\n", msg->id,s->mqtt_config.clientid);
			if (strcmp(msg->id,s->mqtt_config.clientid)!=0) {
				list_delete(s->mq,msg);
				continue;
			}
			memset(&newagent,0,sizeof(newagent));
			ap = &newagent;
			ap->funcs = list_create();
			v = json_parse(msg->data);
			if (!v) {
				dprintf(1,"%s: not a valid json struct\n",msg->data);
				continue;
			}
			p = json_object_dotget_string(json_value_get_object(v),"Pong.ID");
			if (p) {
				strcpy(ap->id,p);
				p = json_object_dotget_string(json_value_get_object(v),"Pong.Target");
				if (p) strcpy(ap->target,p);
			}
			json_destroy_value(v);
			if (!strlen(ap->target)) continue;
			dprintf(1,"adding: id: %s, target: %s\n",ap->id,ap->target);
			list_add(agents,&newagent,sizeof(newagent));
			list_delete(s->mq,msg);
		}
		time(&now);
		if ((now - start) > 1)  break;
		sleep(1);
	}
	if (!list_count(agents)) {
		log_error("no agents responded\n");
		return 1;
	}
	} else {
		memset(&newagent,0,sizeof(newagent));
		ap = &newagent;
		ap->funcs = list_create();
		strncpy(ap->target,target,sizeof(ap->target)-1);
		list_add(agents,&newagent,sizeof(newagent));
	}

	/* For each agent, get the info */
	list_reset(agents);
	while((ap = list_get_next(agents)) != 0) {
		dprintf(0,"===> agent: %s\n", ap->target);

		/* Sub to topic */
		sprintf(topic,"%s/%s/%s",SOLARD_TOPIC_ROOT,ap->target,SOLARD_FUNC_INFO);
		if (mqtt_sub(s->m,topic)) return 0;

		dprintf(0,"===> info: %s\n", ap->info);
		retries = 3;
		while(retries--) {
			/* Ingest info */
			dprintf(0,"waiting for info from: %s\n", ap->target);
			msg = solard_message_wait_target(s->mq, ap->target, 5);
			printf("data: %s\n", msg->data);
			if (!msg) {
				config_function_t gi = { "get_info", 0, 0, 0 };

				/* Call the get info function */
				v = catargs(0,0,&gi);
				json_tostring(v,message,sizeof(message)-1,1);
				if (agent_request(s,ap,message,5)) break;
				json_destroy_value(v);
			} else {
				/* parse the info */
				printf("msg->data: %s\n", msg->data);
				ap->info = json_parse(msg->data);
				if (ap->info) {
					if (json_value_get_type(ap->info) != JSON_TYPE_OBJECT) {
						log_error("invalid info data from: %s\n", ap->target);
						break;
					}
					if (no_flag) {
						char *str = json_object_get_string(json_value_get_object(ap->info),"agent_id");
						if (str) strcpy(ap->id,str);
						else strcpy(ap->id,msg->replyto);
					}
					/* Parse the funcs */
					parse_funcs(ap);
				} else {
					log_error("unable to parse info from: %s\n", ap->target);
					break;
				}
				list_delete(s->mq, msg);
			}
			if (ap->info) break;
		}
		if (retries >= 3) log_error("timeout waiting for info from: %s\n", ap->target);
	}

	if (list_flag) {
		list_reset(agents);
		while((ap = list_get_next(agents)) != 0) do_list(ap);
		return 0;
	}

	/* For each agent ... */
	list_reset(agents);
	while((ap = list_get_next(agents)) != 0) {
		printf("====> id: %s\n", ap->id);
		if (!strlen(ap->id)) return 1;
		f = get_func(ap,func);
		if (!f) {
			log_error("target %s does not support function %s\n", ap->target, func);
			continue;
		}
		dprintf(1,"argc: %d, nargs: %d\n", argc, f->nargs);
		/* Args must be divisible by nargs */
		if (!f->nargs && argc) {
			log_warning("function %s/%s takes %d arguments but %d given, arguments ignored\n",
				ap->target, f->name, f->nargs, argc);
		} else if (f->nargs && (argc % f->nargs) != 0) {
			log_error("insufficient arguments: function %s/%s requires %d arguments but %d given\n",
				ap->target, f->name, f->nargs, argc);
			continue;
		}

		v = catargs(argc,argv,f);
		json_tostring(v,message,sizeof(message)-1,1);
		if (agent_request(s,ap,message,10)) continue;
		json_destroy_value(v);
	}

	return 0;
}

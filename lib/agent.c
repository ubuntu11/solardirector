
/*
Copyright (c) 2021, Stephen P. Shoecraft
All rights reserved.

This source code is licensed under the BSD-style license found in the
LICENSE file in the root directory of this source tree.
*/

#include <time.h>
#include <ctype.h>
#include "agent.h"
#include <sys/signal.h>

#define DEBUG_STARTUP 0

#if DEBUG_STARTUP
#define _ST_DEBUG LOG_DEBUG
#else
#define _ST_DEBUG 0
#endif

static int suspend_read = 0;
static int suspend_write = 0;
void usr1_handler(int signo) {
	if (signo == SIGUSR1) {
		log_write(LOG_INFO,"agent: %s reads\n", (suspend_read ? "resuming" : "suspending"));
		suspend_read = (suspend_read ? 0 : 1);
	} else if (signo == SIGUSR2) {
		log_write(LOG_INFO,"agent: %s writes\n", (suspend_write ? "resuming" : "suspending"));
		suspend_write = (suspend_write ? 0 : 1);
	}
}

int agent_set_callback(solard_agent_t *ap, solard_agent_callback_t cb) {
	ap->cb = cb;
	return 0;
}

/* Get driver handle from role */
void *agent_get_handle(solard_agent_t *ap) {
	return ap->role->get_handle(ap->role_handle);
}

int agent_send_status(solard_agent_t *ap, char *name, char *func, char *op, char *id, int status, char *message) {
	char topic[200],msg[4096];
	json_value_t *o;

	dprintf(1,"op: %s, status: %d, message: %s\n", op, status, message);

	o = json_create_object();
	json_add_number(o,"status",status);
	json_add_string(o,"message",message);
	json_tostring(o,msg,sizeof(msg)-1,0);
	json_destroy(o);
	/* /root/role/name/func/op/Status/id */
	sprintf(topic,"%s/%s/%s/%s/%s/Status/%s",SOLARD_TOPIC_ROOT,ap->role->name,name,func,op,id);
	dprintf(1,"topic: %s\n", topic);
	mqtt_pub(ap->m,topic,msg,0);
	return 0;
}

list agent_config_payload2list(solard_agent_t *ap, char *name, char *func, char *op, char *id, char *payload) {
	json_value_t *v;
	list lp;
	int i;
	solard_confreq_t newreq;

	dprintf(1,"name: %s, op: %s, payload: %s\n", name, op, payload);

	/* Payload must either be an array of param names for get or param name/value pairs for set */

	lp = list_create();
	if (!lp) {
		agent_send_status(ap,name,func,op,id,1,"internal error");
		return 0;
	}

	/* Parse payload */
	v = json_parse(payload);
	if (!v) {
		agent_send_status(ap,name,func,op,id,1,"error parsing json string");
		goto agent_config_payload_error;
	}

	/* If Get, must be array of strings */
	if (strcmp(op,"Get")== 0) {
		json_array_t *a;
		json_value_t *av;
		char *p;

		dprintf(1,"type: %s(%d)\n", json_typestr(v->type), v->type);
		if (v->type != JSONArray) {
			agent_send_status(ap,name,func,op,id,1,"invalid format");
			goto agent_config_payload_error;
		}
		/* Process items */
		a = v->value.array;
		dprintf(1,"a->count: %d\n", (int)a->count);
		for(i=0; i < a->count; i++) {
			av = a->items[i];
			dprintf(1,"av type: %d(%s)\n", av->type, json_typestr(av->type));
			p = (char *)json_string(av);
			dprintf(1,"p: %s\n", p);
			list_add(lp,p,strlen(p)+1);
		}
	/* If Set, must be object of key/value pairs */
	} else if (strcmp(op,"Set")== 0) {
		json_object_t *o;
		json_value_t *ov;

		dprintf(1,"type: %s(%d)\n", json_typestr(v->type), v->type);
		if (v->type != JSONObject) {
			agent_send_status(ap,name,func,op,id,1,"invalid format");
			goto agent_config_payload_error;
		}
		o = v->value.object;
		dprintf(1,"o->count: %d\n", (int)o->count);
		for(i=0; i < o->count; i++) {
			ov = o->values[i];
			dprintf(1,"ov name: %s, type: %s(%d)\n", o->names[i], json_typestr(ov->type),ov->type);
			memset(&newreq,0,sizeof(newreq));
			strncat(newreq.name,o->names[i],sizeof(newreq.name)-1);
			if (ov->type == JSONNumber) {
				newreq.dval = ov->value.number;
				newreq.type = DATA_TYPE_F64;
			} else if (ov->type == JSONString) {
				strncat(newreq.sval,json_string(ov),sizeof(newreq.sval)-1);
				newreq.type = DATA_TYPE_STRING;
			} else if (ov->type == JSONBoolean) {
				newreq.bval = ov->value.boolean;
				newreq.type = DATA_TYPE_BOOL;
			}
			list_add(lp,&newreq,sizeof(newreq));
		}
	} else {
		agent_send_status(ap,name,func,op,id,1,"unsupported action");
		goto agent_config_payload_error;
	}

	json_destroy(v);
	dprintf(1,"returning: %p\n", lp);
	return lp;
agent_config_payload_error:
	json_destroy(v);
	dprintf(1,"returning: 0\n");
	return 0;
}

int agent_config(solard_agent_t *ap, list lp) {
	return 0;
}

void agent_process(solard_agent_t *ap, solard_message_t *msg) {
	char *message;
	list lp;
	long start;

	dprintf(1,"msg: name: %s, func: %s, action: %s, id: %s\n", msg->name, msg->func, msg->action, msg->id);

	start = mem_used();
	if (strcmp(msg->func,"Config")==0) {
		lp = agent_config_payload2list(ap,msg->name,msg->func,msg->action,msg->id,msg->data);
		if (!lp) {
			message = "internal error";
			goto agent_process_error;
		}
		if (ap->role->config) ap->role->config(ap->role_handle,msg->action,msg->id,lp);
		list_destroy(lp);
	} else if (strcmp(msg->func,"Control")==0) {
		json_value_t *v;

		v = json_parse(msg->data);
		dprintf(1,"v: %p\n", v);
		if (!v) {
			message = "invalid json format";
			goto agent_process_error;
		}
		if (ap->role->control) ap->role->control(ap->role_handle,msg->action,msg->id,v);
		json_destroy(v);
	}
	dprintf(1,"used: %ld\n", mem_used() - start);
	return;

agent_process_error:
	agent_send_status(ap,msg->name,msg->func,msg->action,msg->id,1,message);
	return;
}


int agent_sub(solard_agent_t *ap, char *name, char *func, char *action, char *id) {
	char topic[200],*p;

	dprintf(1,"name: %s, func: %s, action: %s, id: %s\n", name, func, action, id);
	/* /Solard/Battery/+/Config/Get/Status */
	p = topic;
	p += sprintf(p,"%s/%s",SOLARD_TOPIC_ROOT,ap->role->name);
	if (name) {
		p += snprintf(p,sizeof(topic)-strlen(topic)-1,"/%s",name);
		if (func) {
			p += snprintf(p,sizeof(topic)-strlen(topic)-1,"/%s",func);
			if (action) {
				p += snprintf(p,sizeof(topic)-strlen(topic)-1,"/%s",action);
				if (id) {
					p += snprintf(p,sizeof(topic)-strlen(topic)-1,"/%s",id);
				}
			}
		}
	}
	dprintf(1,"topic: %s\n", topic);
	return mqtt_sub(ap->m,topic);
}

solard_agent_t *agent_init(int argc, char **argv, opt_proctab_t *agent_opts, solard_module_t *driver) {
	solard_agent_t *ap;
	char tp_info[128],mqtt_info[200],*info,*p;
	char configfile[256];
	mqtt_config_t mqtt_config;
	char transport[SOLARD_AGENT_TRANSPORT_LEN+1];
	char target[SOLARD_AGENT_TARGET_LEN+1];
	char topts[64];
	char sname[64];
	int info_flag,pretty_flag,read_interval,write_interval;
	opt_proctab_t std_opts[] = {
		/* Spec, dest, type len, reqd, default val, have */
		{ "-t::|transport:target:opts",&tp_info,DATA_TYPE_STRING,sizeof(tp_info)-1,0,"" },
		{ "-m::|mqtt host:clientid[:user[:pass]]",&mqtt_info,DATA_TYPE_STRING,sizeof(mqtt_info)-1,0,"" },
		{ "-c:%|config file",&configfile,DATA_TYPE_STRING,sizeof(configfile)-1,0,"" },
		{ "-n::|config section name",&sname,DATA_TYPE_STRING,sizeof(sname)-1,0,"" },
		{ "-I|agent info",&info_flag,DATA_TYPE_LOGICAL,0,0,"0" },
		{ "-P|pretty print json output",&pretty_flag,DATA_TYPE_LOGICAL,0,0,0 },
		{ "-i:#|reporting interval",&read_interval,DATA_TYPE_INT,0,0,"30" },
		{ "-w:#|update (write) interval",&write_interval,DATA_TYPE_INT,0,0,"30" },
		OPTS_END
	}, *opts;
	int logopts = LOG_INFO|LOG_WARNING|LOG_ERROR|LOG_SYSERR|_ST_DEBUG;
	module_t *tp;
	void *tp_handle,*driver_handle;
	list names;

	opts = opt_addopts(std_opts,agent_opts);
	if (!opts) return 0;
	tp_info[0] = mqtt_info[0] = configfile[0] = 0;
	if (solard_common_init(argc,argv,opts,logopts)) return 0;

	ap = calloc(1,sizeof(*ap));
	if (!ap) {
		log_write(LOG_SYSERR,"calloc agent config");
		return 0;
	}
	ap->modules = list_create();
	ap->mq = list_create();
	ap->pretty = pretty_flag;
	ap->read_interval = read_interval;
	ap->write_interval = write_interval;

	/* Could load these as modules, but not really needed here */
	switch(driver->type) {
	case SOLARD_MODTYPE_BATTERY:
		ap->role = &battery;
		break;
	case SOLARD_MODTYPE_INVERTER:
		ap->role = &inverter;
		break;
	default:
		break;
	}

	strncat(ap->section_name,strlen(sname) ? sname : driver->name,sizeof(ap->section_name)-1);
	dprintf(1,"section_name: %s\n", ap->section_name);
	dprintf(1,"configfile: %s\n", configfile);
	if (strlen(configfile)) {
		cfg_proctab_t agent_tab[] = {
			{ ap->section_name, "id", "Agent ID", DATA_TYPE_STRING,&ap->id,sizeof(ap->id), 0 },
			{ ap->section_name, "name", "Agent name", DATA_TYPE_STRING,&ap->name,sizeof(ap->name), 0 },
			{ ap->section_name, "transport", "Transport", DATA_TYPE_STRING,&transport,sizeof(transport), 0 },
			{ ap->section_name, "target", "Transport address/interface/device", DATA_TYPE_STRING,&target,sizeof(target), 0 },
			{ ap->section_name, "topts", "Transport specific options", DATA_TYPE_STRING,&topts,sizeof(topts), 0 },
			CFG_PROCTAB_END
		};
		transport[0] = target[0] = topts[0] = 0;
		ap->cfg = cfg_read(configfile);
		if (!ap->cfg) goto agent_init_error;
		cfg_get_tab(ap->cfg,agent_tab);
		if (debug) cfg_disp_tab(agent_tab,"agent",0);
		memset(&mqtt_config,0,sizeof(mqtt_config));
		if (mqtt_get_config(ap->cfg,&mqtt_config)) goto agent_init_error;
	} 

	if (strlen(tp_info)) {
#if 0
		if (!strlen(tp_info) || !strlen(mqtt_info)) {
			log_write(LOG_ERROR,"either configfile or transport and mqtt info must be specified.\n");
			goto agent_init_error;
		}
#endif
		strncat(transport,strele(0,":",tp_info),sizeof(transport)-1);
		strncat(target,strele(1,":",tp_info),sizeof(target)-1);
		strncat(topts,strele(2,":",tp_info),sizeof(topts)-1);
	}

	if (strlen(mqtt_info)) {
		strncat(mqtt_config.host,strele(0,":",mqtt_info),sizeof(mqtt_config.host)-1);
		strncat(mqtt_config.clientid,strele(1,":",mqtt_info),sizeof(mqtt_config.clientid)-1);
		strncat(mqtt_config.user,strele(2,":",mqtt_info),sizeof(mqtt_config.user)-1);
		strncat(mqtt_config.pass,strele(3,":",mqtt_info),sizeof(mqtt_config.pass)-1);
	}
	if (!strlen(ap->name)) strcpy(ap->name,ap->section_name);

	/* MQTT Init */
	dprintf(1,"host: %s, clientid: %s, user: %s, pass: %s\n",
		mqtt_config.host, mqtt_config.clientid, mqtt_config.user, mqtt_config.pass);
	if (!strlen(mqtt_config.host)) {
		log_write(LOG_ERROR,"mqtt host must be specified\n");
		goto agent_init_error;
	}
	if (!strlen(mqtt_config.clientid)) strncat(mqtt_config.clientid,ap->name,MQTT_CLIENTID_LEN-1);
	ap->m = mqtt_new(&mqtt_config, solard_getmsg, ap->mq);
	if (mqtt_connect(ap->m,20)) goto agent_init_error;
	mqtt_disconnect(ap->m,10);

	/* Role Init */
	if (!strlen(transport) || !strlen(target)) {
		log_write(LOG_ERROR,"transport and target must be specified\n");
		goto agent_init_error;
	}
	dprintf(1,"target: %s\n", target);
	if (!strlen(target)) return 0;

	dprintf(1,"transport: %s, target: %s, topts: %s\n", transport, target, topts);
	/* Load the transport driver */
	tp = load_module(ap->modules,transport,SOLARD_MODTYPE_TRANSPORT);
	if (!tp) goto agent_init_error;

	/* Create an instance of the transport driver */
	tp_handle = tp->new(ap,target,topts);

	/* Create an instance of the agent driver */
	driver_handle = driver->new(ap,tp,tp_handle);
	if (!driver_handle) goto agent_init_error;

	/* Create an instance of the role driver */
	ap->role_handle = ap->role->new(ap,driver,driver_handle);

	/* Get info */
	ap->info = ap->role->info(ap->role_handle);
	if (!ap->info) goto agent_init_error;
	info = json_dumps(ap->info,0);

	/* If info flag, dump info then exit */
	dprintf(1,"info_flag: %d\n", info_flag);
	if (info_flag) {
		printf("%s\n",info);
		free(ap);
		return 0;
	}

	/* Callback - must be done before connect */
//	if (mqtt_setcb(ap->m,ap,0,agent_callback0)) return 0;
	mqtt_connect(ap->m,20);

	/* Publish our Info */
	sprintf(mqtt_info,"%s/%s/%s/%s",SOLARD_TOPIC_ROOT,ap->role->name,ap->name,SOLARD_FUNC_INFO);
	mqtt_pub(ap->m,mqtt_info,info,1);
	json_free_string(info);

	names = list_create();
	list_add(names,"all",4);
	if (strcmp(ap->name,driver->name) != 0) list_add(names,driver->name,strlen(driver->name)+1);
	list_add(names,ap->name,strlen(ap->name)+1);
	p = json_get_string(ap->info,"device_model");
	dprintf(1,"p: %s\n", p);
	if (p && strlen(p)) list_add(names,p,strlen(p)+1);

	/* Sub to all messages for the instance */
	list_reset(names);
	while((p = list_get_next(names)) != 0) {
		agent_sub(ap,p,SOLARD_FUNC_CONFIG,SOLARD_ACTION_GET,"+");
		agent_sub(ap,p,SOLARD_FUNC_CONFIG,SOLARD_ACTION_SET,"+");
		agent_sub(ap,p,SOLARD_FUNC_CONTROL,SOLARD_ACTION_SET,"+");
	}

	/* Set the read toggle handler */
//	signal(SIGUSR1, usr1_handler);

	dprintf(1,"returning: %p\n",ap);
	return ap;
agent_init_error:
	free(ap);
	return 0;
}

int agent_run(solard_agent_t *ap) {
	solard_message_t *msg;
	int read_status;
	uint32_t mem_start;
	time_t last_read,last_write,cur,diff;

	dprintf(1,"ap: %p\n", ap);

//	pthread_mutex_init(&ap->lock, 0);

	last_read = last_write = 0;
	mem_start = mem_used();
	read_status = 0;
	solard_set_state(ap,SOLARD_AGENT_RUNNING);
	while(solard_check_state(ap,SOLARD_AGENT_RUNNING)) {
		/* Call read func */
		time(&cur);
		diff = cur - last_read;
		dprintf(3,"diff: %d, read_interval: %d\n", (int)diff, ap->read_interval);
		if (suspend_read == 0 && diff >= ap->read_interval) {
			dprintf(1,"reading...\n");
			read_status = ap->role->read(ap->role_handle,0,0);
			time(&last_read);
			dprintf(1,"used: %ld\n", mem_used() - mem_start);
		}

		/* Process messages */
		list_reset(ap->mq);
		while((msg = list_get_next(ap->mq)) != 0) {
			agent_process(ap,msg);
			list_delete(ap->mq,msg);
		}

		/* Skip rest if failed to read */
		if (read_status != 0) {
			sleep(1);
			continue;
		}

		/* Call cb */
		if (ap->cb) ap->cb(ap);

		/* Call write func */
		time(&cur);
		diff = cur - last_write;
		dprintf(3,"diff: %d, write_interval: %d\n", (int)diff, ap->write_interval);
		if (read_status == 0 && suspend_write == 0 && diff >= ap->write_interval) {
			dprintf(1,"writing...\n");
			ap->role->write(ap->role_handle,0,0);
			time(&last_write);
			dprintf(1,"used: %ld\n", mem_used() - mem_start);
		}
		sleep(1);
	}
	mqtt_disconnect(ap->m,10);
	mqtt_destroy(ap->m);
	return 0;
}

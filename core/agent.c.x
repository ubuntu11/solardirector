
/*
Copyright (c) 2021, Stephen P. Shoecraft
All rights reserved.

This source code is licensed under the BSD-style license found in the
LICENSE file in the root directory of this source tree.
*/

#define DEBUG_STARTUP 1

#if DEBUG_STARTUP
#define _ST_DEBUG LOG_DEBUG
#else
#define _ST_DEBUG 0
#endif

#include <time.h>
#include <ctype.h>
#include "agent.h"
#include "uuid.h"

extern solard_driver_t null_transport;

struct _mhinfo {
	solard_msghandler_t *func;
	void *ctx;
};
typedef struct _mhinfo mhinfo_t;

static int suspend_read = 0;
static int suspend_write = 0;
#ifndef __WIN32
#include <sys/signal.h>

void usr_handler(int signo) {
	if (signo == SIGUSR1) {
		log_write(LOG_INFO,"agent: %s reads\n", (suspend_read ? "resuming" : "suspending"));
		suspend_read = (suspend_read ? 0 : 1);
	} else if (signo == SIGUSR2) {
		log_write(LOG_INFO,"agent: %s writes\n", (suspend_write ? "resuming" : "suspending"));
		suspend_write = (suspend_write ? 0 : 1);
	}
}
#endif

int agent_set_callback(solard_agent_t *ap, solard_agent_callback_t cb, void *ctx) {
	ap->callback.func = cb;
	ap->callback.ctx = ctx;
	return 0;
}

//void agent_mktopic(char *topic, int topicsz, solard_agent_t *ap, char *name, char *func, char *action, char *id) {
void agent_mktopic(char *topic, int topicsz, solard_agent_t *ap, char *name, char *func) {
	register char *p;

//	dprintf(1,"name: %s, func: %s, action: %s, id: %s\n", name, func, action, id);
	dprintf(1,"name: %s, func: %s\n", name, func);

	/* /Solard/Battery/+/Config/Get/Status */
	p = topic;
	p += snprintf(p,topicsz-strlen(topic),"%s",SOLARD_TOPIC_ROOT);
	if (ap->role) p += snprintf(p,topicsz-strlen(topic),"/%s",ap->role->name);
	if (name) p += snprintf(p,topicsz-strlen(topic),"/%s",name);
	if (func) p += snprintf(p,topicsz-strlen(topic),"/%s",func);
//	if (action) p += snprintf(p,topicsz-strlen(topic),"/%s",action);
//	if (id) p += snprintf(p,topicsz-strlen(topic),"/%s",id);
}

int agent_sub(solard_agent_t *ap, char *name, char *func) {
	char topic[SOLARD_TOPIC_LEN];

	dprintf(1,"name: %s, func: %s\n", name, func);

	agent_mktopic(topic,sizeof(topic)-1,ap,name,func);
	log_write(LOG_INFO,"%s\n", topic);
	return mqtt_sub(ap->m,topic);
}

int agent_pub(solard_agent_t *ap, char *func, char *message, int retain) {
	char topic[SOLARD_TOPIC_LEN];

	dprintf(1,"func: %s, message: %s, retain: %d\n", func, message, retain);

	agent_mktopic(topic,sizeof(topic)-1,ap,ap->instance_name,func);
        dprintf(1,"topic: %s\n", topic);
        return mqtt_pub(ap->m,topic,message,1,retain);
}

int agent_reply(solard_agent_t *ap, char *replyto, int status, char *message) {
	char topic[SOLARD_TOPIC_LEN];
	char payload[2048];
	json_value_t *o;

	dprintf(1,"ap->m: %p\n", ap->m);

	dprintf(1,"replyto: %s, status: %d, message(%d): %s\n", replyto, status, strlen(message),message);

	o = json_create_object();
	json_add_number(o,"status",status);
	json_add_string(o,"message",message);
	json_tostring(o,payload,sizeof(payload)-1,0);
	json_destroy(o);

	snprintf(topic,sizeof(topic)-1,"%s/%s",SOLARD_TOPIC_ROOT,replyto);
	dprintf(1,"topic: %s, payload: %s\n", topic, payload);
	return mqtt_pub(ap->m,topic,payload,1,0);
}

static int agent_mh(void *ctx, solard_message_t *msg) {
//	solard_agent_t *ap = ctx;
#if 0
	proctab_t tab[] = {
		{ "debug", &debug, DATA_TYPE_INT, 0 },
		PROCTAB_END
	};
#endif

	dprintf(1,"called!\n");

#if 0
        char name[SOLARD_NAME_LEN];
        cfg_info_t *cfg;
        mqtt_config_t mqtt_config;
        char section_name[CFG_SECTION_NAME_SIZE];
        mqtt_session_t *m;              /* MQTT Session handle */
        int config_from_mqtt;
        int read_interval;
        int write_interval;
//      list modules;                   /* list of loaded modules */
        list mq;                        /* incoming message queue */
        list mh;                        /* Message handlers */
        uint16_t state;                 /* States */
        int pretty;                     /* Format json messages for readability (uses more mem) */
        solard_driver_t *role;
        void *role_handle;
        char instance_name[SOLARD_NAME_LEN]; /* Agent instance name */
        void *role_data;                /* Role-specific data */
        json_value_t *info;             /* Info returned by role/driver */
        struct {
                solard_agent_callback_t func;   /* Called between read and write */
                void *ctx;
        } callback;
};
#endif

	/* We currently dont handle any messages */
	return 1;
}

void agent_add_msghandler(solard_agent_t *ap, solard_msghandler_t *func, void *ctx) {
	mhinfo_t newmhinfo;

	newmhinfo.func = func;
	newmhinfo.ctx = ctx;
	list_add(ap->mh,&newmhinfo,sizeof(newmhinfo));
}

static int agent_config_from_mqtt(solard_agent_t *ap) {
	char topic[SOLARD_TOPIC_SIZE];
	solard_message_t *msg;
//	list lp;

	/* Sub to our config topic */
	agent_mktopic(topic,sizeof(topic)-1,ap,ap->instance_name,SOLARD_FUNC_CONFIG);
	if (mqtt_sub(ap->m,topic)) return 1;

	/* Ingest config */
	dprintf(1,"ingesting...\n");
	solard_ingest(ap->mq,2);
	dprintf(1,"back from ingest...\n");

	list_reset(ap->mq);
	while((msg = list_get_next(ap->mq)) != 0) {
		solard_message_dump(msg,1);
	}
#if 0
		/* Process messages as config requests */
		lp = list_create();
		dprintf(1,"mq count: %d\n", list_count(s->ap->mq));
		list_reset(s->ap->mq);
		while((msg = list_get_next(s->ap->mq)) != 0) {
			memset(&req,0,sizeof(req));
			strncat(req.name,msg->param,sizeof(req.name)-1);
			req.type = DATA_TYPE_STRING;
			strncat(req.sval,msg->data,sizeof(req.sval)-1);
			dprintf(1,"req: name: %s, type: %d(%s), sval: %s\n", req.name, req.type, typestr(req.type), req.sval);
			list_add(lp,&req,sizeof(req));
			list_delete(s->ap->mq,msg);
		}
		dprintf(1,"lp count: %d\n", list_count(lp));
		if (list_count(lp)) si_config(s,"Set","si_control",lp);
		list_destroy(lp);
#endif
	mqtt_unsub(ap->m,topic);
	return 0;
}

void agent_getmsg(void *ctx, char *topic, char *message, int msglen, char *replyto) {
	solard_agent_t *ap = ctx;
	solard_message_t *msg;

	msg = solard_getmsg(topic,message,msglen,replyto);
	if (!msg) return;

	/* Reply to Ping req immediately */
	dprintf(4,"func: %s\n", msg->func);
	if (strcmp(msg->func,"Ping") == 0 && replyto) {
		char data[256];
		char target[SOLARD_ROLE_LEN+SOLARD_NAME_LEN];
		char topic[SOLARD_TOPIC_LEN];
		json_value_t *v,*vv;

		v = json_create_object();
		vv = json_create_object();
		if (!v || !vv) {
			log_syserror("agent_getmsg: json_create_object");
			return;
		}
		json_add_string(vv,"ID",ap->mqtt_config.clientid);
		sprintf(target,"%s/%s",ap->role->name,ap->instance_name);
		json_add_string(vv,"Target",target);
		json_add_value(v,"Pong",vv);
		json_dumps_r(v,data,sizeof(data)-1);
		json_destroy(v);
		sprintf(topic,"%s/%s",SOLARD_TOPIC_ROOT,replyto);
		mqtt_pub(ap->m,topic,data,0,0);
	} else {
		/* Otherwise add it to our list */
		list_add(ap->mq,msg,sizeof(*msg));
	}
}

/* 
	Get command line options
	Get config
	Init transport
	Init agent
	Init role
	Init MQTT
*/
solard_agent_t *agent_init(int argc, char **argv, opt_proctab_t *agent_opts, solard_driver_t *driver,
		solard_driver_t *transports[], solard_driver_t *role) {
	solard_agent_t *ap;
	char info[65536];
	char tp_info[128],mqtt_info[200],*p;
	char configfile[256];
	char name[SOLARD_NAME_LEN];
	char transport[SOLARD_TRANSPORT_LEN];
	char target[SOLARD_TARGET_LEN];
	char topts[SOLARD_TOPTS_LEN];
	char sname[64];
	int info_flag,pretty_flag,read_interval,write_interval,conf_from_mqtt;
	opt_proctab_t std_opts[] = {
		/* Spec, dest, type len, reqd, default val, have */
		{ "-t::|transport,target,opts",&tp_info,DATA_TYPE_STRING,sizeof(tp_info)-1,0,"" },
		{ "-m::|mqtt host,clientid[,user[,pass]]",&mqtt_info,DATA_TYPE_STRING,sizeof(mqtt_info)-1,0,"" },
		{ "-M|get config from mqtt",&conf_from_mqtt,DATA_TYPE_LOGICAL,0,0,"N" },
		{ "-c:%|config file",&configfile,DATA_TYPE_STRING,sizeof(configfile)-1,0,"" },
		{ "-s::|config section name",&sname,DATA_TYPE_STRING,sizeof(sname)-1,0,"" },
		{ "-n::|agent name",&name,DATA_TYPE_STRING,sizeof(name)-1,0,"" },
		{ "-I|agent info",&info_flag,DATA_TYPE_LOGICAL,0,0,"0" },
		{ "-P|pretty print json output",&pretty_flag,DATA_TYPE_LOGICAL,0,0,0 },
		{ "-r:#|reporting (read) interval",&read_interval,DATA_TYPE_INT,0,0,"30" },
		{ "-w:#|update (write) interval",&write_interval,DATA_TYPE_INT,0,0,"30" },
		OPTS_END
	}, *opts;
	int logopts = LOG_INFO|LOG_WARNING|LOG_ERROR|LOG_SYSERR|_ST_DEBUG;
	solard_driver_t *tp;
	void *tp_handle,*driver_handle;
	list names;

	if (!driver) {
		printf("agent_init: agent driver is null, aborting\n");
		exit(0);
	}

	opts = opt_addopts(std_opts,agent_opts);
	dprintf(1,"opts: %p\n", opts);
	if (!opts) {
		printf("error initializing options processing\n");
		return 0;
	}
	if (solard_common_init(argc,argv,opts,logopts)) {
		dprintf(1,"common init failed!\n");
		return 0;
	}

	/* Create session */
	ap = calloc(1,sizeof(*ap));
	if (!ap) {
		log_write(LOG_SYSERR,"calloc agent config");
		return 0;
	}
	ap->role = role;
	ap->mq = list_create();
	ap->mh = list_create();
	ap->pretty = pretty_flag;
	ap->read_interval = read_interval;
	ap->write_interval = write_interval;
	ap->config_from_mqtt = conf_from_mqtt;

	/* Add our message handler */
	agent_add_msghandler(ap,agent_mh,ap);

	/* Agent name is driver name */
	strncat(ap->name,driver->name,sizeof(ap->name)-1);

	/* Read the config section for the agent */
	strncpy(ap->section_name,strlen(sname) ? sname : (strlen(name) ? name : driver->name), sizeof(ap->section_name)-1);
	dprintf(1,"section_name: %s\n", ap->section_name);
	dprintf(1,"configfile: %s\n", configfile);
	*transport = *target = *topts = 0;
	if (strlen(configfile)) {
		cfg_proctab_t agent_tab[] = {
			{ ap->section_name, "bindir", "default binary location", DATA_TYPE_STRING,&SOLARD_BINDIR,sizeof(SOLARD_BINDIR), 0 },
			{ ap->section_name, "etcdir", "default config location", DATA_TYPE_STRING,&SOLARD_ETCDIR,sizeof(SOLARD_ETCDIR), 0 },
			{ ap->section_name, "libdir", "default library location", DATA_TYPE_STRING,&SOLARD_LIBDIR,sizeof(SOLARD_LIBDIR), 0 },
			{ ap->section_name, "logdir", "default log location", DATA_TYPE_STRING,&SOLARD_LOGDIR,sizeof(SOLARD_LOGDIR), 0 },
			{ ap->section_name, "name", 0, DATA_TYPE_STRING,&ap->instance_name,sizeof(ap->instance_name)-1, 0 },
#ifdef DEBUG
			{ ap->section_name, "debug", 0, DATA_TYPE_INT,&debug,0, 0 },
#endif
			{ ap->section_name, "transport", "Transport", DATA_TYPE_STRING,&transport,sizeof(transport), 0 },
			{ ap->section_name, "target", "Transport address/interface/device", DATA_TYPE_STRING,&target,sizeof(target), 0 },
			{ ap->section_name, "topts", "Transport specific options", DATA_TYPE_STRING,&topts,sizeof(topts), 0 },
			CFG_PROCTAB_END
		};
		*transport = *target = *topts = 0;
		ap->cfg = cfg_read(configfile);
		if (!ap->cfg) {
			log_write(LOG_SYSERR,"initializing config file processing\n");
			goto agent_init_error;
		}
		cfg_get_tab(ap->cfg,agent_tab);
#ifdef DEBUG
		if (debug) cfg_disp_tab(agent_tab,"agent",0);
#endif
		if (mqtt_get_config(ap->cfg,&ap->mqtt_config)) {
			log_write(LOG_SYSERR,"getting MQTT config\n");
			goto agent_init_error;
		}
	} 

	/* Name specified on command line overrides configfile */
	if (strlen(name)) strcpy(ap->instance_name,name);

	/* If still no instance name, set it to driver name */
	if (!strlen(ap->instance_name)) strncpy(ap->instance_name,driver->name,sizeof(ap->instance_name)-1);

	/* tp_info (command line) takes precedence */
	dprintf(1,"tp_info: %s\n", tp_info);
	if (strlen(tp_info)) {
		strncat(transport,strele(0,",",tp_info),sizeof(transport)-1);
		strncat(target,strele(1,",",tp_info),sizeof(target)-1);
		strncat(topts,strele(2,",",tp_info),sizeof(topts)-1);
	}

	/* If no transport and target specified, use null */
	dprintf(1,"transport: %s, target: %s, topts: %s\n", transport, target, topts);
	if (!strlen(transport) || !strlen(target)) {
		tp = &null_transport;
	} else {
		solard_driver_t *dp;
		int i;

		/* Find the transport in the list of transports */
		tp = 0;
		for(i=0; transports[i]; i++) {
			dp = transports[i];
			dprintf(1,"dp->type: %d\n", dp->type);
			if (dp->type != SOLARD_DRIVER_TRANSPORT) continue;
			dprintf(1,"dp->name: %s\n", dp->name);
			if (strcmp(dp->name,transport)==0) {
				tp = dp;
				break;
			}
		}
		dprintf(1,"tp: %p\n", tp);
		if (!tp) goto agent_init_error;
	}

	/* Create an instance of the transport driver */
	tp_handle = tp->new(ap,target,topts);
	dprintf(1,"tp_handle: %p\n", tp_handle);
	if (!tp_handle) goto agent_init_error;

	/* Create an instance of the agent driver */
	driver_handle = driver->new(ap,tp,tp_handle);
	dprintf(1,"driver_handle: %p\n", driver_handle);
	if (!driver_handle) goto agent_init_error;

	/* Create an instance of the role driver */
	if (ap->role) ap->role_handle = ap->role->new(ap,driver,driver_handle);
	dprintf(1,"ap->role_handle: %p\n", ap->role_handle);

	dprintf(1,"mqtt_info: %s\n", mqtt_info);
	if (strlen(mqtt_info) && mqtt_parse_config(&ap->mqtt_config,mqtt_info)) goto agent_init_error;

	/* MQTT Init */
	dprintf(1,"host: %s, clientid: %s, user: %s, pass: %s\n",
		ap->mqtt_config.host, ap->mqtt_config.clientid, ap->mqtt_config.user, ap->mqtt_config.pass);
	if (!strlen(ap->mqtt_config.host)) {
		log_write(LOG_WARNING,"mqtt host not specified, using localhost\n");
		strcpy(ap->mqtt_config.host,"localhost");
	}

	/* MQTT requires a unique ClientID for connections */
	if (!strlen(ap->mqtt_config.clientid)) {
		uint8_t uuid[16];

		dprintf(1,"gen'ing MQTT ClientID...\n");
		uuid_generate_random(uuid);
		my_uuid_unparse(uuid, ap->mqtt_config.clientid);
		dprintf(4,"clientid: %s\n",ap->mqtt_config.clientid);
	}

	/* Create LWT topic */
//	agent_mktopic(ap->mqtt_config.lwt_topic,sizeof(ap->mqtt_config.lwt_topic)-1,ap,ap->instance_name,"Status",0,0);
	agent_mktopic(ap->mqtt_config.lwt_topic,sizeof(ap->mqtt_config.lwt_topic)-1,ap,ap->instance_name,"Status");
//	sprintf(ap->mqtt_config.lwt_topic,"%s/%s/%s/%s",SOLARD_TOPIC_ROOT,ap->role->name,ap->instance_name,SOLARD_FUNC_STATUS);

	/* Create MQTT session */
	ap->m = mqtt_new(&ap->mqtt_config, agent_getmsg, ap);
	if (!ap->m) {
		log_syserror("unable to initialize MQTT\n");
		return 0;
	}
	dprintf(1,"ap->m: %p\n", ap->m);

	/* Callback - must be done before connect */
//	if (mqtt_setcb(ap->m,ap,0,agent_callback0)) return 0;

	/* Connect to the server */
	if (mqtt_connect(ap->m,20)) goto agent_init_error;

	/* Init the config */
	ap->role->config(ap->role_handle,SOLARD_CONFIG_INIT);

	/* Get config from mqtt? */
	if (conf_from_mqtt) agent_config_from_mqtt(ap);

	/* Get info */
	if (ap->role) {
		if (ap->role->config(ap->role_handle,SOLARD_CONFIG_GET_INFO,&ap->info)) {
			log_error("unable to get info from agent driver");
			goto agent_init_error;
		}
		dprintf(1,"ap->info: %p\n", ap->info);
		if (!ap->info) goto agent_init_error;
		json_dumps_r(ap->info,info,sizeof(info));

		/* If info flag, dump info then exit */
		dprintf(1,"info_flag: %d\n", info_flag);
		if (info_flag) {
			printf("%s\n",info);
			exit(0);
		}
	}

	/* Publish our Info */
//	sprintf(mqtt_info,"%s/%s/%s/%s",SOLARD_TOPIC_ROOT,ap->role->name,ap->instance_name,SOLARD_FUNC_INFO);
	agent_mktopic(mqtt_info,sizeof(mqtt_info)-1,ap,ap->instance_name,SOLARD_FUNC_INFO);
	mqtt_pub(ap->m,mqtt_info,info,1,1);

	names = list_create();
	list_add(names,"all",4);
	list_add(names,"All",4);
	list_add(names,ap->name,strlen(ap->name)+1);
	if (strcmp(ap->name,ap->instance_name) != 0) list_add(names,ap->instance_name,strlen(ap->instance_name)+1);
//	list_add(names,ap->mqtt_config.clientid,strlen(ap->mqtt_config.clientid)+1);
	p = json_get_string(ap->info,"device_model");
	dprintf(1,"p: %s\n", p);
	if (p && strlen(p)) list_add(names,p,strlen(p)+1);

	dprintf(1,"ap->m: %p\n", ap->m);

	/* Sub to all messages for the instance */
	list_reset(names);
//	log_write(LOG_INFO,"Subscribing to the following topics:\n");
	while((p = list_get_next(names)) != 0) agent_sub(ap,p,"Ping");

	dprintf(1,"ap->m: %p\n", ap->m);

	/* Also sub to our clientid */
	sprintf(mqtt_info,"%s/%s",SOLARD_TOPIC_ROOT,ap->mqtt_config.clientid);
	log_write(LOG_INFO,mqtt_info);
	mqtt_sub(ap->m,mqtt_info);

	dprintf(1,"returning: %p\n",ap);
	return ap;
agent_init_error:
	free(ap);
	dprintf(1,"returning 0\n");
	return 0;
}

int agent_run(solard_agent_t *ap) {
	solard_message_t *msg;
	int read_status,found;
	uint32_t mem_start;
	time_t last_read,last_write,cur,diff;
	mhinfo_t *mhinfo;

	dprintf(1,"ap: %p\n", ap);

//	pthread_mutex_init(&ap->lock, 0);

	last_read = last_write = 0;
	mem_start = mem_used();
	read_status = 1;
	solard_set_state(ap,SOLARD_AGENT_RUNNING);
	while(solard_check_state(ap,SOLARD_AGENT_RUNNING)) {
		/* If the main script is not running, start it */
//		if (!script_running(ap->scripts.monitor)) script_exec(ap->scripts.monitor);

		/* Call read func */
		time(&cur);
		diff = cur - last_read;
		dprintf(5,"diff: %d, read_interval: %d\n", (int)diff, ap->read_interval);
		if (suspend_read == 0 && diff >= ap->read_interval) {
			dprintf(5,"reading...\n");
			if (ap->role) read_status = ap->role->read(ap->role_handle,0,0);
			time(&last_read);
			dprintf(5,"used: %ld\n", mem_used() - mem_start);
		}

		/* Process messages */
		list_reset(ap->mq);
		while((msg = list_get_next(ap->mq)) != 0) {
			solard_message_dump(msg,1);
			/* Call each message handler func */
			found=0;
			list_reset(ap->mh);
			while((mhinfo = list_get_next(ap->mh)) != 0) {
				if (mhinfo->func(mhinfo->ctx,msg) == 0) {
					found=1;
					break;
				}
			}
			/* If not handled, call the role driver config func with MESSAGE req */
			if (!found && ap->role && ap->role->config)
				ap->role->config(ap->role_handle,SOLARD_CONFIG_MESSAGE,msg);
			list_delete(ap->mq,msg);
		}

		/* Skip rest if failed to read */
		dprintf(5,"read_status: %d\n", read_status);
		if (read_status != 0) {
			sleep(1);
			continue;
		}

		/* Call cb */
		dprintf(5,"func: %p\n", ap->callback.func);
		if (ap->callback.func) ap->callback.func(ap->callback.ctx);

		/* Call write func */
		time(&cur);
		diff = cur - last_write;
		dprintf(5,"diff: %d, write_interval: %d\n", (int)diff, ap->write_interval);
		if (read_status == 0 && suspend_write == 0 && diff >= ap->write_interval) {
			dprintf(5,"writing...\n");
			if (ap->role) ap->role->write(ap->role_handle,0,0);
			time(&last_write);
			dprintf(5,"used: %ld\n", mem_used() - mem_start);
		}
		sleep(1);
	}
	mqtt_disconnect(ap->m,10);
	mqtt_destroy(ap->m);
	return 0;
}

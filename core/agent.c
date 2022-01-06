
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
#ifdef JS
#include "jsengine.h"
#include <pthread.h>
#endif

extern FILE *logfp;

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

void agent_mktopic(char *topic, int topicsz, solard_agent_t *ap, char *name, char *func) {
	register char *p;

	dprintf(1,"name: %s, func: %s\n", name, func);

	/* /Solard/Battery/+/Config/Get/Status */
	p = topic;
	p += snprintf(p,topicsz-strlen(topic),"%s",SOLARD_TOPIC_ROOT);
	if (name) p += snprintf(p,topicsz-strlen(topic),"/%s",name);
	if (func) p += snprintf(p,topicsz-strlen(topic),"/%s",func);
}

int agent_sub(solard_agent_t *ap, char *name, char *func) {
	char topic[SOLARD_TOPIC_LEN];

	dprintf(1,"name: %s, func: %s\n", name, func);

	agent_mktopic(topic,sizeof(topic)-1,ap,name,func);
	log_info("%s\n", topic);
	return mqtt_sub(ap->m,topic);
}

int agent_pub(solard_agent_t *ap, char *func, char *message, int retain) {
	char topic[SOLARD_TOPIC_LEN];
	char temp[32];

	strncpy(temp,message,31);
	dprintf(1,"func: %s, message: %s, retain: %d\n", func, temp, retain);

	agent_mktopic(topic,sizeof(topic)-1,ap,ap->instance_name,func);
        dprintf(1,"topic: %s\n", topic);
        return mqtt_pub(ap->m,topic,message,1,retain);
}

int agent_pubinfo(solard_agent_t *ap, int disp) {
	char *info;
	int r;

	if (!ap->driver_info) return 1;

	info = json_dumps(ap->driver_info, 1);
	if (!info) {
		log_syserror("agent_pubinfo: json_dump");
		return 1;
	}
	if (disp) {
		printf("%s\n",info);
		r = 0;
	} else {
		r = agent_pub(ap, SOLARD_FUNC_INFO, info, 1);
	}
	free(info);
	return r;
}

int agent_pubconfig(solard_agent_t *ap) {
	char *data;
	json_value_t *v;
	int r;

	v = config_to_json(ap->cp);
	if (!v) {
		log_error(config_get_errmsg(ap->cp));
		return 1;
	}
	data = json_dumps(v, 1);

	r = agent_pub(ap, SOLARD_FUNC_CONFIG, data, 1);
	free(data);
	return r;
}

int agent_reply(solard_agent_t *ap, char *replyto, int status, char *message) {
	char topic[SOLARD_TOPIC_LEN];
	char payload[2048];
	json_object_t *o;

	dprintf(1,"ap->m: %p\n", ap->m);

	dprintf(1,"replyto: %s, status: %d, message(%d): %s\n", replyto, status, strlen(message),message);

	o = json_create_object();
	json_object_set_number(o,"status",status);
	json_object_set_string(o,"message",message);
	json_tostring(json_object_get_value(o),payload,sizeof(payload)-1,0);
	json_destroy_object(o);

	snprintf(topic,sizeof(topic)-1,"%s/%s",SOLARD_TOPIC_ROOT,replyto);
	dprintf(1,"topic: %s, payload: %s\n", topic, payload);
	return mqtt_pub(ap->m,topic,payload,1,0);
}

static int agent_config_from_mqtt(solard_agent_t *ap) {
	char topic[SOLARD_TOPIC_SIZE];
	solard_message_t *msg;
//	list lp;

	/* Sub to our config topic */
	agent_mktopic(topic,sizeof(topic)-1,ap,ap->instance_name,SOLARD_FUNC_CONFIG);
	if (mqtt_sub(ap->m,topic)) return 1;

	/* Ingest config */
	if (solard_message_wait(ap->mq,5) < 1) {
		log_error("timeout getting config from %s\n",topic);
		return 1;
	}

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

static int agent_process(solard_agent_t *ap, solard_message_t *msg) {
	int status;

	status = config_process(ap->cp, msg->data);
	return agent_reply(ap,msg->replyto,status,config_get_errmsg(ap->cp));
}

void agent_getmsg(void *ctx, char *topic, char *message, int msglen, char *replyto) {
	solard_agent_t *ap = ctx;
	solard_message_t *msg;

	msg = solard_getmsg(topic,message,msglen,replyto);
	if (!msg) return;

	/* Reply to Ping req immediately */
	dprintf(4,"func: %s\n", msg->func);
	if (strcmp(msg->func,"Ping") == 0) {
		char data[256];
		char target[SOLARD_ROLE_LEN+SOLARD_NAME_LEN];
		char topic[SOLARD_TOPIC_LEN];
		json_object_t *o,*oo;

		dprintf(1,"answering...\n");
		o = json_create_object();
		oo = json_create_object();
		if (!o || !oo) {
			log_syserror("agent_getmsg: json_create_object");
			return;
		}
		json_object_set_string(oo,"ID",ap->mqtt_config.clientid);
		sprintf(target,"%s",ap->instance_name);
		json_object_set_string(oo,"Target",target);
		json_object_set_object(o,"Pong",oo);
		json_dumps_r(json_object_get_value(o),data,sizeof(data)-1);
		json_destroy_object(o);
		dprintf(1,"publishing...\n");
		sprintf(topic,"%s/%s",SOLARD_TOPIC_ROOT,replyto);
		mqtt_pub(ap->m,topic,data,0,0);
	} else {
		/* Otherwise add it to our list */
		list_add(ap->mq,msg,sizeof(*msg));
	}
}

#if JS
#if 0
struct jsargs {
	JSEngine *js;
	char script[256];
	int newcx;
};
	
static void *_dojsexec(void *ctx) {
	struct jsargs *args = ctx;
	JSEngine *e;

	if (args->newcx) {
		e = JS_DupEngine(args->js);
	} else {
		e = args->js;
	}
	if (e) JS_EngineExec(e, args->script, args->newcx);
	free(args);
	return 0;
}
#endif

int agent_start_script(solard_agent_t *ap, char *name) {
	char script[256];
//	pthread_t th;

	dprintf(5,"name: %s, script_dir: %s\n", name, ap->script_dir);
	if (strlen(ap->script_dir)) 
		sprintf(script,"%s/%s",ap->script_dir,name);
	else
		strcpy(script,name);
	dprintf(5,"script: %s\n", script);
	if (access(script,0) != 0 && access(name,0) == 0) strcpy(script,name);
	return JS_EngineExec(ap->js, script, 0);
#if 0
	dprintf(5,"script: %s, bg: %d\n",script,bg);

	if (bg) {
		pthread_attr_t attr;
		struct jsargs *args;
		int status;

		args = calloc(sizeof(*args),1);
		if (!args) return 1;
		args->js = ap->js;
		strncpy(args->script,script,sizeof(args->script)-1);
		args->newcx = newcx;

		status = pthread_attr_init(&attr);
		if (status != 0) { /* . */}
		status = pthread_attr_setscope(&attr, PTHREAD_SCOPE_SYSTEM);
		if (status != 0) { /* . */}
		if (pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_DETACHED)) {
			sprintf(ap->errmsg,"pthread_attr_setdetachstate: %s",strerror(errno));
		}
		status = pthread_create(&th, &attr, _dojsexec, args);
		if (status != 0) { /* . */}
		status = pthread_attr_destroy(&attr); // destroy attribute object
		if (status != 0) { /* . */} 
		return 0;
	} else {
		return JS_EngineExec(ap->js, script, newcx);
	}
	return 1;
#endif
}
#else
#define agent_start_script(a,b,c,d) /* noop */
#endif

int cf_agent_getinfo(void *ctx, list args, char *errmsg) {
	sprintf(errmsg,"publish error");
	return agent_pubinfo((solard_agent_t *)ctx,0);
}

int agent_config_init(solard_agent_t *ap, config_property_t *driver_props, config_function_t *driver_funcs) {
	config_property_t common_props[] = {
		{ "bindir", DATA_TYPE_STRING, SOLARD_BINDIR, sizeof(SOLARD_BINDIR)-1, 0 },
		{ "etcdir", DATA_TYPE_STRING, SOLARD_ETCDIR, sizeof(SOLARD_ETCDIR)-1, 0 },
		{ "libdir", DATA_TYPE_STRING, SOLARD_LIBDIR, sizeof(SOLARD_LIBDIR)-1, 0 },
		{ "logdir", DATA_TYPE_STRING, SOLARD_LOGDIR, sizeof(SOLARD_LOGDIR)-1, 0 },
		{ "debug", DATA_TYPE_INT, &debug, 0, 0, 0,
			"range", 3, (int []){ 0, 99, 1 }, 1, (char *[]) { "debug level" }, 0, 1, 0 },
		{ 0 }
	};
	config_property_t agent_props[] = {
		{ "name", DATA_TYPE_STRING, ap->instance_name, sizeof(ap->instance_name)-1, 0 },
		{ "read_interval", DATA_TYPE_INT, &ap->read_interval, 0, "30", 0,
			"range", 3, (int []){ 0, 300, 1 }, 1, (char *[]) { "Interval" }, "S", 1, 0 },
		{ "open_before_read", DATA_TYPE_BOOL, &ap->open_before_read, 0, "no", 0,
			"range", 3, (int []){ -1, 0, 1 }, 3, (char *[]){ "Not set", "No", "Yes" }, 0, 1, 0 },
		{ "close_after_read", DATA_TYPE_BOOL, &ap->close_after_read, 0, "no", 0,
			"range", 3, (int []){ -1, 0, 1 }, 3, (char *[]){ "Not set", "No", "Yes" }, 0, 1, 0 },
		{ "write_interval", DATA_TYPE_INT, &ap->write_interval, 0, "30", 0,
			"range", 3, (int []){ 0, 300, 1 }, 1, (char *[]) { "Interval" }, "S", 1, 0 },
		{ "open_before_write", DATA_TYPE_BOOL, &ap->open_before_write, 0, "no", 0,
			"range", 3, (int []){ -1, 0, 1 }, 3, (char *[]){ "Not set", "No", "Yes" }, 0, 1, 0 },
		{ "close_after_write", DATA_TYPE_BOOL, &ap->close_after_write, 0, "no", 0,
			"range", 3, (int []){ -1, 0, 1 }, 3, (char *[]){ "Not set", "No", "Yes" }, 0, 1, 0 },
#ifdef JS
		{ "script_dir", DATA_TYPE_STRING, ap->script_dir, sizeof(ap->script_dir)-1, 0 },
		{ "init_script", DATA_TYPE_STRING, ap->init_script, sizeof(ap->init_script)-1, "init.js", 0 },
		{ "start_script", DATA_TYPE_STRING, ap->start_script, sizeof(ap->start_script)-1, "start.js", 0 },
		{ "read_script", DATA_TYPE_STRING, ap->read_script, sizeof(ap->read_script)-1, "read.js", 0 },
		{ "write_script", DATA_TYPE_STRING, ap->write_script, sizeof(ap->write_script)-1, "write.js", 0 },
		{ "run_script", DATA_TYPE_STRING, ap->run_script, sizeof(ap->run_script)-1, "run.js", 0 },
		{ "stop_script", DATA_TYPE_STRING, ap->stop_script, sizeof(ap->stop_script)-1, "stop.js", 0 },
#endif
		{0}
	};
	config_property_t mqtt_props[] = {
		{ "host", DATA_TYPE_STRING, ap->mqtt_config.host, sizeof(ap->mqtt_config.host)-1, "localhost", 0,
                        0, 0, 0, 1, (char *[]){ "MQTT host" }, 0, 1, 0 },
		{ "port", DATA_TYPE_INT, &ap->mqtt_config.port, 0, "1883", 0,
			0, 0, 0, 1, (char *[]){ "MQTT port" }, 0, 1, 0 },
		{ "clientid", DATA_TYPE_STRING, ap->mqtt_config.clientid, sizeof(ap->mqtt_config.clientid)-1, "", 0,
                        0, 0, 0, 1, (char *[]){ "MQTT clientid" }, 0, 1, 0 },
		{ "username", DATA_TYPE_STRING, ap->mqtt_config.username, sizeof(ap->mqtt_config.username)-1, "", 0,
                        0, 0, 0, 1, (char *[]){ "MQTT username" }, 0, 1, 0 },
		{ "password", DATA_TYPE_STRING, ap->mqtt_config.password, sizeof(ap->mqtt_config.password)-1, "", 0,
                        0, 0, 0, 1, (char *[]){ "MQTT password" }, 0, 1, 0 },
		{ 0 }
	};
	config_function_t agent_funcs[] = {
		{ "get_info", cf_agent_getinfo, ap, 0 },
		{ 0 }
	};

	ap->cp = config_init(ap->instance_name, driver_props, driver_funcs);
	if (!ap->cp) return 1;
	dprintf(1,"adding common...\n");
	config_add_props(ap->cp, ap->instance_name, common_props, 0);
	dprintf(1,"adding agent...\n");
	config_add_props(ap->cp, ap->instance_name, agent_props, 0);
	config_add_props(ap->cp, "agent", agent_props, CONFIG_FLAG_NOSAVE | CONFIG_FLAG_NOID);
	config_add_funcs(ap->cp, agent_funcs);
	dprintf(1,"adding mqtt...\n");
	config_add_props(ap->cp, "mqtt", mqtt_props, CONFIG_FLAG_NOID);

	/* Add the mqtt props to the instance config */
	{
		char *names[] = { "mqtt_host", "mqtt_port", "mqtt_clientid", "mqtt_username", "mqtt_password", 0 };
		config_section_t *s;
		config_property_t *p;
		int i;

		s = config_get_section(ap->cp, ap->instance_name);
		if (!s) {
			log_error("%s section does not exist?!?\n", ap->instance_name);
			return 1;
		}
		for(i=0; names[i]; i++) {
			p = config_section_dup_property(s, &mqtt_props[i], names[i]);
			if (!p) continue;
			dprintf(1,"p->name: %s\n",p->name);
			config_section_add_property(ap->cp, s, p, CONFIG_FLAG_NOID);
		}
	}

#ifdef JS
	/* Set script_dir if empty */
	dprintf(1,"script_dir(%d): %s\n", strlen(ap->script_dir), ap->script_dir);
	if (!strlen(ap->script_dir)) {
		sprintf(ap->script_dir,"%s/scripts/%s",SOLARD_LIBDIR,ap->instance_name);
		fixpath(ap->script_dir,sizeof(ap->script_dir));
		dprintf(1,"NEW script_dir(%d): %s\n", strlen(ap->script_dir), ap->script_dir);
	}
#endif

	dprintf(1,"done!\n");
	return 0;
}

int agent_mqtt_init(solard_agent_t *ap) {

	if (ap->mqtt_init) return 0;

	dprintf(1,"host: %s, clientid: %s, user: %s, pass: %s\n",
		ap->mqtt_config.host, ap->mqtt_config.clientid, ap->mqtt_config.username, ap->mqtt_config.password);
	if (!strlen(ap->mqtt_config.host)) {
		log_write(LOG_WARNING,"mqtt host not specified, using localhost\n");
		strcpy(ap->mqtt_config.host,"localhost");
	}

	/* Create a unique ID for MQTT */
	if (!strlen(ap->mqtt_config.clientid)) {
		uint8_t uuid[16];

		dprintf(1,"gen'ing MQTT ClientID...\n");
		uuid_generate_random(uuid);
		my_uuid_unparse(uuid, ap->mqtt_config.clientid);
		dprintf(4,"clientid: %s\n",ap->mqtt_config.clientid);
	}

	/* Create LWT topic */
	agent_mktopic(ap->mqtt_config.lwt_topic,sizeof(ap->mqtt_config.lwt_topic)-1,ap,ap->instance_name,"Status");

	/* Create a new client */
	if (mqtt_newclient(ap->m, &ap->mqtt_config)) goto agent_mqtt_init_error;

	/* Connect to the server */
	if (mqtt_connect(ap->m,20)) goto agent_mqtt_init_error;

	ap->mqtt_init = 0;
	return 0;
agent_mqtt_init_error:
	return 1;
}

/* Do most of the mechanics of an agent startup */
solard_agent_t *agent_init(int argc, char **argv, opt_proctab_t *agent_opts,
		solard_driver_t *Cdriver, void *handle, config_property_t *props, config_function_t *funcs) {
#ifdef JS
	char jsexec[4096];
	int rtsize;
#endif
	solard_agent_t *ap;
	char mqtt_info[200],*p;
	char configfile[256];
	char name[SOLARD_NAME_LEN];
	char sname[64];
	int info_flag,read_interval,write_interval,config_from_mqtt,cformat;
	opt_proctab_t std_opts[] = {
		/* Spec, dest, type len, reqd, default val, have */
		{ "-m::|mqtt host,clientid[,user[,pass]]",&mqtt_info,DATA_TYPE_STRING,sizeof(mqtt_info)-1,0,"" },
		{ "-M|get config from mqtt",&config_from_mqtt,DATA_TYPE_LOGICAL,0,0,"N" },
		{ "-c:%|config file",&configfile,DATA_TYPE_STRING,sizeof(configfile)-1,0,"" },
		{ "-F:%|config file format",&cformat,DATA_TYPE_INT,0,0,"-1" },
		{ "-s::|config section name",&sname,DATA_TYPE_STRING,sizeof(sname)-1,0,"" },
		{ "-n::|agent name",&name,DATA_TYPE_STRING,sizeof(name)-1,0,"" },
#ifdef JS
		{ "-e:%|exectute javascript",&jsexec,DATA_TYPE_STRING,sizeof(jsexec)-1,0,"" },
		{ "-R:#|javascript runtime size",&rtsize,DATA_TYPE_INT,0,0,"67108864" },
#endif
		{ "-I|agent info",&info_flag,DATA_TYPE_LOGICAL,0,0,"0" },
		{ "-r:#|reporting (read) interval",&read_interval,DATA_TYPE_INT,0,0,"-1" },
		{ "-w:#|update (write) interval",&write_interval,DATA_TYPE_INT,0,0,"-1" },
		OPTS_END
	}, *opts;
	int logopts = LOG_INFO|LOG_WARNING|LOG_ERROR|LOG_SYSERR|_ST_DEBUG;
	list names;

	if (!Cdriver) {
		printf("agent_init: agent driver is null, aborting\n");
		exit(0);
	}

	if (agent_opts) opts = opt_addopts(std_opts,agent_opts);
	dprintf(1,"opts: %p\n", opts);
	if (!opts) {
		printf("error initializing options processing\n");
		return 0;
	}
	if (solard_common_init(argc,argv,opts,logopts)) {
		dprintf(1,"common init failed!\n");
		return 0;
	}

	ap = calloc(1,sizeof(*ap));
	if (!ap) {
		log_write(LOG_SYSERR,"calloc agent config");
		return 0;
	}
	ap->driver = Cdriver;
	ap->handle = handle;
	ap->mq = list_create();
	ap->mh = list_create();
#ifdef DEBUG_MEM
	ap->mem_start = mem_used();
#endif

	/* Agent name is driver name */
	strncat(ap->name,Cdriver->name,sizeof(ap->name)-1);

	/* Read the config section for the agent */
	strncpy(ap->section_name,strlen(sname) ? sname : (strlen(name) ? name : Cdriver->name), sizeof(ap->section_name)-1);
	dprintf(1,"section_name: %s\n", ap->section_name);
	dprintf(1,"configfile: %s\n", configfile);

	/* Name specified on command line overrides configfile */
	if (strlen(name)) strcpy(ap->instance_name,name);

	/* If still no instance name, set it to driver name */
	if (!strlen(ap->instance_name)) strncpy(ap->instance_name,Cdriver->name,sizeof(ap->instance_name)-1);

	/* Create MQTT session */
	ap->m = mqtt_new(agent_getmsg, ap);
	if (!ap->m) {
		log_syserror("unable to create MQTT session\n");
		goto agent_init_error;
	}
	dprintf(1,"ap->m: %p\n", ap->m);

	/* Init the config */
	if (agent_config_init(ap,props,funcs)) return 0;

	dprintf(1,"mqtt_info: %s\n", mqtt_info);
	if (strlen(mqtt_info)) {
		strncat(ap->mqtt_config.host,strele(0,",",mqtt_info),sizeof(ap->mqtt_config.host)-1);
		strncat(ap->mqtt_config.clientid,strele(1,",",mqtt_info),sizeof(ap->mqtt_config.clientid)-1);
		strncat(ap->mqtt_config.username,strele(2,",",mqtt_info),sizeof(ap->mqtt_config.username)-1);
		strncat(ap->mqtt_config.password,strele(3,",",mqtt_info),sizeof(ap->mqtt_config.password)-1);
		if (config_from_mqtt) {
			if (agent_mqtt_init(ap)) goto agent_init_error;
			if (agent_config_from_mqtt(ap)) goto agent_init_error;
		}
	}

	if (strlen(configfile)) {
		int fmt;
		if (cformat > CONFIG_FILE_FORMAT_UNKNOWN && cformat < CONFIG_FILE_FORMAT_MAX) {
			fmt = cformat;
		} else {
			char *p;

			/* Try to determine format */
			p = strrchr(configfile,'.');
			if (p && (strcasecmp(p,".json") == 0)) fmt = CONFIG_FILE_FORMAT_JSON;
			else fmt = CONFIG_FILE_FORMAT_INI;
		}
		if (config_read(ap->cp,configfile,fmt)) {
			log_error(config_get_errmsg(ap->cp));
			goto agent_init_error;
		}
	}
	config_dump(ap->cp);

	/* If read/write interval specified on command line, they take precidence */
	if (read_interval >= 0) ap->read_interval = read_interval;
	if (write_interval >= 0) ap->write_interval = write_interval;

	if (!ap->mqtt_init) if (agent_mqtt_init(ap)) goto agent_init_error;

	names = list_create();
	list_add(names,"all",4);
	list_add(names,"All",4);
	list_add(names,Cdriver->name,strlen(Cdriver->name)+1);
	if (strcmp(Cdriver->name,ap->instance_name) != 0) list_add(names,ap->instance_name,strlen(ap->instance_name)+1);
	list_add(names,ap->mqtt_config.clientid,strlen(ap->mqtt_config.clientid)+1);

	/* Sub to all messages for the instance */
	list_reset(names);
	log_info("Subscribing to the following topics:\n");
	while((p = list_get_next(names)) != 0) agent_sub(ap,p,"Ping");
	list_destroy(names);
	agent_sub(ap,ap->mqtt_config.clientid,0);

#ifdef JS
	/* Init js scripting */
	ap->js = JS_EngineInit(rtsize, (js_outputfunc_t *)log_info);
	common_jsinit(ap->js);
	config_jsinit(ap->js, ap->cp);
	agent_jsinit(ap->js, ap);
	mqtt_jsinit(ap->js, ap->m);
	dprintf(1,"handle: %p\n", ap->handle);
#endif
	if (ap->driver->config) ap->driver->config(ap->handle, SOLARD_CONFIG_INIT, ap);
#ifdef JS
	if (strlen(jsexec)) {
		if (JS_EngineExecString(ap->js, jsexec))
			log_warning("error executing js expression\n");
	}
	if (strlen(ap->init_script)) agent_start_script(ap,ap->init_script);
#endif

	/* Get driver info */
	if (Cdriver->config(handle,SOLARD_CONFIG_GET_INFO,&ap->driver_info) == 0) {
		/* If info flag, print info to stdout then exit */
		dprintf(1,"info_flag: %d\n", info_flag);
		if (info_flag) exit(agent_pubinfo(ap, 1));

		/* If model number in info, sub to that too */
		p = json_object_get_string(json_value_get_object(ap->driver_info),"device_model");
		if (p) agent_sub(ap,p,"Ping");

		/* Publish the info */
		agent_pubinfo(ap, 0);
	}

	/* Publish our config */
	if (agent_pubconfig(ap)) goto agent_init_error;

#ifdef DEBUG_MEM
	if ((mem_used()-ap->mem_start) > ap->mem_last) {
		dprintf(1,"agent_init mem usage: %d\n", mem_used()-ap->mem_start);
		ap->mem_last = mem_used() - ap->mem_start;
	}
#endif

	dprintf(1,"returning: %p\n",ap);
	return ap;
agent_init_error:
	free(ap);
	dprintf(1,"returning 0\n");
	return 0;
}

int agent_run(solard_agent_t *ap) {
	solard_message_t *msg;
	int read_status,write_status;
	time_t last_read,last_write,cur,diff;
//	mhinfo_t *mhinfo;

	dprintf(1,"ap: %p\n", ap);

	/* If we have a start script, run it in the background */
	dprintf(1,"start_script: %s\n", ap->start_script);
	if (strlen(ap->start_script)) agent_start_script(ap,ap->start_script);

	last_read = last_write = 0;
	solard_set_state(ap,SOLARD_AGENT_RUNNING);
	while(solard_check_state(ap,SOLARD_AGENT_RUNNING)) {
		dprintf(1,"mem usage: %d\n", mem_used()-ap->mem_start);
#ifdef DEBUG_MEM
#if 0
		if ((mem_used()-ap->mem_start) != ap->mem_last) {
			dprintf(1,"mem usage: %d\n", mem_used()-ap->mem_start);
			ap->mem_last = mem_used() - ap->mem_start;
		}
#endif
#endif
		/* Call read func */
		time(&cur);
		diff = cur - last_read;
		dprintf(1,"diff: %d, read_interval: %d\n", (int)diff, ap->read_interval);
		read_status = 0;
		if (suspend_read == 0 && diff >= ap->read_interval) {
			dprintf(1,"open_before_read: %d, open: %p\n", ap->open_before_read, ap->driver->open);
			if (ap->open_before_read && ap->driver->open) {
				if (ap->driver->open(ap->handle)) {
					log_error("agent_run: open for read failed\n");
					/* make sure we wait the correct amount */
					time(&last_read);
					continue;
				}
			}
			dprintf(1,"read: %p\n", ap->driver->read);
			if (ap->driver->read) read_status = ap->driver->read(ap->handle,0,0);
			dprintf(1,"read_script: %s\n", ap->read_script);
			if (strlen(ap->read_script) && access(ap->read_script,0) == 0)
				read_status = agent_start_script(ap,ap->read_script);
			dprintf(1,"close_after_read: %d, close: %p\n", ap->close_after_read, ap->driver->close);
			if (ap->close_after_read && ap->driver->close) ap->driver->close(ap->handle);
			time(&last_read);
#ifdef DEBUG_MEM
			dprintf(5,"used: %ld\n", mem_used() - ap->mem_start);
#endif
		}

		/* Process messages */
		dprintf(1,"mq count: %d\n", list_count(ap->mq));
		list_reset(ap->mq);
		while((msg = list_get_next(ap->mq)) != 0) {
			solard_message_dump(msg,1);
			agent_process(ap,msg);
			list_delete(ap->mq,msg);
		}

		/* Skip rest if failed to read */
		dprintf(1,"read_status: %d\n", read_status);
		if (read_status != 0) {
			sleep(1);
			continue;
		}

		/* Call cb */
		dprintf(1,"func: %p\n", ap->callback.func);
		if (ap->callback.func) ap->callback.func(ap->callback.ctx);
		if (strlen(ap->run_script)) agent_start_script(ap,ap->run_script);
#ifdef DEBUG_MEM
		dprintf(5,"used: %ld\n", mem_used() - ap->mem_start);
#endif

		/* Call write func */
		time(&cur);
		diff = cur - last_write;
		dprintf(1,"diff: %d, write_interval: %d\n", (int)diff, ap->write_interval);
		if (read_status == 0 && suspend_write == 0 && diff >= ap->write_interval) {
			dprintf(5,"writing...\n");
			if (ap->open_before_write && ap->driver->open) {
				if (ap->driver->open(ap->handle)) {
					time(&last_write);
					log_error("agent_run: open for write failed\n");
					continue;
				}
			}
			write_status = 0;
			dprintf(1,"write: %p\n", ap->driver->write);
			if (ap->driver->write) write_status = ap->driver->write(ap->handle,0,0);
			dprintf(1,"write_status: %d\n", write_status);
			dprintf(1,"write_script: %s\n", ap->write_script);
			if (strlen(ap->write_script) && access(ap->write_script,0) == 0)
				write_status = agent_start_script(ap,ap->write_script);
			dprintf(1,"write_status: %d\n", write_status);
			dprintf(1,"close_after_write: %d, close: %p\n", ap->close_after_write, ap->driver->close);
			if (ap->close_after_write && ap->driver->close) ap->driver->close(ap->handle);
			time(&last_write);
#ifdef DEBUG_MEM
			dprintf(5,"used: %ld\n", mem_used() - ap->mem_start);
#endif
			if (write_status) log_error("write failed!\n");
		}
		sleep(1);
	}
	if (strlen(ap->stop_script)) agent_start_script(ap,ap->stop_script);
	mqtt_disconnect(ap->m,10);
	mqtt_destroy(ap->m);
	return 0;
}

#ifdef JS
static JSBool agent_getprop(JSContext *cx, JSObject *obj, jsval id, jsval *rval) {
	solard_agent_t *ap;

	ap = JS_GetPrivate(cx,obj);
	if (!ap) return JS_FALSE;
	return config_jsgetprop(cx, obj, id, rval, ap->cp, ap->props);
}

static JSBool agent_setprop(JSContext *cx, JSObject *obj, jsval id, jsval *vp) {
	solard_agent_t *ap;

	ap = JS_GetPrivate(cx,obj);
	if (!ap) return JS_FALSE;
	return config_jssetprop(cx, obj, id, vp, ap->cp, ap->props);
}

static JSClass agent_class = {
	"agent",		/* Name */
	JSCLASS_HAS_PRIVATE,	/* Flags */
	JS_PropertyStub,	/* addProperty */
	JS_PropertyStub,	/* delProperty */
	agent_getprop,		/* getProperty */
	agent_setprop,		/* setProperty */
	JS_EnumerateStub,	/* enumerate */
	JS_ResolveStub,		/* resolve */
	JS_ConvertStub,		/* convert */
	JS_FinalizeStub,	/* finalize */
	JSCLASS_NO_OPTIONAL_MEMBERS
};

JSObject *JSAgent(JSContext *cx, void *priv) {
	JSFunctionSpec agent_funcs[] = {
		{ 0 }
	};
	JSObject *obj;
	solard_agent_t *ap = priv;
	JSPropertySpec *props;

	props = 0;
	if (ap && !ap->props) {
		ap->props = config_to_props(ap->cp, ap->instance_name, 0);
		if (!ap->props) {
			log_error("unable to create props: %s\n", config_get_errmsg(ap->cp));
			return 0;
		}
		props = ap->props;
	}

	dprintf(1,"=====> defining %s object\n",agent_class.name);
	obj = JS_InitClass(cx, JS_GetGlobalObject(cx), 0, &agent_class, 0, 0, props, agent_funcs, 0, 0);
	if (!obj) {
		JS_ReportError(cx,"unable to initialize %s class", agent_class.name);
		return 0;
	}
	JS_SetPrivate(cx,obj,priv);
	dprintf(5,"done!\n");
	return obj;
}

int agent_jsinit(JSEngine *e, void *priv) {
	return JS_EngineAddInitFunc(e, "agent", JSAgent, priv);
}

#endif

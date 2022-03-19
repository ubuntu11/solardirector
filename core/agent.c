
/*
Copyright (c) 2021, Stephen P. Shoecraft
All rights reserved.

This source code is licensed under the BSD-style license found in the
LICENSE file in the root directory of this source tree.
*/

#define DEBUG_AGENT 1
#define DEBUG_STARTUP 0
#define dlevel 2

#ifdef DEBUG
#undef DEBUG
#endif
#define DEBUG DEBUG_AGENT
#include "debug.h"

#include "agent.h"
#include "jsobj.h"
#include "jsstr.h"

#if DEBUG_STARTUP
#define _ST_DEBUG LOG_DEBUG
#else
#define _ST_DEBUG 0
#endif

int agent_set_callback(solard_agent_t *ap, solard_agent_callback_t cb, void *ctx) {
	ap->callback.func = cb;
	ap->callback.ctx = ctx;
	return 0;
}

int agent_clear_callback(solard_agent_t *ap) {
	ap->callback.func = 0;
	ap->callback.ctx = 0;
	return 0;
}

void agent_mktopic(char *topic, int topicsz, solard_agent_t *ap, char *name, char *func) {
	register char *p;

	dprintf(dlevel,"name: %s, func: %s\n", name, func);

	p = topic;
	p += snprintf(p,topicsz-strlen(topic),"%s/%s",SOLARD_TOPIC_ROOT,SOLARD_TOPIC_AGENTS);
	if (name) p += snprintf(p,topicsz-strlen(topic),"/%s",name);
	if (func) p += snprintf(p,topicsz-strlen(topic),"/%s",func);
}

int agent_sub(solard_agent_t *ap, char *name, char *func) {
	char topic[SOLARD_TOPIC_LEN];

	dprintf(dlevel,"name: %s, func: %s\n", name, func);

	agent_mktopic(topic,sizeof(topic)-1,ap,name,func);
        dprintf(dlevel,"topic: %s\n", topic);
	return mqtt_sub(ap->m,topic);
}

int agent_pub(solard_agent_t *ap, char *func, char *message, int retain) {
	char topic[SOLARD_TOPIC_LEN];
	char temp[32];

	strncpy(temp,message,31);
	dprintf(dlevel,"func: %s, message: %s, retain: %d\n", func, temp, retain);

	agent_mktopic(topic,sizeof(topic)-1,ap,ap->instance_name,func);
        dprintf(dlevel,"topic: %s\n", topic);
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

int agent_get_info(solard_agent_t *ap, int info_flag) {

	/* If info flag, print info to stdout then exit */
	dprintf(dlevel,"info_flag: %d\n", info_flag);

	if (ap->driver_info) json_destroy_value(ap->driver_info);

	/* Get driver info */
	if (ap->driver->config(ap->handle,SOLARD_CONFIG_GET_INFO,&ap->driver_info)) return 1;

	/* Publish the info */
	agent_pubinfo(ap, info_flag);
	if (info_flag) exit(0);
	return 0;
}

int agent_pubconfig(solard_agent_t *ap) {
	char *data;
	json_value_t *v;
	int r;

	dprintf(dlevel,"publishing config...\n");

	v = config_to_json(ap->cp,CONFIG_FLAG_NOPUB,0);
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

	dprintf(dlevel,"replyto: %s, status: %d, message(%d): %s\n", replyto, status, strlen(message),message);

	o = json_create_object();
	json_object_set_number(o,"status",status);
	json_object_set_string(o,"message",message);
	json_tostring(json_object_get_value(o),payload,sizeof(payload)-1,0);
	json_destroy_object(o);

	snprintf(topic,sizeof(topic)-1,"%s/%s/%s",SOLARD_TOPIC_ROOT,SOLARD_TOPIC_CLIENTS,replyto);
	dprintf(dlevel,"topic: %s, payload: %s\n", topic, payload);
	return mqtt_pub(ap->m,topic,payload,1,0);
}

static int agent_process(solard_agent_t *ap, solard_message_t *msg) {
	int status;

	status = config_process(ap->cp, msg->data);
	dprintf(dlevel,"status: %d\n", status);
	/* pub config 1st then reply so the data is there when msg is read */
	if (!status) {
		dprintf(dlevel,"writing config\n");
		config_write(ap->cp);
		dprintf(dlevel,"publishing config...\n");
		status = agent_pubconfig(ap);
	}
	dprintf(dlevel,"status: %d\n", status);
	status = agent_reply(ap,msg->replyto,status,config_get_errmsg(ap->cp));
	dprintf(dlevel,"status: %d\n", status);
	return status;
}

void agent_getmsg(void *ctx, char *topic, char *message, int msglen, char *replyto) {
	solard_agent_t *ap = ctx;
	solard_message_t newmsg;

	if (solard_getmsg(&newmsg,topic,message,msglen,replyto)) return;
	list_add(ap->mq,&newmsg,sizeof(newmsg));
}

int agent_script_exists(solard_agent_t *ap, char *name) {
	char path[256];
	int r;

	sprintf(path,"%s/%s",ap->script_dir,name);
	dprintf(dlevel,"path: %s\n", path);
	r = (access(path,0) == 0);
	dprintf(dlevel,"returning: %d\n",r );
	return r;
}

int agent_start_script(solard_agent_t *ap, char *name) {
	char script[256];

	dprintf(dlevel,"name: %s, script_dir: %s\n", name, ap->script_dir);
	if (strncmp(name,"./",2) != 0 && strlen(ap->script_dir))
		sprintf(script,"%s/%s",ap->script_dir,name);
	else
		strcpy(script,name);
	dprintf(dlevel,"script: %s\n", script);
//	if (agent_script_exists(ap,script)) strcpy(script,name);
	return JS_EngineExec(ap->js, script, 0);
}

int cf_agent_stop(void *ctx, list args, char *errmsg) {
	solard_clear_state((solard_agent_t *)ctx,SOLARD_AGENT_RUNNING);
	return 0;
}

int cf_agent_ping(void *ctx, list args, char *errmsg) {
	sprintf(errmsg,"pong");
	return 1;
}

int cf_agent_getinfo(void *ctx, list args, char *errmsg) {
	sprintf(errmsg,"publish error");
	return agent_get_info((solard_agent_t *)ctx,0);
}

int cf_agent_getconfig(void *ctx, list args, char *errmsg) {
	sprintf(errmsg,"publish error");
	return agent_pubconfig((solard_agent_t *)ctx);
}

int cf_agent_log_open(void *ctx, list args, char *errmsg) {
	solard_agent_t *ap = ctx;
	char *filename;
	extern int logopts;

	dprintf(dlevel,"count: %d\n", list_count(args));
	list_reset(args);
	filename = list_get_next(args);
	dprintf(dlevel,"filename: %s\n", filename);
	if (strcmp(filename,"stdout") == 0) filename = 0;
	log_open(ap->instance_name,filename,logopts);
	return 0;
}

int cf_agent_exec(void *ctx, list args, char *errmsg) {
	solard_agent_t *ap = ctx;
	char *string;
	extern int logopts;

	dprintf(dlevel,"count: %d\n", list_count(args));
	list_reset(args);
	string = list_get_next(args);
	dprintf(dlevel,"string: %s\n", string);
	if (JS_EngineExecString(ap->js, string)) {
		sprintf(errmsg,"unable to execute");
		return 1;
	}
	return 0;
}

static int agent_startup(solard_agent_t *ap, char *configfile, char *mqtt_info, char *influx_info,
		config_property_t *driver_props, config_function_t *driver_funcs) {
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
		{ "script_dir", DATA_TYPE_STRING, ap->script_dir, sizeof(ap->script_dir)-1, 0 },
		{ "init_script", DATA_TYPE_STRING, ap->init_script, sizeof(ap->init_script)-1, "init.js", 0 },
		{ "start_script", DATA_TYPE_STRING, ap->start_script, sizeof(ap->start_script)-1, "start.js", 0 },
		{ "read_script", DATA_TYPE_STRING, ap->read_script, sizeof(ap->read_script)-1, "read.js", 0 },
		{ "write_script", DATA_TYPE_STRING, ap->write_script, sizeof(ap->write_script)-1, "write.js", 0 },
		{ "run_script", DATA_TYPE_STRING, ap->run_script, sizeof(ap->run_script)-1, "run.js", 0 },
		{ "stop_script", DATA_TYPE_STRING, ap->stop_script, sizeof(ap->stop_script)-1, "stop.js", 0 },
		{0}
	};
	config_property_t *props;
	config_function_t *funcs;
	config_function_t agent_funcs[] = {
		{ "ping", cf_agent_ping, ap, 0 },
		{ "stop", cf_agent_stop, ap, 0 },
		{ "get_info", cf_agent_getinfo, ap, 0 },
		{ "get_config", cf_agent_getconfig, ap, 0 },
		{ "log_open", cf_agent_log_open, ap, 1 },
		{ "exec", cf_agent_exec, ap, 1 },
		{ 0 }
	};
//	solard_startup_info_t info;

	props = driver_props ? config_combine_props(driver_props,agent_props) : agent_props;
	funcs = driver_funcs ? config_combine_funcs(driver_funcs,agent_funcs) : agent_funcs;

	/* Create LWT topic */
	agent_mktopic(ap->mqtt_config.lwt_topic,sizeof(ap->mqtt_config.lwt_topic)-1,ap,ap->instance_name,"Status");

        /* Call common startup */
#if 0
	info.ctx = ap;
	info.config_handle = &ap->cp;
	info.config_section = ap->section_name;
	info.config_file = configfile;
	info.config_props = props;
	info.config_funcs = funcs;
	info.mqtt_handle = &ap->m;
	info.mqtt_info = mqtt_info;
	info.mqtt_config = &ap->mqtt_config;
	info.mqtt_callback = agent_getmsg;
	info.influx_handle = &ap->i;
	info.influx_info = influx_info;
	info.influx_config = &ap->influx_config;
	info.js_handle = &ap->js;
	info.js_rtsize = ap->rtsize;
	info.js_output = (js_outputfunc_t *) log_info;

	if (solard_common_startup(&info)) return 1;
return 1;
#endif
	if (solard_common_startup(&ap->cp, ap->section_name, configfile, props, funcs, &ap->m, agent_getmsg, ap, mqtt_info, &ap->mqtt_config, ap->config_from_mqtt, &ap->i, influx_info, &ap->influx_config, &ap->js, ap->rtsize, ap->stksize, (js_outputfunc_t *)log_info)) return 1;

	/* Set script_dir if empty */
	dprintf(dlevel,"script_dir(%d): %s\n", strlen(ap->script_dir), ap->script_dir);
	if (!strlen(ap->script_dir)) {
		sprintf(ap->script_dir,"%s/agents/%s",SOLARD_LIBDIR,ap->driver->name);
		fixpath(ap->script_dir,sizeof(ap->script_dir));
		dprintf(dlevel,"NEW script_dir(%d): %s\n", strlen(ap->script_dir), ap->script_dir);
	}

	config_add_props(ap->cp, "agent", agent_props, CONFIG_FLAG_NOPUB | CONFIG_FLAG_NOSAVE | CONFIG_FLAG_NOID);
	agent_jsinit(ap->js, ap);
	return 0;
}

/* Do most of the mechanics of an agent startup */
solard_agent_t *agent_init(int argc, char **argv, char *agent_version, opt_proctab_t *agent_opts,
		solard_driver_t *Cdriver, void *handle, config_property_t *driver_props, config_function_t *driver_funcs) {
	char jsexec[4096];
	char script[256];
	int rtsize,stksize;
	solard_agent_t *ap;
	char mqtt_info[200];
	char influx_info[200];
	char mqtt_topic[SOLARD_TOPIC_SIZE];
	char configfile[256];
	char name[SOLARD_NAME_LEN];
	char sname[64];
	int info_flag,read_interval,write_interval,config_from_mqtt,cformat;
	opt_proctab_t std_opts[] = {
		/* Spec, dest, type len, reqd, default val, have */
		{ "-m::|mqtt host,clientid[,user[,pass]]",&mqtt_info,DATA_TYPE_STRING,sizeof(mqtt_info)-1,0,"" },
		{ "-M|get config from mqtt",&config_from_mqtt,DATA_TYPE_LOGICAL,0,0,"N" },
		{ "-T|override mqtt topic",&mqtt_topic,DATA_TYPE_STRING,sizeof(mqtt_topic)-1,0,"" },
		{ "-c:%|config file",&configfile,DATA_TYPE_STRING,sizeof(configfile)-1,0,"" },
		{ "-F:%|config file format",&cformat,DATA_TYPE_INT,0,0,"-1" },
		{ "-s::|config section name",&sname,DATA_TYPE_STRING,sizeof(sname)-1,0,"" },
		{ "-n::|agent name",&name,DATA_TYPE_STRING,sizeof(name)-1,0,"" },
		{ "-i::|influx host[,user[,pass]]",&influx_info,DATA_TYPE_STRING,sizeof(influx_info)-1,0,"" },
		{ "-e:%|exectute javascript",&jsexec,DATA_TYPE_STRING,sizeof(jsexec)-1,0,"" },
//		{ "-R:#|javascript runtime size",&rtsize,DATA_TYPE_INT,0,0,"67108864" },
		{ "-R:#|javascript runtime size",&rtsize,DATA_TYPE_INT,0,0,"131072" },
		{ "-S:#|javascript stack size",&stksize,DATA_TYPE_INT,0,0,"8192" },
		{ "-X::|execute JS script and exit",&script,DATA_TYPE_STRING,sizeof(script)-1,0,"" },
		{ "-I|display agent info and exit",&info_flag,DATA_TYPE_LOGICAL,0,0,"0" },
		{ "-r:#|reporting (read) interval",&read_interval,DATA_TYPE_INT,0,0,"-1" },
		{ "-w:#|update (write) interval",&write_interval,DATA_TYPE_INT,0,0,"-1" },
		OPTS_END
	}, *opts;
	int logopts = LOG_INFO|LOG_WARNING|LOG_ERROR|LOG_SYSERR|_ST_DEBUG;

	if (!Cdriver) {
		printf("agent_init: agent driver is null, aborting\n");
		return 0;
	}

	if (agent_opts) opts = opt_addopts(std_opts,agent_opts);
	dprintf(dlevel,"opts: %p\n", opts);
	if (!opts) {
		printf("error initializing options processing\n");
		return 0;
	}
	*jsexec = *mqtt_info = *configfile = *name = *sname = 0;
	if (solard_common_init(argc,argv,agent_version,opts,logopts)) {
		dprintf(dlevel,"common init failed!\n");
		return 0;
	}

	ap = malloc(sizeof(*ap));
	if (!ap) {
		log_write(LOG_SYSERR,"calloc agent config");
		return 0;
	}
	memset(ap,0,sizeof(*ap));
	ap->driver = Cdriver;
	ap->handle = handle;
	ap->mq = list_create();
#ifdef DEBUG_MEM
	ap->mem_start = mem_used();
#endif
	ap->config_from_mqtt = config_from_mqtt;
	ap->rtsize = rtsize;
	ap->stksize = stksize;

	/* Agent name is driver name */
	strncpy(ap->name,Cdriver->name,sizeof(ap->name)-1);

	if (!config_from_mqtt && !strlen(configfile)) {
		char *names[4];
		char *types[] = { "conf", "json", 0 };
		char path[256];
		int c,i,j,f;

		f = c = 0;
		if (strlen(name)) names[c++] = name;
		if (strlen(sname)) names[c++] = sname;
		names[c++] = Cdriver->name;
		for(i=0; i < c; i++) {
			for(j=0; types[j]; j++) {
				sprintf(path,"%s/%s.%s",SOLARD_ETCDIR,names[i],types[j]);
				dprintf(dlevel,"trying: %s\n", path);
				if (access(path,0) == 0) {
					f = 1;
					break;
				}
			}
			if (f) break;
		}
		if (f) strcpy(configfile,path);
	}

	/* Read the config section for the agent */
	strncpy(ap->section_name,strlen(sname) ? sname : (strlen(name) ? name : Cdriver->name), sizeof(ap->section_name)-1);
	dprintf(dlevel,"section_name: %s\n", ap->section_name);
	dprintf(dlevel,"configfile: %s\n", configfile);

	/* Name specified on command line overrides configfile */
	if (strlen(name)) strcpy(ap->instance_name,name);

	/* If still no instance name, set it to driver name */
	if (!strlen(ap->instance_name)) strncpy(ap->instance_name,Cdriver->name,sizeof(ap->instance_name)-1);

	/* call common startup */
	if (agent_startup(ap, configfile, mqtt_info, influx_info, driver_props, driver_funcs)) goto agent_init_error;

	/* Call the agent's config init func */
	if (ap->driver->config && ap->driver->config(ap->handle, SOLARD_CONFIG_INIT, ap)) goto agent_init_error;

	/* XXX req'd for sdconfig */
	agent_sub(ap, ap->instance_name, 0);

	/* Execute any command-line javascript code */
	dprintf(dlevel,"jsexec(%d): %s\n", strlen(jsexec), jsexec);
	if (strlen(jsexec)) {
		if (JS_EngineExecString(ap->js, jsexec))
			log_warning("error executing js expression\n");
	}

	/* Start the init script */
	if (agent_script_exists(ap,ap->init_script) && agent_start_script(ap,ap->init_script)) {
		log_error("init_script failed\n");
		goto agent_init_error;
	}

	/* if specified, Execute javascript file then exit */
	dprintf(dlevel,"script: %s\n", script);
	if (strlen(script)) {
		agent_start_script(ap,script);
		goto agent_init_error;
	}

	/*  Get and publish driver info */
	if (agent_get_info(ap,info_flag)) goto agent_init_error;

	/* Publish our config */
	if (strcmp(Cdriver->name,"sdjs") != 0 && agent_pubconfig(ap)) goto agent_init_error;

#ifdef DEBUG_MEM
	if ((mem_used()-ap->mem_start) > ap->mem_last) {
		dprintf(dlevel,"agent_init mem usage: %d\n", mem_used()-ap->mem_start);
		ap->mem_last = mem_used() - ap->mem_start;
	}
#endif

	dprintf(dlevel,"returning: %p\n",ap);
	return ap;

agent_init_error:
	free(ap);
	dprintf(dlevel,"returning 0\n");
	return 0;
}

int agent_run(solard_agent_t *ap) {
	solard_message_t *msg;
	int read_status,write_status;
	time_t last_read,last_write,cur,diff;
//	mhinfo_t *mhinfo;

	dprintf(dlevel,"ap: %p\n", ap);

	/* If we have a start script, run it in the background */
	dprintf(dlevel,"start_script: %s\n", ap->start_script);
	if (strlen(ap->start_script)) agent_start_script(ap,ap->start_script);

	last_read = last_write = 0;
	solard_set_state(ap,SOLARD_AGENT_RUNNING);
	while(solard_check_state(ap,SOLARD_AGENT_RUNNING)) {
#ifdef DEBUG_MEM
		if ((mem_used()-ap->mem_start) != ap->mem_last) {
			dprintf(dlevel,"mem usage: %d\n", mem_used()-ap->mem_start);
			ap->mem_last = mem_used() - ap->mem_start;
		}
#endif
		/* Call read func */
		time(&cur);
		diff = cur - last_read;
		dprintf(dlevel,"diff: %d, read_interval: %d\n", (int)diff, ap->read_interval);
		read_status = 0;
		if (diff >= ap->read_interval) {
			dprintf(dlevel,"open_before_read: %d, open: %p\n", ap->open_before_read, ap->driver->open);
			if (ap->open_before_read && ap->driver->open) {
				if (ap->driver->open(ap->handle)) {
					log_error("agent_run: open for read failed\n");
					/* make sure we wait the correct amount */
					time(&last_read);
					continue;
				}
			}
			dprintf(dlevel,"read: %p\n", ap->driver->read);
			if (ap->driver->read) read_status = ap->driver->read(ap->handle,0,0,0);
			dprintf(dlevel,"read_script: %s\n", ap->read_script);
			if (agent_script_exists(ap,ap->read_script)) read_status = agent_start_script(ap,ap->read_script);
			dprintf(dlevel,"close_after_read: %d, close: %p\n", ap->close_after_read, ap->driver->close);
			if (ap->close_after_read && ap->driver->close) ap->driver->close(ap->handle);
			time(&last_read);
#ifdef DEBUG_MEM
			dprintf(dlevel,"used: %ld\n", mem_used() - ap->mem_start);
#endif
		}

		/* Process messages */
		dprintf(dlevel,"mq count: %d\n", list_count(ap->mq));
		list_reset(ap->mq);
		while((msg = list_get_next(ap->mq)) != 0) {
			agent_process(ap,msg);
			list_delete(ap->mq,msg);
		}

		/* Skip rest if failed to read */
		dprintf(dlevel,"read_status: %d\n", read_status);
		if (read_status != 0) {
			sleep(1);
			continue;
		}

		/* Call cb */
		dprintf(dlevel,"func: %p\n", ap->callback.func);
		if (ap->callback.func) ap->callback.func(ap->callback.ctx);

		/* Call run script */
		if (agent_script_exists(ap,ap->run_script)) agent_start_script(ap,ap->run_script);

#ifdef DEBUG_MEM
		dprintf(dlevel,"used: %ld\n", mem_used() - ap->mem_start);
#endif

		/* Call write func */
		time(&cur);
		diff = cur - last_write;
		dprintf(dlevel,"diff: %d, write_interval: %d\n", (int)diff, ap->write_interval);
		if (read_status == 0 && diff >= ap->write_interval) {
			dprintf(dlevel,"writing...\n");
			if (ap->open_before_write && ap->driver->open) {
				if (ap->driver->open(ap->handle)) {
					time(&last_write);
					log_error("agent_run: open for write failed\n");
					continue;
				}
			}
			write_status = 0;
			dprintf(dlevel,"write: %p\n", ap->driver->write);
			if (ap->driver->write) write_status = ap->driver->write(ap->handle,0,0,0);
			dprintf(dlevel,"write_status: %d\n", write_status);
			dprintf(dlevel,"write_script: %s\n", ap->write_script);
			if (agent_script_exists(ap,ap->write_script)) write_status = agent_start_script(ap,ap->write_script);
			dprintf(dlevel,"write_status: %d\n", write_status);
			dprintf(dlevel,"close_after_write: %d, close: %p\n", ap->close_after_write, ap->driver->close);
			if (ap->close_after_write && ap->driver->close) ap->driver->close(ap->handle);
			time(&last_write);
#ifdef DEBUG_MEM
			dprintf(dlevel,"used: %ld\n", mem_used() - ap->mem_start);
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

#define AGENT_PROPERTY_ID_CONFIG 1024
#define AGENT_PROPERTY_ID_MQTT 1025
#define AGENT_PROPERTY_ID_INFLUX 1026

static JSBool agent_getprop(JSContext *cx, JSObject *obj, jsval id, jsval *rval) {
	int prop_id;
	solard_agent_t *ap;
	config_property_t *p;


	ap = JS_GetPrivate(cx,obj);
	if (!ap) return JS_FALSE;

	dprintf(dlevel,"id type: %s\n", jstypestr(cx,id));
	p = 0;
	if(JSVAL_IS_INT(id)) {
		prop_id = JSVAL_TO_INT(id);
		dprintf(dlevel,"prop_id: %d\n", prop_id);
		switch(prop_id) {
		case AGENT_PROPERTY_ID_CONFIG:
			*rval = OBJECT_TO_JSVAL(JSConfig(cx,ap->cp));
			break;
		case AGENT_PROPERTY_ID_MQTT:
			*rval = OBJECT_TO_JSVAL(JSMQTT(cx,ap->m));
			break;
		case AGENT_PROPERTY_ID_INFLUX:
			*rval = OBJECT_TO_JSVAL(JSInflux(cx,ap->i));
			break;
		default:
			p = CONFIG_GETMAP(ap->cp,prop_id);
			if (!p) p = config_get_propbyid(ap->cp,prop_id);
			if (!p) {
				JS_ReportError(cx, "internal error: property %d not found", prop_id);
				return JS_FALSE;
			}
			break;
		}
	} else if (JSVAL_IS_STRING(id)) {
		char *sname, *name;
		JSClass *classp = OBJ_GET_CLASS(cx, obj);

		sname = (char *)classp->name;
		name = (char *)js_GetStringBytes(cx, JSVAL_TO_STRING(id));
		dprintf(dlevel,"sname: %s, name: %s\n", sname, name);
		if (sname && name) p = config_get_property(ap->cp, sname, name);
	}
	dprintf(dlevel,"p: %p\n", p);
	if (p && p->dest) {
		dprintf(dlevel,"p: type: %d(%s), name: %s\n", p->type, typestr(p->type), p->name);
		*rval = type_to_jsval(cx,p->type,p->dest,p->len);
	}
	return JS_TRUE;
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

static JSBool agent_jsmktopic(JSContext *cx, uintN argc, jsval *vp) {
	solard_agent_t *ap;
	char topic[SOLARD_TOPIC_SIZE], *func;
	JSObject *obj;

	obj = JS_THIS_OBJECT(cx, vp);
	if (!obj) return JS_FALSE;
	ap = JS_GetPrivate(cx, obj);

	if (argc != 1) {
		JS_ReportError(cx,"mktopic requires 1 arguments (func:string)\n");
		return JS_FALSE;
	}

        func = 0;
        if (!JS_ConvertArguments(cx, argc, JS_ARGV(cx,vp), "s", &func)) return JS_FALSE;
	dprintf(dlevel,"func: %s\n", func);

	agent_mktopic(topic,sizeof(topic)-1,ap,ap->instance_name,func);
	*vp = STRING_TO_JSVAL(JS_NewStringCopyZ(cx,topic));
	return JS_TRUE;
}

#if 0
static JSBool agent_ctor(JSContext *cx, JSObject *obj, uintN argc, jsval *argv, jsval *rval) {
	agent_private_t *p;
	JSObject *newobj;
	int r;

	p = calloc(sizeof(*p),1);
	if (!p) {
		JS_ReportError(cx,"error allocating memory");
		return JS_FALSE;
	}
	p->connected = &p->internal_connected;
	if (argc == 3) {
		if (!JS_ConvertArguments(cx, argc, argv, "s s s", &p->transport, &p->target, &p->topts))
			return JS_FALSE;
	} else if (argc > 1) {
		JS_ReportError(cx, "CAN requires 3 arguments (transport, target, topts)");
		return JS_FALSE;
	}

	dprintf(dlevel,"transport: %s, target: %s, topts:%s\n", p->transport, p->target, p->topts);

	r = do_connect(cx,p);

	newobj = js_InitCANClass(cx,JS_GetGlobalObject(cx));
	JS_SetPrivate(cx,newobj,p);
	*rval = OBJECT_TO_JSVAL(newobj);

	return r;
}
#endif

JSObject *jsagent_new(JSContext *cx, void *tp, void *handle, char *transport, char *target, char *topts, int *con) {
#if 0
	solard_agent_t *p;
	JSObject *newobj;

	p = calloc(sizeof(*p),1);
	if (!p) {
		JS_ReportError(cx,"jsagent_new: error allocating memory");
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

JSObject *JSAgent(JSContext *cx, void *priv) {
	JSPropertySpec agent_props[] = {
		{ "config", AGENT_PROPERTY_ID_CONFIG, JSPROP_ENUMERATE },
		{ "mqtt", AGENT_PROPERTY_ID_MQTT, JSPROP_ENUMERATE },
		{ "influx", AGENT_PROPERTY_ID_INFLUX, JSPROP_ENUMERATE },
		{ 0 }
	};
	JSFunctionSpec agent_funcs[] = {
		JS_FN("mktopic",agent_jsmktopic,1,1,0),
//		JS_FN("run",agent_jsrun,1,1,0),
		{ 0 }
	};
	JSObject *obj;
	solard_agent_t *ap = priv;
	JSPropertySpec *props;

	props = 0;
	if (ap && !ap->props) {
		ap->props = config_to_props(ap->cp, ap->section_name, agent_props);
		if (!ap->props) {
			ap->props = config_to_props(ap->cp, ap->instance_name, agent_props);
			if (!ap->props) {
				ap->props = config_to_props(ap->cp, ap->driver->name, agent_props);
				if (!ap->props) {
					log_error("unable to create props: %s\n", config_get_errmsg(ap->cp));
					return 0;
				}
			}
		}
		props = ap->props;
	}

	dprintf(dlevel,"creating %s object\n",agent_class.name);
	obj = JS_InitClass(cx, JS_GetGlobalObject(cx), 0, &agent_class, 0, 0, props, agent_funcs, 0, 0);
	if (!obj) {
		JS_ReportError(cx,"unable to initialize %s class", agent_class.name);
		return 0;
	}
	JS_SetPrivate(cx,obj,priv);
	dprintf(dlevel,"done!\n");
	return obj;
}

int agent_jsinit(JSEngine *e, void *priv) {
	return JS_EngineAddInitFunc(e, "agent", JSAgent, priv);
}

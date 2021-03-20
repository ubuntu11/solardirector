
#include <time.h>
#include <ctype.h>
#include "agent.h"
#include "opts.h"
#include "init.h"
#include "json.h"

#define DEBUG_STARTUP 0

#if DEBUG_STARTUP
#define _ST_DEBUG LOG_DEBUG
#else
#define _ST_DEBUG 0
#endif

int agent_callback(void *ctx, char *topicName, int topicLen, MQTTClient_message *message);

int do_sub(solard_agent_t *ap, char *st, char *name) {
	char topic[128];

	if (name) {
		sprintf(topic,"%s/%s/%s",ap->topic,name,st);
	} else {
		sprintf(topic,"%s/%s",ap->topic,st);
	}
	dprintf(1,"topic: %s\n", topic);
	return mqtt_sub(ap->m,topic);
}

int agent_callback(void *ctx, char *topicName, int topicLen, MQTTClient_message *message);

solard_agent_t *agent_init(int argc, char **argv, char *agent_name, enum SOLARD_MODTYPE agent_type, opt_proctab_t *agent_opts) {
	solard_agent_t *conf;
	char tp_info[128],mqtt_info[128],*info;
	char configfile[256];
	mqtt_config_t mqtt_config;
	char transport[SOLARD_AGENT_TRANSPORT_LEN+1];
	char target[SOLARD_AGENT_TARGET_LEN+1];
	char topts[64];
	char name[32],*section_name;
	int info_flag,pretty_flag,interval;
	opt_proctab_t std_opts[] = {
		/* Spec, dest, type len, reqd, default val, have */
		{ "-t::|transport:target:opts",&tp_info,DATA_TYPE_STRING,sizeof(tp_info)-1,0,"" },
		{ "-m::|mqtt host:clientid[:user[:pass]]",&mqtt_info,DATA_TYPE_STRING,sizeof(mqtt_info)-1,0,"" },
		{ "-c:%|config file",&configfile,DATA_TYPE_STRING,sizeof(configfile)-1,0,"" },
		{ "-n::|config section name",&name,DATA_TYPE_STRING,sizeof(name)-1,0,"" },
		{ "-I|agent info",&info_flag,DATA_TYPE_LOGICAL,0,0,0 },
		{ "-P|pretty print json output",&pretty_flag,DATA_TYPE_LOGICAL,0,0,0 },
		{ "-i:#|reporting interval",&interval,DATA_TYPE_INT,0,0,"35" },
		OPTS_END
	}, *opts;
	int logopts = LOG_INFO|LOG_WARNING|LOG_ERROR|LOG_SYSERR|_ST_DEBUG;

	opts = opt_addopts(std_opts,agent_opts);
	if (!opts) return 0;
	tp_info[0] = mqtt_info[0] = configfile[0] = 0;
	info_flag = pretty_flag = 0;
	if (solard_common_init(argc,argv,opts,logopts)) return 0;

	conf = calloc(1,sizeof(*conf));
	if (!conf) {
		log_write(LOG_SYSERR,"calloc agent config");
		return 0;
	}
	if (agent_name && strlen(agent_name)) strncat(conf->agent_name,agent_name,SOLARD_AGENT_NAME_LEN-1);
	conf->modules = list_create();
	conf->mq = list_create();
//	conf->type = agent_type;
	conf->interval = 30;
//	conf->pretty = pretty_flag;

	switch(agent_type) {
	case SOLARD_MODTYPE_BATTERY:
		memcpy(&conf->type,&battery,sizeof(struct solard_module));
		break;
	case SOLARD_MODTYPE_INVERTER:
		memcpy(&conf->type,&inverter,sizeof(struct solard_module));
		break;
	default:
		/* Mr. Abbey Normal */
		break;
	}
	/* If no agent name, agent wants to handle everything from here */
	if (!agent_name || !strlen(agent_name)) return conf;

#if 0
	/* Setup convienience funcs */
	switch(agent_type) {
	case SOLARD_MODTYPE_BATTERY:
		conf->role = battery;
		conf->info = battery_info;
		conf->update = battery_update;
		conf->config = battery_config;
		conf->control = battery_control;
		conf->process = battery_process;
		info = "Battery";
		break;
	case SOLARD_MODTYPE_INVERTER:
		conf->role = inverter;
#if 0
		conf->info = inverter_info;
		conf->update = inverter_update;
		conf->config = inverter_config;
		conf->control = inverter_control;
		conf->process = inverter_process;
#endif
		info = "Inverter";
		break;
	default:
		info = "Unknown";
		break;
	}
	sprintf(conf->topic,"/SolarD/%s",info);
	dprintf(1,"topic: %s\n", conf->topic);
#endif

	section_name = strlen(name) ? name : agent_name;
	dprintf(1,"section_name: %s\n", section_name);
	dprintf(1,"configfile: %s\n", configfile);
	if (strlen(configfile)) {
		cfg_proctab_t transport_tab[] = {
			{ section_name, "transport", "Transport", DATA_TYPE_STRING,&transport,sizeof(transport), 0 },
			{ section_name, "target", "Transport address/interface/device", DATA_TYPE_STRING,&target,sizeof(target), 0 },
			{ section_name, "opts", "Transport specific options", DATA_TYPE_STRING,&topts,sizeof(topts), 0 },
			CFG_PROCTAB_END
		};
		transport[0] = target[0] = topts[0] = 0;
		conf->cfg = cfg_read(configfile);
		if (!conf->cfg) goto agent_init_error;
		cfg_get_tab(conf->cfg,transport_tab);
		if (debug) cfg_disp_tab(transport_tab,agent_name,0);
		if (conf->init(conf,section_name)) goto agent_init_error;
		memset(&mqtt_config,0,sizeof(mqtt_config));
		if (mqtt_get_config(conf->cfg,&mqtt_config)) goto agent_init_error;
	} else {
		if (!strlen(tp_info) || !strlen(mqtt_info)) {
			log_write(LOG_ERROR,"either configfile or transport and mqtt info must be specified.\n");
			goto agent_init_error;
		}
		strncat(transport,strele(0,":",tp_info),sizeof(transport)-1);
		strncat(target,strele(1,":",tp_info),sizeof(target)-1);
		strncat(topts,strele(2,":",tp_info),sizeof(topts)-1);
		strncat(mqtt_config.host,strele(0,":",mqtt_info),sizeof(mqtt_config.host)-1);
		strncat(mqtt_config.clientid,strele(1,":",mqtt_info),sizeof(mqtt_config.clientid)-1);
		strncat(mqtt_config.user,strele(2,":",mqtt_info),sizeof(mqtt_config.user)-1);
		strncat(mqtt_config.pass,strele(3,":",mqtt_info),sizeof(mqtt_config.pass)-1);
		dprintf(1,"host: %s, clientid: %s, user: %s, pass: %s\n", mqtt_config.host, mqtt_config.clientid, mqtt_config.user, mqtt_config.pass);
	}

	/* Test MQTT settings */
	if (strlen(mqtt_config.host)) {
		if (!strlen(mqtt_config.clientid)) strncat(mqtt_config.clientid,agent_name,MQTT_CLIENTID_LEN-1);
		conf->m = mqtt_new(mqtt_config.host,mqtt_config.clientid);
		if (mqtt_connect(conf->m,20,0,0)) goto agent_init_error;
		mqtt_disconnect(conf->m,10);
	} else {
		log_write(LOG_ERROR,"mqtt host must be specified\n");
		goto agent_init_error;
	}

	if (strlen(transport) && strlen(target)) {
		solard_module_t *tp;
		void *tp_handle;

		dprintf(1,"target: %s\n", target);
		if (!strlen(target)) return 0;

		/* Load the transport driver */
		tp = load_module(conf->modules,transport,SOLARD_MODTYPE_TRANSPORT);
		if (!tp) goto agent_init_error;

		/* Create an instance of the transport driver */
		tp_handle = tp->new(conf,target,topts);

		/* Load the agent driver */
		conf->funcs = load_module(conf->modules,agent_name,agent_type);
		if (!conf->funcs) goto agent_init_error;

		/* Create an instance of the agent driver */
		conf->handle = conf->funcs->new(conf,tp,tp_handle);
		if (!conf->handle) goto agent_init_error;
	} else {
		log_write(LOG_ERROR,"transport and target must be specified\n");
		goto agent_init_error;
	}

	dprintf(1,"conf->funcs: %p\n", conf->funcs);

	/* dump Info if requested */
	if (info_flag) {
		info = conf->info(conf);
		if (!info) goto agent_init_error;
		dprintf(5,"info: %s\n", info);
		printf("%s\n",info);
		free(conf);
		return 0;
	}

	/* Callback - must be done before connect */
	if (mqtt_setcb(conf->m,conf,0,agent_callback,0)) return 0;
	mqtt_connect(conf->m,20,0,0);

	/* Subscribe to all messages for the driver (all devices) */
	dprintf(1,"conf->agent_name: %s\n", conf->agent_name);
	do_sub(conf,"Config/+",conf->agent_name);
	/* Sub to all messages for the specific ID */

	dprintf(1,"returning: %p\n",conf);
	return conf;
agent_init_error:
	free(conf);
	return 0;
}

int agent_callback(void *ctx, char *topicName, int topicLen, MQTTClient_message *message) {
	solard_agent_t *ap = ctx;
	solard_message_t newm;
	int len;

	if (topicLen) {
		len = topicLen > sizeof(newm.topic)-1 ? sizeof(newm.topic)-1 : topicLen;
		memcpy(newm.topic,topicName,len);
		newm.topic[len] = 0;
	} else {
		newm.topic[0] = 0;
		strncat(newm.topic,topicName,sizeof(newm.topic)-1);
	}
	dprintf(1,"topic: %s\n",newm.topic);

	len = message->payloadlen > sizeof(newm.payload)-1 ? sizeof(newm.payload)-1 : message->payloadlen;
	memcpy(newm.payload,message->payload,len);
	newm.payload[len] = 0;
	dprintf(1,"payload: %s\n", newm.payload);

	list_add(ap->mq,&newm,sizeof(newm));

	MQTTClient_freeMessage(&message);
	MQTTClient_free(topicName);
	dprintf(1,"returning...\n");
	return 1;
}

int agent_send_status(solard_agent_t *ap, char *name, char *op, int status, char *message) {
	char topic[128],msg[4096];
	json_object_t *o;

	dprintf(1,"op: %s, status: %d, message: %s\n", op, status, message);

	o = json_create_object();
	json_add_number(o,"status",status);
	json_add_string(o,"message",message);
	json_tostring(o,msg,sizeof(msg)-1,0);
	sprintf(topic,"%s/%s/Config/Status/%s",ap->topic,name,op);
	dprintf(1,"topic: %s\n", topic);
	mqtt_pub(ap->m,topic,msg,0);
	return 0;
}

list agent_config_payload2list(solard_agent_t *ap, char *name, char *op, char *payload) {
	json_value_t *v;
	list lp;
	int i;
	solard_confreq_t newreq;

	dprintf(1,"name: %s, op: %s, payload: %s\n", name, op, payload);

	/* Payload must either be an array of param names for get or param name/value pairs for set */

	lp = list_create();
	if (!lp) {
		agent_send_status(ap,name,op,1,"internal error");
		return 0;
	}

	/* Parse payload */
	v = json_parse(payload);
	if (!v) {
		agent_send_status(ap,name,op,1,"error parsing json string");
		goto agent_config_payload_error;
	}

	/* If Get, must be array of strings */
	if (strcmp(op,"Get")== 0) {
		json_array_t *a;
		json_value_t *av;
		char *p;

		dprintf(1,"type: %s(%d)\n", json_typestr(v->type), v->type);
		if (v->type != JSONArray) {
			agent_send_status(ap,name,op,1,"invalid format");
			goto agent_config_payload_error;
		}
		/* Process items */
		a = v->value.array;
		dprintf(1,"a->count: %d\n", (int)a->count);
		for(i=0; i < a->count; i++) {
			av = a->items[i];
			dprintf(1,"av type: %s(%d)\n", json_typestr(av->type),av->type);
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
			agent_send_status(ap,name,op,1,"invalid format");
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
		agent_send_status(ap,name,op,1,"unsupported action");
		goto agent_config_payload_error;
	}

	dprintf(1,"returning: %p\n", lp);
	return lp;
agent_config_payload_error:
	dprintf(1,"returning: 0\n");
	return 0;
}

int agent_config(solard_agent_t *ap, list lp) {
	return 0;
}

void agent_process(solard_agent_t *ap, solard_message_t *msg) {
	char action[8],name[32],*p,*p2;
	int c;
	list lp;

	dprintf(1,"topic: %s, payload: %s\n", msg->topic, msg->payload);

	/* See if it's a Config request */
	p = msg->topic + (strlen(msg->topic)-1);
	c = 0;
	while(*p && p >= msg->topic) {
//		dprintf(1,"p: %p, msg->topic: %p, len: %d\n", p, msg->topic, (int)strlen(msg->topic));
		if (*p == '/') {
			if (++c == 3)
				break;
		}
		p--;
	}
	/* Something not right */
	if (c != 3) return;
	dprintf(1,"p: %s\n", p);
	p2 = ++p;
	while(*p && *p != '/') p++;
	dprintf(1,"p: %s\n", p);
	*p = 0;
	name[0] = 0;
	strncat(name,p2,sizeof(name)-1);
	dprintf(1,"name: %s\n",name);
	p2 = ++p;
	while(*p && *p != '/') p++;
	dprintf(1,"p: %s\n", p);
	*p = 0;
	if (strcmp(p2,"Config")==0) {
		p2 = ++p;
		while(*p && *p != '/') p++;
		dprintf(1,"p: %s\n", p);
		*p = 0;
		action[0] = 0;
		strncat(action,p2,sizeof(action)-1);
		dprintf(1,"action: %s\n", action);
		lp = agent_config_payload2list(ap,name,action,msg->payload);
		if (!lp) return;
		ap->config(ap,action,lp);
		list_destroy(lp);
	}
#if 0
	c = 0;
	for(p=msg->topic; *p; p++) {
		if (*p == '/') c++;
	}
	dprintf(1,"c: %d\n", c);
	strcpy(action,strele(c,"/",msg->topic));
#endif

#if 0
	p = msg->payload;
	while(!isspace(*p)) p++;
	*p = 0;
	strcpy(name,msg->payload);
	strcpy(name,strele(0,"=",msg->payload));
	trim(name);
	strcpy(value,strele(1,"=",msg->payload));
	trim(value);

	dprintf(1,"action: %s, name: %s, value: %s\n", action, name, value);
#endif
}

int agent_run(solard_agent_t *ap) {
	solard_message_t *msg;
	int r;
	uint32_t mem_start;
	time_t last_read,cur,diff;

	dprintf(1,"ap: %p\n", ap);

//	pthread_mutex_init(&conf->lock, 0);

	last_read = 0;
	mem_start = mem_used();
	while(1) {
		time(&cur);
		diff = cur - last_read;
		dprintf(5,"diff: %d\n", (int)diff);
		if (diff >= ap->interval) {
			dprintf(1,"reading...\n");
			time(&last_read);
			r = ap->read(ap);
			dprintf(1,"mem: %ld\n", mem_used() - mem_start);
		}

		list_reset(ap->mq);
		while((msg = list_get_next(ap->mq)) != 0) {
			dprintf(1,"msg->topic: %s\n", msg->topic);
			agent_process(ap,msg);
			list_delete(ap->mq,msg);
		}
		sleep(1);
	}
	mqtt_disconnect(ap->m,10);
	mqtt_destroy(ap->m);
	return r;
}

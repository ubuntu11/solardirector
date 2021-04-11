
#include <pthread.h>
#include "client.h"
#include "uuid.h"

#define DEBUG_STARTUP 0

#if DEBUG_STARTUP
#define _ST_DEBUG LOG_DEBUG
#else
#define _ST_DEBUG 0
#endif

#define CLIENT_TEMP_SIZE 1048576

static char *find_entry(solard_client_t *cp, char *name) {
	solard_message_t *dp;
	char *p;

	dprintf(7,"name: %s\n", name);

	p = 0;
	list_reset(cp->messages);
	while((dp = list_get_next(cp->messages)) != 0) {
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
		dprintf(5,"count: %d\n", list_count(cp->messages));
		list_reset(cp->messages);
		while((msg = list_get_next(cp->messages)) != 0) {
			solard_message_dump(msg,3);
			if (msg->type != SOLARD_MESSAGE_STATUS) continue;
			if (strcmp(msg->action,action)==0) {
				dprintf(1,"match.\n");
				list_delete(cp->messages,msg);
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

char *client_get_config(solard_client_t *cp, char *target, char *param, int timeout, int direct) {
	char topic[SOLARD_TOPIC_SIZE], *p;
	json_value_t *a;

	dprintf(1,"target: %s, param: %s, timeout: %d, direct: %d\n", target, param, timeout, direct);

	if (!direct && (p = check(cp, param, 0)) != 0) return p;

	a = json_create_array();
	json_array_add_string(a,param);
	json_tostring(a,cp->data,sizeof(cp->data)-1,0);
	json_destroy(a);

	/* Request it */
	sprintf(topic,"%s/%s/%s/%s/%s",SOLARD_TOPIC_ROOT,target,SOLARD_FUNC_CONFIG,SOLARD_ACTION_GET,cp->id);
	dprintf(1,"topic: %s\n",topic);
	mqtt_pub(cp->m,topic,cp->data,0);

	/* Get status message */
	client_get_status(cp, SOLARD_ACTION_GET, param, timeout);

	/* Get value message */
	dprintf(1,"finding...\n");
	p = find_entry(cp, param);

        /* Clear our request and the status reply */
        mqtt_pub(cp->m,topic,0,0);
        sprintf(topic,"%s/%s/%s/%s/Status/%s",SOLARD_TOPIC_ROOT,target,SOLARD_FUNC_CONFIG,SOLARD_ACTION_GET,cp->id);
        mqtt_pub(cp->m,topic,0,0);

	dprintf(1,"returning: %s\n", p);
	return p;
}

list client_get_mconfig(solard_client_t *cp, char *target, int count, char **params, int timeout) {
	char topic[SOLARD_TOPIC_SIZE], *p;
	json_value_t *a;
	int i;
	list results;

	dprintf(1,"target: %s, count: %d, params: %p, timeout: %d\n", target, count, params, timeout);

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
		sprintf(topic,"%s/%s/%s/%s/%s",SOLARD_TOPIC_ROOT,target,SOLARD_FUNC_CONFIG,SOLARD_ACTION_GET,cp->id);
		dprintf(1,"topic: %s\n",topic);
		mqtt_pub(cp->m,topic,cp->data,0);

		/* Get status message */
		client_get_status(cp, SOLARD_ACTION_GET, 0, timeout);

		/* Clear our request and the status reply */
		mqtt_pub(cp->m,topic,0,0);
		sprintf(topic,"%s/%s/%s/%s/Status/%s",SOLARD_TOPIC_ROOT,target,SOLARD_FUNC_CONFIG,SOLARD_ACTION_SET,cp->id);
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

int client_set_config(solard_client_t *cp, char *target, char *param, char *value, int timeout) {
	char topic[SOLARD_TOPIC_SIZE];
	json_value_t *o;

	dprintf(1,"target: %s, param: %s, value: %s\n", target, param, value);

	o = json_create_object();
	json_add_string(o,param,value);
	json_tostring(o,cp->data,sizeof(cp->data)-1,0);
	json_destroy(o);

	/* Request it */
	sprintf(topic,"%s/%s/%s/%s/%s",SOLARD_TOPIC_ROOT,target,SOLARD_FUNC_CONFIG,SOLARD_ACTION_SET,cp->id);
	dprintf(1,"topic: %s\n",topic);
	mqtt_pub(cp->m,topic,cp->data,0);

	/* Get status message */
	client_get_status(cp, SOLARD_ACTION_SET, param, timeout);

	/* Clear our request and the status reply */
	mqtt_pub(cp->m,topic,0,0);
	sprintf(topic,"%s/%s/%s/%s/Status/%s",SOLARD_TOPIC_ROOT,target,SOLARD_FUNC_CONFIG,SOLARD_ACTION_SET,cp->id);
	mqtt_pub(cp->m,topic,0,0);
	return 0;
}

solard_client_t *client_init(int argc,char **argv,opt_proctab_t *client_opts,char *id,char *cf) {
	solard_client_t *s;
	char mqtt_info[200];
	char configfile[256];
	mqtt_config_t mqtt_config;
	char name[32],*section_name;
	opt_proctab_t std_opts[] = {
		/* Spec, dest, type len, reqd, default val, have */
		{ "-m::|mqtt host:clientid[:user[:pass]]",&mqtt_info,DATA_TYPE_STRING,sizeof(mqtt_info)-1,0,"" },
		{ "-c:%|config file",&configfile,DATA_TYPE_STRING,sizeof(configfile)-1,0,"" },
		{ "-n::|config section name",&name,DATA_TYPE_STRING,sizeof(name)-1,0,"" },
		OPTS_END
	}, *opts;
	int logopts = LOG_INFO|LOG_WARNING|LOG_ERROR|LOG_SYSERR|_ST_DEBUG;

	opts = opt_addopts(std_opts,client_opts);
	if (!opts) return 0;
	mqtt_info[0] = configfile[0] = 0;
	if (solard_common_init(argc,argv,opts,logopts)) return 0;

	dprintf(1,"creating session...\n");
	s = calloc(1,sizeof(*s));
	if (!s) {
		perror("calloc");
		return 0;
	}
	pthread_mutex_init(&s->lock, 0);
	s->messages = list_create();

	dprintf(1,"argc: %d, argv: %p, opts: %p, id: %s\n", argc, argv, opts, id);

	memset(&mqtt_config,0,sizeof(mqtt_config));

	section_name = strlen(name) ? name : id;
	dprintf(1,"section_name: %s\n", section_name);
	dprintf(1,"configfile: %s\n", configfile);
	if (strlen(configfile) == 0 && cf != 0) strncat(configfile,cf,sizeof(configfile)-1);
	if (strlen(configfile)) {
		cfg_proctab_t agent_tab[] = {
			{ section_name, "id", "Client ID", DATA_TYPE_STRING,&s->id,sizeof(s->id), 0 },
			{ section_name, "name", "Client name", DATA_TYPE_STRING,&s->name,sizeof(s->name), id },
			CFG_PROCTAB_END
		};
		s->cfg = cfg_read(configfile);
		if (s->cfg) {
			cfg_get_tab(s->cfg,agent_tab);
			if (debug) cfg_disp_tab(agent_tab,"agent",0);
			if (mqtt_get_config(s->cfg,&mqtt_config)) goto client_init_error;
		}
	} else {
		if (!strlen(mqtt_info)) {
			log_write(LOG_ERROR,"either configfile or mqtt info must be specified.\n");
			goto client_init_error;
		}
		strncat(mqtt_config.host,strele(0,":",mqtt_info),sizeof(mqtt_config.host)-1);
		strncat(mqtt_config.clientid,strele(1,":",mqtt_info),sizeof(mqtt_config.clientid)-1);
		strncat(mqtt_config.user,strele(2,":",mqtt_info),sizeof(mqtt_config.user)-1);
		strncat(mqtt_config.pass,strele(3,":",mqtt_info),sizeof(mqtt_config.pass)-1);
		dprintf(1,"host: %s, clientid: %s, user: %s, pass: %s\n", mqtt_config.host, mqtt_config.clientid, mqtt_config.user, mqtt_config.pass);
	}
	if (!strlen(s->id)) strcpy(s->id,id);
	if (!strlen(s->name)) strcpy(s->name,id);

#if 0
	/* if we dont have a UUID, gen one */
	if (!strlen(s->id) && s->cfg) {
		uint8_t uuid[16];

		dprintf(1,"gen'ing UUID...\n");
		uuid_generate_random(uuid);
		my_uuid_unparse(uuid, s->id);
		dprintf(4,"conf->c->id: %s\n",s->id);
 		cfg_set_item(s->cfg,"solard","id",0,s->id);
		cfg_write(s->cfg);
	}
#endif

	/* MQTT Init */
	if (!strlen(mqtt_config.host)) strcpy(mqtt_config.host,"localhost");
	if (!strlen(mqtt_config.clientid)) strncat(mqtt_config.clientid,s->name,sizeof(mqtt_config.clientid)-1);
	s->m = mqtt_new(&mqtt_config,solard_getmsg,s->messages);
	if (!s->m) goto client_init_error;
//	if (mqtt_setcb(s->m,s,client_mqtt_reconnect,client_callback,0)) return 0;
	if (mqtt_connect(s->m,20)) goto client_init_error;

	return s;
client_init_error:
	free(s);
	return 0;
}

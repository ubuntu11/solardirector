
#include <pthread.h>
#include "client.h"

#define DEBUG_STARTUP 0

#if DEBUG_STARTUP
#define _ST_DEBUG LOG_DEBUG
#else
#define _ST_DEBUG 0
#endif

char *find_entry(solard_client_t *cp, char *name) {
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

int get_status(solard_client_t *cp, char *action, char *name, int timeout) {
	solard_message_t *dp;
	int retries, r, found;

	dprintf(1,"action: %s, name: %s\n", action, name);

	r = 1;
	retries=timeout;
	found = 0;
	while(retries--) {
		dprintf(5,"count: %d\n", list_count(cp->messages));
		list_reset(cp->messages);
		while((dp = list_get_next(cp->messages)) != 0) {
			dprintf(1,"dp: type: %d, action: %s, name %s, data: %s\n", dp->type, dp->action, dp->name, dp->data);
			if (dp->type != SOLARD_MESSAGE_STATUS) continue;
			if (strcmp(dp->action,action)==0) {
				dprintf(1,"match.\n");
//				mqtt_pub(cp->m,dp->topic,0,0);
				list_delete(cp->messages,dp);
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

char *check(solard_client_t *s, char *name, int wait) {
	char *p;
	int retries;

	/* See if we have it first */
	retries=1;
	while(retries--) {
		p = find_entry(s,name);
		dprintf(7,"p: %s\n", p);
		if (p) {
			char *p2;
			p2 = malloc(strlen(p)+1);
			strcpy(p2,p);
			return p2;
		}
		if (wait) sleep(1);
	}
	return 0;
}

char *client_get_config(solard_client_t *cp, char *name, int timeout, int direct) {
	char temp[200],data[128], *p;
	json_value_t *a;

	dprintf(1,"name: %s, timeout: %d, direct: %d\n", name, timeout, direct);

	if (!direct && (p = check(cp, name, 0)) != 0) return p;

	a = json_create_array();
	json_array_add_string(a,name);
	json_tostring(a,data,sizeof(data)-1,0);
	json_destroy(a);

	/* Request it */
	sprintf(temp,"%s/Config/Get/%s",cp->topic,cp->id);
	dprintf(1,"temp: %s\n",temp);
	mqtt_pub(cp->m,temp,data,0);

	/* Get status message */
	if (get_status(cp, "Get", name, timeout) != 0) return 0;

	/* Get value message */
	dprintf(1,"finding...\n");
	p = find_entry(cp, name);

	/* Clear our get request */
	mqtt_pub(cp->m,temp,0,0);

	dprintf(1,"returning: %s\n", p);
	return p;
}

list client_get_mconfig(solard_client_t *cp, int count, char **names, int timeout) {
	char topic[200], *p;
	json_value_t *a;
	int i;
	list results;

	dprintf(1,"count: %d, names: %p, timeout: %d\n", count, names, timeout);

	results = list_create();

	a = json_create_array();
	for(i=0; i < count; i++) {
		dprintf(1,"names[%d]: %s\n", i, names[i]);
		if ((p = check(cp, names[i],0)) == 0) json_array_add_string(a,names[i]);
	}
	dprintf(1,"a->count: %d\n",(int)a->value.array->count);
	if (a->value.array->count) {
		json_tostring(a,cp->temp,1048576,0);
		dprintf(1,"temp: %s\n", cp->temp);

		/* Request */
		sprintf(topic,"%s/Config/Get/%s",cp->topic,cp->id);
		dprintf(1,"topic: %s\n",topic);
		mqtt_pub(cp->m,topic,cp->temp,0);

		/* Get status message */
		if (get_status(cp, "Get", 0, timeout) != 0) goto client_get_mconfig_done;

		/* Clear our get request */
		mqtt_pub(cp->m,topic,0,0);
	}

	/* Get value message */
	dprintf(1,"finding...\n");
	for(i=0; i < count; i++) {
		if ((p = find_entry(cp, names[i])) != 0) list_add(results,p,strlen(p)+1);
	}

client_get_mconfig_done:
	json_destroy(a);
	dprintf(1,"returning results\n");
	return results;
}

int client_set_config(solard_client_t *cp, char *name, char *value, int timeout) {
	char temp[200],data[256];
	json_value_t *o;

	dprintf(1,"name: %s, value: %s\n", name, value);

	o = json_create_object();
	json_add_string(o,name,value);
	json_tostring(o,data,sizeof(data)-1,0);
	json_destroy((json_value_t *)o);

	/* Request it */
	sprintf(temp,"%s/Config/%s/%s",cp->topic,(strcmp(name,"Add")==0 ? "Add" : "Set"),cp->id);
	dprintf(1,"temp: %s\n",temp);
	mqtt_pub(cp->m,temp,data,0);

	/* Get status message */
	get_status(cp, "Set", name, timeout);

	/* Clear our get request and the status reply */
	mqtt_pub(cp->m,temp,0,0);
	sprintf(temp,"%s/Status/Set/%s",cp->topic,cp->id);
	mqtt_pub(cp->m,temp,0,0);
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
	strcpy(s->id,id);
	s->temp = malloc(1048576);

	dprintf(1,"argc: %d, argv: %p, opts: %p, id: %s\n", argc, argv, opts, id);

	section_name = strlen(name) ? name : "client";
	dprintf(1,"section_name: %s\n", section_name);
	dprintf(1,"configfile: %s\n", configfile);
	if (strlen(configfile) == 0 && cf != 0) strncat(configfile,cf,sizeof(configfile)-1);
	if (strlen(configfile)) {
		cfg_proctab_t agent_tab[] = {
			{ section_name, "name", "Client name", DATA_TYPE_STRING,&s->name,sizeof(s->name), 0 },
			{ section_name, "uuid", "Client ID", DATA_TYPE_STRING,&s->id,sizeof(s->id), 0 },
			CFG_PROCTAB_END
		};
		s->cfg = cfg_read(configfile);
		if (!s->cfg) {
			log_write(LOG_SYSERROR,"unable to read configfile: %s\n",configfile);
			goto client_init_error;
		}
		cfg_get_tab(s->cfg,agent_tab);
		if (debug) cfg_disp_tab(agent_tab,"agent",0);
		memset(&mqtt_config,0,sizeof(mqtt_config));
		if (mqtt_get_config(s->cfg,&mqtt_config)) goto client_init_error;
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

	/* MQTT Init */
	if (!strlen(mqtt_config.host)) {
		log_write(LOG_ERROR,"mqtt host must be specified\n");
		goto client_init_error;
	}
	if (!strlen(mqtt_config.clientid)) strncat(mqtt_config.clientid,s->name,MQTT_CLIENTID_LEN-1);
	s->m = mqtt_new(&mqtt_config,solard_getmsg,s->messages);
	if (!s->m) goto client_init_error;
//	if (mqtt_setcb(s->m,s,client_mqtt_reconnect,client_callback,0)) return 0;
	if (mqtt_connect(s->m,20)) goto client_init_error;

	return s;
client_init_error:
	free(s);
	return 0;
}

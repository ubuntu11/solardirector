
#include "solard.h"

#define dlevel 1

extern char *sd_version_string;

#define AGENTINFO_PROPS(info) \
	{ "agent_name", DATA_TYPE_STRING, (info)->agent, sizeof((info)->agent)-1, "", 0, 0, 0, 0, 0, 0, 0, 1, 0 }, \
	{ "agent_path", DATA_TYPE_STRING, (info)->path, sizeof((info)->path)-1, "", 0, 0, 0, 0, 0, 0, 0, 1, 0 }, \
	{ "agent_role", DATA_TYPE_STRING, (info)->role, sizeof((info)->role)-1, "", 0, 0, 0, 0, 0, 0, 0, 1, 0 }, \
	{ "name", DATA_TYPE_STRING, (info)->name, sizeof((info)->name)-1, "", 0, 0, 0, 0, 0, 0, 0, 1, 0 }, \
	{ "transport", DATA_TYPE_STRING, (info)->transport, sizeof((info)->transport)-1, "", 0, 0, 0, 0, 0, 0, 0, 1, 0 }, \
	{ "target", DATA_TYPE_STRING, (info)->target, sizeof((info)->target)-1, "", 0, 0, 0, 0, 0, 0, 0, 1, 0 }, \
	{ "topts", DATA_TYPE_STRING, (info)->topts, sizeof((info)->topts)-1, "", 0, 0, 0, 0, 0, 0, 0, 1, 0 }, \
	{ "managed", DATA_TYPE_BOOL, &(info)->managed, 0, "yes", 0, 0, 0, 0, 0, 0, 0, 1, 0 }, \
	{ 0 }

static int set_agent_config(config_t *cp, char *sname, solard_agentinfo_t *info) {
//	solard_agentinfo_t newinfo;
//	config_property_t props[] = { AGENTINFO_PROPS(&newinfo) };
	return 0;
}

int solard_read_config(solard_config_t *conf) {
	char index[16];
	config_section_t *s,*as;
	config_property_t *p;
	int i,count;
	solard_agentinfo_t newinfo;
	config_property_t props[] = { AGENTINFO_PROPS(&newinfo) };

	/* Get the agents section */
	s = config_get_section(conf->ap->cp, "agents");
	if (!s) return 0;

	/* Get the count */
	p = config_section_get_property(s,"count");
	if (!p) return 0;
	conv_type(DATA_TYPE_INT,&count,0,p->type,p->dest,p->len);

	/* For each agent  ... */
	for(i=0; i < count; i++) {
		sprintf(index,"A%02d",i);
		dprintf(1,"index: %s\n", index);
		p = config_section_get_property(s,index);
		if (!p) {
			log_warning("agents entry for %s does not exist",index);
			continue;
		}
		dprintf(1,"dest: %s\n", p->dest);
		as = config_get_section(conf->ap->cp, p->dest);
		if (!as) {
			log_warning("section %s does not exist for agents entry %s",p->dest,index);
			continue;
		}

		memset(&newinfo,0,sizeof(newinfo));
		config_section_get_properties(as, props);
		agentinfo_newid(&newinfo);
		agentinfo_add(conf,&newinfo);
	}
	return 0;
}

int solard_write_config(solard_config_t *conf) { return 1; }

int solard_add_config(solard_config_t *conf, char *label, char *value, char *errmsg) {
	solard_agentinfo_t newinfo,*info;
	char name[64],key[64],*str;
	register char *p;
	list lp;
	int have_managed;

	dprintf(1,"label: %s, value: %s\n", label, value);

	strcpy(errmsg,"unable to add agent");

	strncpy(name,label,sizeof(name)-1);
	info = agent_find(conf,name);
	if (info) {
		strcpy(errmsg,"agent already exists");
		return 1;
	}

	conv_type(DATA_TYPE_STRING_LIST,&lp,0,DATA_TYPE_STRING,value,strlen(value));
	dprintf(1,"lp: %p\n", lp);
	if (!lp) {
		strcpy(errmsg,"invalid format (agent_name:agnet_parm[,agent_parm,...]");
		return 1;
	}
	dprintf(1,"count: %d\n", list_count(lp));
	memset(&newinfo,0,sizeof(newinfo));
	strncpy(newinfo.agent,name,sizeof(newinfo.agent)-1);
	have_managed = 0;
	list_reset(lp);
	while((str = list_get_next(lp)) != 0) {
		p = strchr(str,'=');
		if (!p) continue;
		*p++ = 0;
		strncpy(key,str,sizeof(key)-1);
//		dprintf(1,"key: %s, value: %s\n", key, p);
		if (strcmp(key,"managed")==0) have_managed = 1;
		agentinfo_set(&newinfo,key,p);
	}
	list_destroy(lp);
	if (!strlen(newinfo.name)) strncpy(newinfo.name,name,sizeof(newinfo.name)-1);
	/* Default to managed if not specified */
	if (!have_managed) newinfo.managed = 1;
	agentinfo_newid(&newinfo);
	info = agentinfo_add(conf,&newinfo);
	if (!info) {
		strcpy(errmsg,conf->errmsg);
		return 1;
	}
	set_agent_config(conf->ap->cp, info->id, info);
	config_write(conf->ap->cp);
	return 0;
}

int solard_agent_init(int argc, char **argv, opt_proctab_t *sd_opts, solard_config_t *conf) {
	config_property_t sd_props[] = {
		{ "agent_notify", DATA_TYPE_INT, &conf->agent_notify, 0, "600", 0,
                        "range", 3, (int []){ 0, 1440, 1 }, 1, (char *[]) { "Dead agent notification timer" }, "S", 1, 0 },
		{ "agent_error", DATA_TYPE_INT, &conf->agent_error, 0, "300", 0,
                        "range", 3, (int []){ 0, 1440, 1 }, 1, (char *[]) { "Agent error timer" }, "S", 1, 0 },
		{ "agent_warning", DATA_TYPE_INT, &conf->agent_warning, 0, "300", 0,
                        "range", 3, (int []){ 0, 1440, 1 }, 1, (char *[]) { "Agent warning timer" }, "S", 1, 0 },
		{ "interval", DATA_TYPE_INT, &conf->interval, 0, "15", 0,
                        "range", 3, (int []){ 0, 1440, 1 }, 1, (char *[]) { "Agent check interval" }, "S", 1, 0 },
		{ "notify", DATA_TYPE_STRING, &conf->notify_path, sizeof(conf->notify_path)-1, "", 0,
			0, 0, 0, 0, 0, 0, 1, 0 },
//		{ "agents", DATA_TYPE_STRING_LIST, conf->agents, 0, 0, CONFIG_FLAG_NOID },
//		{ "batteries", DATA_TYPE_STRING_LIST, conf->batteries, 0, 0, CONFIG_FLAG_NOID | CONFIG_FLAG_NOSAVE },
		{0}
	};
	config_function_t sd_funcs[] = {
		{ "add", (config_funccall_t *)solard_add_config, conf, 2 },
		{0}
	};

	conf->ap = agent_init(argc,argv,sd_version_string,sd_opts,&sd_driver,conf,sd_props,sd_funcs);
	dprintf(1,"ap: %p\n",conf->ap);
	return (conf->ap ? 0 : 1);
}

#if 0

static int solard_cb(void *ctx) {
	solard_config_t *conf = ctx;
	list agents = conf->c->agents;
	client_agentinfo_t *ap;
	char *p;

#if 0
	time_t cur,diff;

	/* Check agents */
	time(&cur);
	diff = cur - conf->last_check;
	dprintf(3,"diff: %d, interval: %d\n", (int)diff, conf->interval);
	if (diff >= conf->interval) {
		check_agents(conf);
		solard_monitor(conf);
		agent_start_script(conf->ap,"monitor.js");
		time(&conf->last_check);
	}
	dprintf(1,"mem used: %ld\n", mem_used() - conf->start);
	return 0;
#endif
        /* Update the agents */
	dprintf(0,"count: %d\n", list_count(agents));
        list_reset(agents);
        while((ap = list_get_next(agents)) != 0) {
                p = client_getagentrole(ap);
                if (!p) continue;
		dprintf(0,"role: %s, name: %s, count: %d\n", p, ap->name, list_count(ap->mq));
		if (strcmp(p,SOLARD_ROLE_BATTERY)) getpack(conf,ap);
                else if (strcmp(p,SOLARD_ROLE_BATTERY)) getinv(conf,ap);
                else list_purge(ap->mq);
        }
	return 0;
}
#endif

int solard_config(void *h, int req, ...) {
	solard_config_t *conf = h;
	int r;
	va_list va;

	r = 1;
	dprintf(1,"req: %d\n", req);
	va_start(va,req);
	switch(req) {
	case SOLARD_CONFIG_INIT:
		conf->ap = va_arg(va,solard_agent_t *);
		dprintf(1,"name: %s\n", conf->ap->instance_name);
		dprintf(1,"conf->ap: %p\n", conf->ap);
		r = solard_read_config(conf);

		/* Set callback */
//		agent_set_callback(conf->ap,solard_cb,conf);

#ifdef JS
		if (!r) r = solard_jsinit(conf);
#endif
		break;
	case SOLARD_CONFIG_GET_INFO:
		{
			json_value_t **vp = va_arg(va,json_value_t **);
			dprintf(dlevel,"vp: %p\n", vp);
			if (vp) {
				*vp = sd_get_info(conf);
				r = 0;
			}
		}
		break;
	}
	dprintf(1,"r: %d\n", r);
	return r;
}

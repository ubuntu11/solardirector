
#include "solard.h"

extern solard_driver_t sd_driver;

#define AGENTINFO_PROPS \
	{ "agent_name", DATA_TYPE_STRING, info->agent, sizeof(info->agent)-1, 0, 0, 0, 0, 0, 0, 0, 0, 1, 0 }, \
	{ "agent_path", DATA_TYPE_STRING, info->path, sizeof(info->path)-1, 0, 0, 0, 0, 0, 0, 0, 0, 1, 0 }, \
	{ "agent_role", DATA_TYPE_STRING, info->role, sizeof(info->role)-1, 0, 0, 0, 0, 0, 0, 0, 0, 1, 0 }, \
	{ "name", DATA_TYPE_STRING, info->name, sizeof(info->name)-1, 0, 0, 0, 0, 0, 0, 0, 0, 1, 0 }, \
	{ "transport", DATA_TYPE_STRING, info->transport, sizeof(info->transport)-1, 0, 0, 0, 0, 0, 0, 0, 0, 1, 0 }, \
	{ "target", DATA_TYPE_STRING, info->target, sizeof(info->target)-1, 0, 0, 0, 0, 0, 0, 0, 0, 1, 0 }, \
	{ "tops", DATA_TYPE_STRING, info->topts, sizeof(info->topts)-1, 0, 0, 0, 0, 0, 0, 0, 0, 1, 0 }, \
	{ "managed", DATA_TYPE_BOOL, &info->managed, 0, "1", 0, 0, 0, 0, 0, 0, 0, 1, 0 }

static int get_agent_config(config_t *cp, char *sname, solard_agentinfo_t *info) {
	config_property_t agent_props[] = {
		AGENTINFO_PROPS,
		{ 0 }
	};
	config_property_t *p, *pp;
	config_section_t *s;

	s = config_get_section(cp, sname);
	if (!s) {
		log_error("section not found: %s\n", sname);
		return 1;
	}
	for(p = agent_props; p->name; p++) {
		pp = config_section_get_property(s,p->name);
		dprintf(1,"p->name: %s, pp: %p\n", p->name, pp);
		if (!pp) continue;
		conv_type(p->type,p->dest,p->len,pp->type,pp->dest,pp->len);
		dprintf(1,"prop: name: %s, id: %d, type: %d, flags: %02x, def: %p\n",p->name, p->id, p->type,p->flags, p->def);
	}
	config_add_props(cp, sname, agent_props, CONFIG_FLAG_NOID);
	config_dump(cp);
	return 0;
}

static int set_agent_config(config_t *cp, char *sname, solard_agentinfo_t *info) {
	config_property_t agent_props[] = {
		AGENTINFO_PROPS,
		{ 0 }
	};
	config_property_t *p, *pp;
	config_section_t *s;

	config_add_props(cp, sname, agent_props, 0 );
	return 0;
}

int solard_read_config(solard_config_t *conf) {
	char index[16],temp[1024];
	config_section_t *s,*as;
	config_property_t *p;
	int i,count;
	solard_agentinfo_t newagent,*info;

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
		conv_type(DATA_TYPE_STRING,temp,sizeof(temp)-1,p->type,p->dest,p->len);
		dprintf(1,"p->name: %s, value: %s\n", p->name, temp);
		as = config_get_section(conf->ap->cp, temp);
		if (!as) {
			log_warning("section %s does not exist for agents entry %s",temp,index);
			continue;
		}

		memset(&newagent,0,sizeof(newagent));
		info = list_add(conf->agents,&newagent,sizeof(newagent));
		dprintf(1,"info: %p\n", info);
		if (!info) continue;
		get_agent_config(conf->ap->cp, temp, info);
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

	conv_type(DATA_TYPE_LIST,&lp,0,DATA_TYPE_STRING,value,strlen(value));
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
		{ "agents", DATA_TYPE_LIST, conf->agents, 0, 0, CONFIG_FLAG_NOID },
		{ "batteries", DATA_TYPE_LIST, conf->batteries, 0, 0, CONFIG_FLAG_NOID | CONFIG_FLAG_NOSAVE },
		{0}
	};
	config_function_t sd_funcs[] = {
		{ "Add", (config_funccall_t *)solard_add_config, conf, 2 },
		{0}
	};

	conf->ap = agent_init(argc,argv,sd_opts,&sd_driver,conf,sd_props,sd_funcs);
	dprintf(1,"ap: %p\n",conf->ap);
	if (!conf->ap) return 1;
	return 0;
}

int solard_config(void *h, int req, ...) {
	solard_config_t *conf = h;
	int r;
	va_list ap;

	r = 1;
	dprintf(1,"req: %d\n", req);
	va_start(ap,req);
	switch(req) {
	case SOLARD_CONFIG_INIT:
		conf->ap = va_arg(ap,solard_agent_t *);
		dprintf(1,"name: %s\n", conf->ap->instance_name);
		dprintf(1,"conf->ap: %p\n", conf->ap);
		solard_read_config(conf);
		r = solard_jsinit(conf);
		break;
#if 0
	case SOLARD_CONFIG_MESSAGE:
		r = solard_config_getmsg(conf,va_arg(ap,solard_message_t *));
		break;
#endif
#if 0
	case SOLARD_CONFIG_GET_INFO:
		{
			json_value_t **vp;

			vp = va_arg(ap,json_value_t **);
			dprintf(1,"vp: %p\n", vp);
			if (vp) {
				*vp = solard_info(conf);
				dprintf(1,"*vp: %p\n", *vp);
				if (*vp) r = 0;
			}
		}
		break;
#endif
	}
	dprintf(1,"r: %d\n", r);
	return r;
}

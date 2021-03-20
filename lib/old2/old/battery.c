
#include "agent.h"
#include "opts.h"

#define DEBUG_STARTUP 0

#if DEBUG_STARTUP
#define _ST_DEBUG LOG_DEBUG
#else
#define _ST_DEBUG 0
#endif

int battery_init(solard_agent_t *conf) {
	return 0;
}

#if 0
solard_battery_t *battery_init(int argc, char **argv, opt_proctab_t *program_opts, char *progname) {
	solard_battery_t *conf;
	char transport[32],target[32],topts[64];
	char tp_info[128],mqtt_info[128];
	char configfile[256];
	opt_proctab_t battery_opts[] = {
		/* Spec, dest, type len, reqd, default val, have */
		{ "-t::|transport:target:opts",&tp_info,DATA_TYPE_STRING,sizeof(tp_info)-1,0,"" },
		{ "-m::|mqtt host:clientid[:user[:pass]]",&mqtt_info,DATA_TYPE_STRING,sizeof(mqtt_info)-1,0,"" },
		{ "-c:%|config file",&configfile,DATA_TYPE_STRING,sizeof(configfile)-1,0,"" },
		OPTS_END
	}, *opts;
	int logopts = LOG_INFO|LOG_WARNING|LOG_ERROR|LOG_SYSERR|_ST_DEBUG;

	opts = opt_addopts(battery_opts,program_opts);
	if (!opts) return 0;
	configfile[0] = 0;
	if (solard_common_init(argc,argv,opts,logopts)) return 0;

	conf = calloc(1,sizeof(*conf));
	if (!conf) {
		log_write(LOG_SYSERR,"calloc battery config");
		return 0;
	}
	conf->transports = list_create();

	if (strlen(configfile)) {
		char *section;
		char **p, *snames[] = { progname, "battery", "pack", "bms", 0 };

		transport[0] = target[0] = topts[0] = 0;
		conf->cfg = cfg_read(configfile);
		if (!conf->cfg) goto battery_init_error;
		/* Look for section with transport */
		section = 0;
		for(p=snames; *p; p++) {
			dprintf(1,"p: %s\n", *p);
			if (cfg_get_item(conf->cfg,*p,"transport")) {
				section = *p;
				break;
			}
		}
		dprintf(1,"section: %s\n", section);
		if (!section) {
			log_write(LOG_ERROR,"unable to find conf section with transport= in [%s] or [battery] or [pack] or [bms]\n",progname);
			goto battery_init_error;
		} else {
			cfg_proctab_t tab[] = {
				{ section, "name", "Battery name", DATA_TYPE_STRING,&conf->name,sizeof(conf->name), 0 },
				{ section, "transport", "Battery transport", DATA_TYPE_STRING,&transport,sizeof(transport), 0 },
				{ section, "target", "Transport address/interface/device", DATA_TYPE_STRING,&target,sizeof(target), 0 },
				{ section, "opts", "Transport specific options", DATA_TYPE_STRING,&topts,sizeof(topts), 0 },
				{ section, "capacity", "Pack Capacity in AH", DATA_TYPE_FLOAT,&conf->capacity, 0, 0 },
				CFG_PROCTAB_END
			};
			cfg_get_tab(conf->cfg,tab);
			dprintf(1,"debug: %d\n", debug);
			if (debug) cfg_disp_tab(tab,section,0);
		}
		mqtt_get_config(conf->cfg,&conf->mqtt);
	} else {
		if (!strlen(tp_info) || !strlen(mqtt_info)) {
			log_write(LOG_ERROR,"either configfile or transport and mqtt info must be specified.\n");
			goto battery_init_error;
		}
		strncat(transport,strele(0,":",tp_info),sizeof(transport)-1);
		strncat(target,strele(1,":",tp_info),sizeof(target)-1);
		strncat(topts,strele(2,":",tp_info),sizeof(topts)-1);
		strncat(conf->mqtt.host,strele(0,":",mqtt_info),sizeof(conf->mqtt.host)-1);
		strncat(conf->mqtt.clientid,strele(1,":",mqtt_info),sizeof(conf->mqtt.clientid)-1);
		strncat(conf->mqtt.user,strele(2,":",mqtt_info),sizeof(conf->mqtt.user)-1);
		strncat(conf->mqtt.pass,strele(3,":",mqtt_info),sizeof(conf->mqtt.pass)-1);
		dprintf(1,"host: %s, clientid: %s, user: %s, pass: %s\n", conf->mqtt.host, conf->mqtt.clientid, conf->mqtt.user, conf->mqtt.pass);
	}

	/* Test MQTT settings */
	if (strlen(conf->mqtt.host)) {
		if (!strlen(conf->mqtt.clientid)) strcpy(conf->mqtt.clientid,(strlen(conf->name) ? conf->name : "battery"));
		conf->m = mqtt_new(conf->mqtt.host,conf->mqtt.clientid);
		if (mqtt_connect(conf->m,20,conf->mqtt.user,conf->mqtt.pass)) goto battery_init_error;
	} else {
		log_write(LOG_ERROR,"mqtt host must be specified\n");
		goto battery_init_error;
	}

	if (strlen(transport) && strlen(target)) {
		conf->tp = load_transport(conf->transports,transport,SOLARD_MODTYPE_TRANSPORT);
		if (!conf->tp) goto battery_init_error;
		conf->tp_handle = conf->tp->new(conf,target);
		if (!conf->tp_handle) goto battery_init_error;
	} else {
		log_write(LOG_ERROR,"transport and target must be specified\n");
		goto battery_init_error;
	}

	return conf;
battery_init_error:
	free(conf);
	return 0;
}
#endif


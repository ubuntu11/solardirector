
#include <dlfcn.h>
#include <errno.h>
#include <sys/types.h>
#include <pwd.h>
#include "solard.h"
#include "agent.h"
#include "battery.h"

int get_mqtt_conf(sdagent_config_t *conf) {
	struct cfg_proctab tab[] = {
		{ "mqtt", "broker", "Broker URL", DATA_TYPE_STRING,&conf->mqtt_broker,sizeof(conf->mqtt_broker), 0 },
		{ "mqtt", "topic", "Topic", DATA_TYPE_STRING,&conf->mqtt_topic,sizeof(conf->mqtt_topic), 0 },
		{ "mqtt", "username", "Broker username", DATA_TYPE_STRING,&conf->mqtt_username,sizeof(conf->mqtt_username), 0 },
		{ "mqtt", "password", "Broker password", DATA_TYPE_STRING,&conf->mqtt_password,sizeof(conf->mqtt_password), 0 },
		CFG_PROCTAB_END
	};

	cfg_get_tab(conf->cfg,tab);
#ifdef DEBUG
	if (debug) cfg_disp_tab(tab,0,1);
#endif
	return 0;
}

int read_config(sdagent_config_t *conf) {
        struct cfg_proctab myconf[] = {
		{ "solard", "debug", "debug level", DATA_TYPE_INT, &debug, 0, 0 },
		{ "solard", "interval", "Time between updates", DATA_TYPE_INT, &conf->interval, 0, "30" },
		{ "solard", "max_charge_amps", "Charge Current", DATA_TYPE_FLOAT, &conf->max_charge_amps, 0, "-1" },
		{ "solard", "max_discharge_amps", "Discharge Current", DATA_TYPE_FLOAT, &conf->max_discharge_amps, 0, "-1" },
		{ "solard", "system_voltage", "Battery/System Voltage", DATA_TYPE_INT, &conf->system_voltage, 0, "48" },
		{ "solard", "battery_chem", "Battery Chemistry", DATA_TYPE_INT, &conf->battery_chem, 0, "1" },
		{ "solard", "cells", "Number of battery cells per pack", DATA_TYPE_INT, &conf->cells, 0, "14" },
		{ "solard", "cell_low", "Cell low voltage", DATA_TYPE_FLOAT, &conf->cell_low, 0, "-1" },
		{ "solard", "cell_crit_low", "Critical cell low voltage", DATA_TYPE_FLOAT, &conf->cell_crit_low, 0, "-1" },
		{ "solard", "cell_high", "Cell high voltage", DATA_TYPE_FLOAT, &conf->cell_high, 0, "-1" },
		{ "solard", "cell_crit_high", "Critical cell high voltage", DATA_TYPE_FLOAT, &conf->cell_crit_high, 0, "-1" },
		{ "solard", "c_rate", "Charge rate", DATA_TYPE_FLOAT, &conf->c_rate, 0, "-1" },
		{ "solard", "d_rate", "Discharge rate", DATA_TYPE_FLOAT, &conf->d_rate, 0, "-1" },
		/* Forced values */
		{ "solard", "capacity", "Battery Capacity", DATA_TYPE_FLOAT, &conf->user_capacity, 0, "-1" },
		{ "solard", "charge_voltage", "Charge Voltage", DATA_TYPE_FLOAT, &conf->user_charge_voltage, 0, "-1" },
		{ "solard", "charge_amps", "Charge Current", DATA_TYPE_FLOAT, &conf->user_charge_amps, 0, "-1" },
		{ "solard", "discharge_voltage", "Discharge Voltage", DATA_TYPE_FLOAT, &conf->user_discharge_voltage, 0, "-1" },
		{ "solard", "discharge_amps", "Discharge Current", DATA_TYPE_FLOAT, &conf->user_discharge_amps, 0, "-1" },
		{ "solard", "soc", "Force State of Charge", DATA_TYPE_FLOAT, &conf->user_soc, 0, "-1.0" },
		CFG_PROCTAB_END
	};

	if (conf->filename) {
		conf->cfg = cfg_read(conf->filename);
		dprintf(3,"cfg: %p\n", conf->cfg);
		if (!conf->cfg) {
			printf("error: unable to read config file '%s': %s\n", conf->filename, strerror(errno));
			return 1;
		}
	}

	cfg_get_tab(conf->cfg,myconf);
#ifdef DEBUG
	if (debug) cfg_disp_tab(myconf,0,1);
#endif

#ifdef MQTT
	if (get_mqtt_conf(conf)) return 1;
#endif

	dprintf(1,"db_name: %s\n", conf->db_name);
//	if (strlen(conf->db_name)) db_init(conf,conf->db_name);

#if 0
	conf->dlsym_handle = dlopen(0,RTLD_LAZY);
	if (!conf->dlsym_handle) {
		printf("error getting dlsym_handle: %s\n",dlerror());
		return 1;
	}
	dprintf(3,"dlsym_handle: %p\n",conf->dlsym_handle);
#endif


#if 0
	/* Init battery config */
	if (battery_init(conf)) return 1;

	/* Init inverter */
	if (inverter_init(conf)) return 1;

	/* Init battery pack */
	if (pack_init(conf)) return 1;

	/* If config is dirty, write it back out */
	dprintf(1,"conf: dirty? %d\n", solard_check_state(conf,SOLARD_CONFIG_DIRTY));
	if (solard_check_state(conf,SOLARD_CONFIG_DIRTY)) {
		cfg_write(conf->cfg);
		solard_clear_state(conf,SOLARD_CONFIG_DIRTY);
	}
#endif

	return 0;
}

int reconfig(sdagent_config_t *conf) {
	dprintf(1,"destroying lists...\n");
	list_destroy(conf->modules);
	list_destroy(conf->packs);
	dprintf(1,"destroying threads...\n");
	pthread_cancel(conf->inverter_tid);
	pthread_cancel(conf->pack_tid);
	dprintf(1,"creating lists...\n");
	conf->modules = list_create();
	conf->packs = list_create();
	free(conf->cfg);
	return read_config(conf);
}

sdagent_config_t *get_config(char *filename) {
	sdagent_config_t *conf;

	conf = calloc(1,sizeof(sdagent_config_t));
	if (!conf) {
		perror("malloc sdagent_config_t");
		return 0;
	}
	conf->modules = list_create();
	conf->packs = list_create();

	conf->filename = filename;
	if (read_config(conf)) {
		free(conf);
		return 0;
	}
	return conf;
}

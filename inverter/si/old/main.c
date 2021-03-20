
/*
Copyright (c) 2021, Stephen P. Shoecraft
All rights reserved.

This source code is licensed under the BSD-style license found in the
LICENSE file in the root directory of this source tree.
*/

#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include <stdlib.h>
#include <fcntl.h>
#include <unistd.h>
#include <stdarg.h>
#include <errno.h>
#include <ctype.h>
#include <sys/signal.h>
#include <sys/types.h>
#include <pwd.h>
#include "solard.h"
#include "agent.h"
#include "config.h"
#include "mqtt.h"
#include "battery.h"

#define DEBUG_STARTUP 0

volatile uint8_t reconf;
void usr1_handler(int signo) {
	if (signo == SIGUSR1) {
		printf("reconf!\n");
		reconf = 1;
	}
}

void usage(char *name) {
	printf("usage: %s [-acjJrwlh] [-f filename] [-b <bluetooth mac addr | -i <ip addr>] [-o output file]\n",name);
	printf("arguments:\n");
	printf("  -b <type,capacity> specify battery chem (1=lion, 2=lfp, 3=lto)\n");
#ifdef DEBUG
	printf("  -d <#>		debug output\n");
#endif
	printf("  -c <filename> specify configuration file\n");
	printf("  -l <filename> specify log file\n");
	printf("  -h		this output\n");
	printf("  -i <type:transport:target> specify inverter");
	printf("  -p <type:transport:target> specify cell monitor\n");
}

static sdagent_config_t *init(int argc, char **argv) {
	int logopts = LOG_INFO|LOG_WARNING|LOG_ERROR|LOG_SYSERR;
	int opt,battery_chem,battery_cap;
	char inv_type[SOLARD_MODULE_NAME_LEN+1],inv_transport[SOLARD_MODULE_NAME_LEN+1],inv_target[SOLARD_TARGET_LEN+1];
	sigset_t set;
	sdagent_config_t *conf;
	char *configfile,*logfile;

	battery_chem = battery_cap = -1;
	inv_type[0] = inv_transport[0] = inv_target[0] = 0;
	configfile = logfile = 0;
	while ((opt=getopt(argc, argv, "b:d:c:l:i:e:ht")) != -1) {
		switch (opt) {
		case 'b':
			battery_chem = atoi(strele(0,",",optarg));
			battery_cap = atoi(strele(1,",",optarg));
			break;
#ifdef DEBUG
		case 'd':
			debug=atoi(optarg);
			logopts |= LOG_DEBUG;
			break;
#endif
		case 'c':
			configfile=optarg;
			break;
		case 'l':
			logfile=optarg;
			break;
                case 'i':
			strncpy(inv_type,strele(0,":",optarg),sizeof(inv_type)-1);
			strncpy(inv_transport,strele(1,":",optarg),sizeof(inv_transport)-1);
			strncpy(inv_target,strele(2,":",optarg),sizeof(inv_target)-1);
			if (!strlen(inv_type) || !strlen(inv_transport) || !strlen(inv_target)) {
				printf("error: format is type:transport:target\n");
				usage(argv[0]);
				return 0;
			}
			break;
		case 'h':
		default:
			usage(argv[0]);
			exit(0);
                }
	}

	/* Open logfile if specified */
	if (logfile) log_open("siagent",logfile,logopts|LOG_TIME);

	conf = get_config(configfile);
	dprintf(4,"conf: %p\n", conf);
	if (!conf) return 0;
#ifdef MQTT
	mqtt_init(conf);
#endif

	DDLOG("battery_chem: %d\n",battery_chem);
	if (battery_chem >= 0) {
		conf->battery_chem = battery_chem;
		conf->capacity = battery_cap >= 0 ? battery_cap : 100;
		conf->cell_low = conf->cell_crit_low = conf->cell_high = conf->cell_crit_high = -1;
		conf->c_rate = -1;
		battery_init(conf);
	}

	if (strlen(inv_type)) {
		solard_inverter_t *inv;
		inv = calloc(1,sizeof(*inv));
		if (!inv) {
			perror("calloc inverter");
			return 0;
		}
		strcpy(inv->type,inv_type);
		strcpy(inv->transport,inv_transport);
		strcpy(inv->target,inv_target);
		inverter_add(conf,inv);
	}

	/* Ignore SIGPIPE */
        sigemptyset(&set);
        sigaddset(&set, SIGPIPE);
        sigprocmask(SIG_BLOCK, &set, NULL);

	/* USR causes config re-read (could also check file modification time) */
	reconf = 0;
        signal(SIGUSR1, usr1_handler);

	return conf;
}

int main(int argc, char **argv) {
	int startup,inv_reported,packs_reported,cell,in_range,npacks;
	sdagent_config_t *conf;
	solard_inverter_t *inv;
	solard_pack_t *pp;
	float cell_total, cell_min, cell_max, cell_diff, cell_avg;
	uint32_t mask;
	time_t start,end,diff;

	/* Initialize system */
	conf = init(argc,argv);
	if (!conf) return 1;

	/* Start off by setting SOC to 99% and charge_amps to 0.1 then to normal values on 2nd pass */
	startup = 1;

	while(1) {
		if (reconf) {
			dprintf(1,"calling reconfig...\n");
			reconfig(conf);
			reconf = 0;
		}
		inv_reported = packs_reported = 0;
		conf->battery_voltage = conf->battery_current = 0.0;

		/* Get starting time */
		time(&start);

		/* Update inverter */
		inv = conf->inverter;
		if (inv) {
			inverter_read(inv);
			if (solard_check_state(inv,SOLARD_INVERTER_STATE_UPDATED)) {
				inv_reported++;
				if ((int)inv->battery_voltage) conf->battery_voltage = inv->battery_voltage;
				if ((int)inv->battery_current) conf->battery_current = inv->battery_current;
			}
		}
		packs_reported = 0;

		/* If we dont have any inverts or packs, no sense continuing... */
		dprintf(1,"inv_reported: %d, pack_reported: %d\n", inv_reported, packs_reported);
		if (!inv_reported && !packs_reported) {
			if (!conf->filename && !conf->inverter) {
				printf("no config file, no inverter specified and no pack specified, nothing to do!\n");
				break;
			} else {
				sleep(conf->interval);
				continue;
			}
		}

		lprintf(0,"Battery voltage: %.1f, Battery current: %.1f\n", conf->battery_voltage, conf->battery_current);

		dprintf(2,"user_charge_voltage: %.1f, user_charge_amps: %.1f\n", conf->user_charge_voltage, conf->user_charge_amps);
		conf->charge_voltage = conf->user_charge_voltage < 0.0 ? conf->cell_high * conf->cells : conf->user_charge_voltage;
		dprintf(2,"conf->c_rate: %f, conf->capacity: %f\n", conf->c_rate, conf->capacity);
		conf->charge_amps = conf->user_charge_amps < 0.0 ? conf->c_rate * conf->capacity : conf->user_charge_amps;
		if (startup) conf->charge_amps = 0.1;
		lprintf(0,"Charge voltage: %.1f, Charge amps: %.1f\n", conf->charge_voltage, conf->charge_amps);

		dprintf(2,"user_discharge_voltage: %.1f, user_discharge_amps: %.1f\n", conf->user_discharge_voltage, conf->user_discharge_amps);
		conf->discharge_voltage = conf->user_discharge_voltage < 0.0 ? conf->cell_low * conf->cells : conf->user_discharge_voltage;
		dprintf(2,"conf->d_rate: %f, conf->capacity: %f\n", conf->d_rate, conf->capacity);
		conf->discharge_amps = conf->user_discharge_amps < 0.0 ? conf->d_rate * conf->capacity : conf->user_discharge_amps;
		lprintf(0,"Discharge voltage: %.1f, Discharge amps: %.1f\n", conf->discharge_voltage, conf->discharge_amps);

		conf->soc = conf->user_soc < 0.0 ? ( ( conf->battery_voltage - conf->discharge_voltage) / (conf->charge_voltage - conf->discharge_voltage) ) * 100.0 : conf->user_soc;
		if (startup) conf->soc = 99.9;
		lprintf(0,"SoC: %.1f\n", conf->soc);
		conf->soh = 100.0;

		/* Update inverter */
		if (inv_reported) {
			inverter_write(conf->inverter);
			startup = 0;
		}

		/* Get ending time */
		time(&end);

		dprintf(3,"start: %d, end: %d, diff: %d, interval: %d\n",(int)start,(int)end,(int)end-(int)start,conf->interval);
		diff = end - start;
		if (diff < conf->interval) {
			int delay;

			delay = conf->interval - (int)diff;
			dprintf(1,"interval: %d, diff: %d, delay: %d\n", conf->interval,diff,delay);
			dprintf(1,"Sleeping for %d seconds...\n",delay);
			sleep(delay);
		}
	}

	return 0;
}

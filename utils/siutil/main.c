
/*
Copyright (c) 2021, Stephen P. Shoecraft
All rights reserved.

This source code is licensed under the BSD-style license found in the
LICENSE file in the root directory of this source tree.
*/

#include "common.h"
#include "client.h"
#include "smanet.h"

#define DEBUG_STARTUP 1

#if DEBUG_STARTUP
#define _ST_DEBUG LOG_DEBUG
#else
#define _ST_DEBUG 0
#endif

void usage(char *myname) {
	printf("usage: %s [-alps] -n chan | ChanName [value]\n",myname);
	return;
}
int main(int argc, char **argv) {
	smanet_session_t *s;
	int param,list,all;
	char configfile[256],tpinfo[128],chanpath[256];
	char transport[32],target[64],topts[32];
	opt_proctab_t opts[] = {
                /* Spec, dest, type len, reqd, default val, have */
		{ "-b",0,0,0,0,0 },
		{ "-l|list",&list,DATA_TYPE_BOOL,0,0,"N" },
		{ "-p|list params",&all,DATA_TYPE_BOOL,0,0,"N" },
		{ "-a|list all",&all,DATA_TYPE_BOOL,0,0,"N" },
		{ "-t::|<transport,target,topts>",&tpinfo,DATA_TYPE_STRING,sizeof(tpinfo)-1,0,"" },
		{ "-c::|configfile",&configfile,DATA_TYPE_STRING,sizeof(configfile)-1,0,"" },
		{ "-p::|channels path",&chanpath,DATA_TYPE_STRING,sizeof(chanpath)-1,0,"" },
		OPTS_END
	};
	cfg_proctab_t tab[] = {
		{ "siutil","transport","Device transport",DATA_TYPE_STRING,transport,SOLARD_TRANSPORT_LEN-1, "" },
		{ "siutil","target","Device target",DATA_TYPE_STRING,target,SOLARD_TARGET_LEN-1, "" },
		{ "siutil","topts","Device transport options",DATA_TYPE_STRING,topts,SOLARD_TOPTS_LEN-1, "" },
		CFG_PROCTAB_END
	};
	solard_module_t *tp;
	void *tp_handle;
	int logopts = LOG_INFO|LOG_WARNING|LOG_ERROR|LOG_SYSERR|_ST_DEBUG;
	int type,i,action;
	cfg_info_t *cfg;
	smanet_channel_t *c;
	smanet_value_t *v,nv;

	log_open("siutil",0,logopts);

	all = list = param = 0;
	configfile[0] = tpinfo[0] = 0;
	if (solard_common_init(argc,argv,opts,logopts)) return 1;
	dprintf(1,"all: %d, list: %d, param: %d\n", all, list, param);

	if (debug) logopts |= LOG_DEBUG|LOG_DEBUG2;
	log_open("siutil",0,logopts);

	if (all && !list) printf("info: all flag (-a) only applies to list (-l), ignored.\n");

	argc -= optind;
	argv += optind;

	for(i=0; i < argc; i++) dprintf(1,"arg[%d]: %s\n",i,argv[i]);

	dprintf(1,"argc: %d\n", argc);
	if (argc < 1 && list == 0) {
		printf("argc < 1 && list\n");
		return 1;
	}
	if (list) {
		action = 0;
		if (all) {
			type = 3;
		} else if (param) {
			type = 1;
		} else {
			type = 0;
		}
	} else if (argc < 2) {
		action = 1;
	} else {
		action = 2;
	}
	dprintf(1,"action: %d, type: %d\n", action, type);

	/* -t takes precedence */
	cfg = 0;
	dprintf(1,"tpinfo: %s, configfile: %s\n", tpinfo, configfile);
	*transport = *target = *topts = 0;
	if (strlen(tpinfo)) {
		*transport = *target = *topts = 0;
		strncat(transport,strele(0,",",tpinfo),sizeof(transport)-1);
		strncat(target,strele(1,",",tpinfo),sizeof(target)-1);
		strncat(topts,strele(2,",",tpinfo),sizeof(topts)-1);
	} else if (strlen(configfile)) {
		cfg = cfg_read(configfile);
		if (!cfg) {
			log_write(LOG_SYSERROR,"error reading configfile '%s'", configfile);
			return 1;
		}
	} else {
		dprintf(1,"finding conf...\n");
		if (find_config_file("siutil.conf",configfile,sizeof(configfile)-1)) {
			printf("error: unable to find siutil.conf\n");
			return 1;
		}
		dprintf(1,"configfile: %s\n", configfile);
		cfg = cfg_read(configfile);
		if (!cfg) {
			log_write(LOG_SYSERROR,"error reading configfile '%s'", configfile);
			return 1;
		}
	}
	if (cfg) {
		cfg_get_tab(cfg,tab);
		if (debug) cfg_disp_tab(tab,"siutil",0);
	}
	if (strlen(transport) == 0 || strlen(target) == 0) {
		log_write(LOG_ERROR,"transport and target must be specified in config or command line\n");
		return 1;
	}
	dprintf(1,"transport: %s, target: %s, topts: %s\n", transport, target, topts);
	tp = load_module(0,transport,SOLARD_MODTYPE_TRANSPORT);
	if (!tp) return 1;
	tp_handle = tp->new(0,target,topts);
	if (!tp_handle) return 1;
	s = smanet_init(tp, tp_handle);
	if (!s) return 1;

	dprintf(1,"chanpath: %s\n", chanpath);
	if (!strlen(chanpath)) sprintf(chanpath,"%s/%s.dat",LIBDIR,s->type);
	if (smanet_load_channels(s,chanpath) != 0) {
		dprintf(1,"count: %d\n", list_count(s->channels));
		if (!list_count(s->channels)) {
			log_write(LOG_INFO,"Downloading channels...\n");
			smanet_get_channels(s);
			smanet_save_channels(s,chanpath);
		}
	}

	dprintf(1,"action: %d\n", action);
	switch(action) {
	case 0:
		list_reset(s->channels);
		while((c = list_get_next(s->channels)) != 0) printf("%s\n",c->name);
		break;
	case 1:
		c = smanet_get_channelbyname(s,argv[0]);
		if (!c) {
			log_write(LOG_ERROR,"%s: not found\n",argv[0]);
			return 1;
		}
		if (c->mask & CH_STATUS) {
			smanet_get_optionbyname(s,c->name,tpinfo,sizeof(tpinfo)-1);
			printf("%s: %s\n", c->name, tpinfo);
		} else {
			v = smanet_get_value(s,c);
			if (!v) {
				log_write(LOG_ERROR,"%s: error getting value\n",c->name);
				return 1;
			}
			conv_type(DATA_TYPE_STRING,tpinfo,sizeof(tpinfo)-1,v->type,&v->bval,0);
		}
		break;
	case 2:
		c = smanet_get_channelbyname(s,argv[0]);
		if (!c) {
			log_write(LOG_ERROR,"%s: not found\n",argv[0]);
			return 1;
		}
		if (c->mask & CH_STATUS) {
			smanet_set_optionbyname(s,c->name,argv[1]);
		} else {
			v = smanet_get_value(s,c);
			if (!v) {
				log_write(LOG_ERROR,"%s: error getting value\n",c->name);
				return 1;
			}
			conv_type(v->type,&v->bval,0,DATA_TYPE_STRING,argv[1],strlen(argv[1]));
		}
		break;
	}

	return 0;
}


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

struct siutil_session {
	solard_module_t *can;
	void *can_handle;
	solard_module_t *tty;
	void *tty_handle;
	smanet_session_t *smanet;
};
typedef struct siutil_session siutil_session_t;

/* Func for can data that is remote (dont use thread/messages) */
int can_read(siutil_session_t *s, int id, uint8_t *data, int datasz) {
	int retries,bytes,len;
	struct can_frame frame;

	dprintf(5,"id: %03x, data: %p, data_len: %d\n", id, data, datasz);

	retries=5;
	while(retries--) {
		bytes = s->can->read(s->can_handle,&frame,id);
		dprintf(5,"bytes: %d\n", bytes);
		if (bytes < 0) return 1;
		if (bytes == sizeof(frame)) {
			len = (frame.can_dlc > datasz ? datasz : frame.can_dlc);
			memcpy(data,&frame.data,len);
			if (debug >= 5) bindump("FROM SERVER",data,len);
			break;
		}
		sleep(1);
	}
	dprintf(5,"returning: %d\n", (retries > 0 ? 0 : 1));
	return (retries > 0 ? 0 : 1);
}

int can_write(siutil_session_t *s, int id, uint8_t *data, int data_len) {
	struct can_frame frame;
	int len;

	dprintf(1,"id: %03x, data: %p, data_len: %d\n",id,data,data_len);

	len = data_len > 8 ? 8 : data_len;

	memset(&frame,0,sizeof(frame));
	frame.can_id = id;
	frame.can_dlc = len;
	memcpy(&frame.data,data,len);
	if (debug > 2) bindump("write data",data,data_len);
	return s->can->write(s->can_handle,&frame,sizeof(frame));
}

int startstop(siutil_session_t *s, int op) {
	int retries;
	uint8_t data[8],b;

	dprintf(1,"op: %d, have_can: %d\n", op, (s->can != 0));
	if (!s->can) {
		log_write(LOG_ERROR,"Start/Stop requested but no can interface specified");
		return 1;
	}
	s->can->open(s->can_handle);

	b = (op ? 1 : 2);
	dprintf(1,"b: %d\n", b);
	retries=10;
	while(retries--) {
		dprintf(1,"writing...\n");
		if (can_write(s,0x35c,&b,1) < 0) return 1;
		dprintf(1,"reading...\n");
		if (can_read(s,0x307,data,8) == 0) {
			if (debug >= 3) bindump("data",data,8);
			dprintf(1,"*** data[3] & 2: %d\n", data[3] & 0x0002);
			if (op) {
				if (data[3] & 0x0002) return 0;
			} else {
				if ((data[3] & 0x0002) == 0) return 0;
			}
		}
		sleep(1);
	}
	if (retries < 0) printf("start failed.\n");
	s->can->close(s->can_handle);
	return (retries < 0 ? 1 : 0);
}

void usage(char *myname) {
	printf("usage: %s [-alps] -n chan | ChanName [value]\n",myname);
        printf("  -j            JSON output\n");
        printf("  -J            JSON output pretty print\n");
        printf("  -s            Start SI\n");
        printf("  -S            Stop SI\n");
        printf("  -h            this output\n");
	return;
}

enum ACTIONS {
	ACTION_LIST,
	ACTION_GET,
	ACTION_SET
};

int main(int argc, char **argv) {
	siutil_session_t *s;
	int param,list,all;
	char configfile[256],tpinfo[128],chanpath[256];
	char transport[32],target[64],topts[32];
	int start_flag,stop_flag,info_flag;
	opt_proctab_t opts[] = {
                /* Spec, dest, type len, reqd, default val, have */
		{ "-b",0,0,0,0,0 },
		{ "-s|Start Sunny Island",&start_flag,DATA_TYPE_BOOL,0,0,"N" },
		{ "-S|Stop Sunny Island",&stop_flag,DATA_TYPE_BOOL,0,0,"N" },
		{ "-l|list",&list,DATA_TYPE_BOOL,0,0,"N" },
		{ "-p|list params",&all,DATA_TYPE_BOOL,0,0,"N" },
		{ "-a|list all",&all,DATA_TYPE_BOOL,0,0,"N" },
		{ "-i|disp info",&info_flag,DATA_TYPE_BOOL,0,0,"N" },
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
	int logopts = LOG_INFO|LOG_WARNING|LOG_ERROR|LOG_SYSERR|_ST_DEBUG;
	int type,i,action;
	cfg_info_t *cfg;
	smanet_channel_t *c;

	log_open("siutil",0,logopts);

	all = list = param = 0;
	configfile[0] = tpinfo[0] = 0;
	if (solard_common_init(argc,argv,opts,logopts)) return 1;
	dprintf(1,"all: %d, list: %d, param: %d\n", all, list, param);

	if (debug) logopts |= LOG_DEBUG|LOG_DEBUG2;
	log_open("siutil",0,logopts);

	s = calloc(1,sizeof(*s));
	if (!s) {
		log_write(LOG_SYSERR,"calloc session");
		return 1;
	}

	if (all && !list) printf("info: all flag (-a) only applies to list (-l), ignored.\n");

	argc -= optind;
	argv += optind;

	for(i=0; i < argc; i++) dprintf(1,"arg[%d]: %s\n",i,argv[i]);

	dprintf(1,"argc: %d\n", argc);
	if (argc < 1 && list == 0 && !start_flag && !stop_flag) {
		printf("argc < 1 && list\n");
		return 1;
	}
	if (list) {
		action = ACTION_LIST;
		if (all) {
			type = 3;
		} else if (param) {
			type = 1;
		} else {
			type = 0;
		}
	} else if (argc < 2) {
		action = ACTION_GET;
	} else {
		action = ACTION_SET;
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
	if (strcmp(transport,"can") == 0 || (strcmp(transport,"rdev") == 0 && strncmp(topts,"can",3) == 0)) {
		s->can = load_module(0,transport,SOLARD_MODTYPE_TRANSPORT);
		if (!s->can) return 1;
		s->can_handle = s->can->new(0,target,topts);
		if (!s->can_handle) return 1;
	} else {
		s->tty = load_module(0,transport,SOLARD_MODTYPE_TRANSPORT);
		if (!s->tty) return 1;
		s->tty_handle = s->tty->new(0,target,topts);
		if (!s->tty_handle) return 1;
		s->smanet = smanet_init(s->tty, s->tty_handle);
		if (!s->smanet) return 1;
	}

	if (start_flag) return startstop(s,1);
	else if (stop_flag) return startstop(s,0);
	else if (s->can) return 1;

	dprintf(1,"chanpath: %s\n", chanpath);
	if (!strlen(chanpath)) sprintf(chanpath,"%s/%s.dat",SOLARD_LIBDIR,s->smanet->type);
	if (smanet_load_channels(s->smanet,chanpath) != 0) {
		dprintf(1,"count: %d\n", list_count(s->smanet->channels));
		if (!list_count(s->smanet->channels)) {
			log_write(LOG_INFO,"Downloading channels...\n");
			smanet_read_channels(s->smanet);
			smanet_save_channels(s->smanet,chanpath);
		}
	}

	dprintf(1,"action: %d\n", action);
	switch(action) {
	case ACTION_LIST:
		list_reset(s->smanet->channels);
		while((c = list_get_next(s->smanet->channels)) != 0) printf("%s\n",c->name);
		break;
	case ACTION_GET:
		{
			double d;
			char *text;

			if (smanet_get_value(s->smanet,argv[0],&d,&text)) {
				log_write(LOG_ERROR,"%s: error getting value\n",argv[0]);
				return 1;
			}
			if (text) printf("%s\n",text);
			else if ((int)d == d) printf("%d\n",(int)d);
			else printf("%f\n",d);
		}
		break;
	case ACTION_SET:
		smanet_set_value(s->smanet,argv[0],atof(argv[1]),argv[1]);
		break;
	}

	return 0;
}

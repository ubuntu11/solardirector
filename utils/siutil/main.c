
/*
Copyright (c) 2021, Stephen P. Shoecraft
All rights reserved.

This source code is licensed under the BSD-style license found in the
LICENSE file in the root directory of this source tree.
*/

#include "common.h"
#include "transports.h"
#include "si.h"
#include "si_info.h"

#define DEBUG_STARTUP 0

#if DEBUG_STARTUP
#define _ST_DEBUG LOG_DEBUG
#else
#define _ST_DEBUG 0
#endif

int outfmt = 0;
FILE *outfp;
char sepch = ',';
char *sepstr = ",";

void dint(char *label, char *format, int val) {
	char temp[128];

	dprintf(3,"dint: label: %s, val: %d\n", label, val);
	switch(outfmt) {
	case 2:
//		json_object_set_number(root_object, label, val);
		break;
	case 1:
		sprintf(temp,"%%s,%s\n",format);
		dprintf(3,"temp: %s\n", temp);
		fprintf(outfp,temp,label,val);
		break;
	default:
		sprintf(temp,"%%-25s %s\n",format);
		dprintf(3,"temp: %s\n", temp);
		fprintf(outfp,temp,label,val);
		break;
	}
}
#define _dint(l,v) dint(l,"%d",v)

void dfloat(char *label, char *format, float val) {
	char temp[128];

	dprintf(3,"dint: label: %s, val: %f\n", label, val);
	switch(outfmt) {
	case 2:
//		json_object_set_number(root_object, label, val);
		break;
	case 1:
		sprintf(temp,"%%s,%s\n",format);
		dprintf(3,"temp: %s\n", temp);
		fprintf(outfp,temp,label,val);
		break;
	default:
		sprintf(temp,"%%-25s %s\n",format);
		dprintf(3,"temp: %s\n", temp);
		fprintf(outfp,temp,label,val);
		break;
	}
}
#define _dfloat(l,v) dfloat(l,"%f",v)

void dstr(char *label, char *format, char *val) {
	char temp[128];

	dprintf(3,"dint: label: %s, val: %s\n", label, val);
	switch(outfmt) {
	case 2:
//		json_object_set_string(root_object, label, val);
		break;
	case 1:
		sprintf(temp,"%%s,%s\n",format);
		dprintf(3,"temp: %s\n", temp);
		fprintf(outfp,temp,label,val);
		break;
	default:
		sprintf(temp,"%%-25s %s\n",format);
		dprintf(3,"temp: %s\n", temp);
		fprintf(outfp,temp,label,val);
		break;
	}
}
#define _dstr(l,v) dstr(l,"%s",v)

static inline void _addstr(char *str,char *newstr) {
	dprintf(4,"str: %s, newstr: %s\n", str, newstr);
	if (strlen(str)) strcat(str,sepstr);
	dprintf(4,"str: %s\n", str);
	if (outfmt == 2) strcat(str,"\"");
	strcat(str,newstr);
	if (outfmt == 2) strcat(str,"\"");
	dprintf(4,"str: %s\n", str);
}

void _outpower(char *label, si_power_t *p) {
	char str[128],temp[196];

	str[0] = 0;
	sprintf(temp,"%3.2f",p->l1);
	_addstr(str,temp);
	sprintf(temp,"%3.2f",p->l2);
	_addstr(str,temp);
	sprintf(temp,"%3.2f",p->l3);
	_addstr(str,temp);
	switch(outfmt) {
	case 2:
		sprintf(temp,"[ %s ]",str);
		dprintf(1,"temp: %s\n", temp);
//		json_object_dotset_value(root_object, label, json_parse_string(temp));
		break;
	case 1:
		fprintf(outfp,"%s,%s\n",label,str);
		break;
	default:
		fprintf(outfp,"%-25s %s\n",label,str);
		break;
	}
}

void _outrelay(char *label, int r1, int r2) {
	char str[32],temp[36];

	str[0] = 0;
	_addstr(str,r1 ? "1" : "0");
	_addstr(str,r2 ? "1" : "0");
	switch(outfmt) {
	case 2:
		sprintf(temp,"[ %s ]",str);
		dprintf(1,"temp: %s\n", temp);
//		json_object_dotset_value(root_object, label, json_parse_string(temp));
		break;
	case 1:
		fprintf(outfp,"%s,%s\n",label,str);
		break;
	default:
		fprintf(outfp,"%-25s %s\n",label,str);
		break;
	}
}

void display_info(si_info_t *info) {
	_outpower("Active Grid",&info->active.grid);
	_outpower("Active SI",&info->active.si);
	_outpower("ReActive Grid",&info->reactive.grid);
	_outpower("ReActive SI",&info->reactive.si);
	_outpower("AC1 Voltage",&info->voltage);
	dfloat("AC1 Frequency","%2.2f",info->frequency);
	_outpower("AC2 Voltage",&info->ac2);
	dfloat("AC2 Frequency","%2.2f",info->ac2_frequency);
	dfloat("Battery Voltage","%3.2f",info->battery_voltage);
	dfloat("Battery Current","%3.2f",info->battery_current);
	dfloat("Battery Temp","%3.2f",info->battery_temp);
	dfloat("Battery SoC","%3.2f",info->battery_soc);
	dfloat("Battery CVSP","%3.2f",info->battery_cvsp);
	_outrelay("Master Relay",info->bits.relay1,info->bits.relay2);
	_outrelay("Slave1 Relays",info->bits.s1_relay1,info->bits.s1_relay2);
	_outrelay("Slave2 Relays",info->bits.s2_relay1,info->bits.s2_relay2);
	_outrelay("Slave3 Relays",info->bits.s3_relay1,info->bits.s3_relay2);
	_dstr("Generator Running",info->bits.GnRn ? "true" : "false");
	_dstr("Generator Autostart",info->bits.AutoGn ? "true" : "false");
	_dstr("AutoLodExt",info->bits.AutoLodExt ? "true" : "false");
	_dstr("AutoLodSoc",info->bits.AutoLodSoc ? "true" : "false");
	_dstr("Tm1",info->bits.Tm1 ? "true" : "false");
	_dstr("Tm2",info->bits.Tm2 ? "true" : "false");
	_dstr("ExtPwrDer",info->bits.ExtPwrDer ? "true" : "false");
	_dstr("ExtVfOk",info->bits.ExtVfOk ? "true" : "false");
	_dstr("Grid On",info->bits.GdOn ? "true" : "false");
	_dstr("Error",info->bits.Error ? "true" : "false");
	_dstr("Run",info->bits.Run ? "true" : "false");
	_dstr("Battery Fan On",info->bits.BatFan ? "true" : "false");
	_dstr("Acid Circulation On",info->bits.AcdCir ? "true" : "false");
	_dstr("MccBatFan",info->bits.MccBatFan ? "true" : "false");
	_dstr("MccAutoLod",info->bits.MccAutoLod ? "true" : "false");
	_dstr("CHP #1 On",info->bits.Chp ? "true" : "false");
	_dstr("CHP #2 On",info->bits.ChpAdd ? "true" : "false");
	_dstr("SiComRemote",info->bits.SiComRemote ? "true" : "false");
	_dstr("Overload",info->bits.OverLoad ? "true" : "false");
	_dstr("ExtSrcConn",info->bits.ExtSrcConn ? "true" : "false");
	_dstr("Silent",info->bits.Silent ? "true" : "false");
	_dstr("Current",info->bits.Current ? "true" : "false");
	_dstr("FeedSelfC",info->bits.FeedSelfC ? "true" : "false");
	_outpower("Load",&info->load);
	_dint("Charging procedure",info->charging_proc);
	_dint("State",info->state);
	_dint("Error Message",info->errmsg);
	dfloat("PVPwrAt","%3.2f",info->PVPwrAt);
	dfloat("GdCsmpPwrAt","%3.2f",info->GdCsmpPwrAt);
	dfloat("GdFeedPwr","%3.2f",info->GdFeedPwr);
}

int si_config(void *h, int req, ...) {
//        si_session_t *s = h;
//        va_list va;
//        void **vp;
//        int r;

//        r = 1;
//        va_start(va,req);
        dprintf(1,"req: %d\n", req);
	return 1;
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
	ACTION_INFO,
	ACTION_GET,
	ACTION_SET,
	ACTION_FILE
};

//int si_config(void *h, int req, ...) { return(1); }

static solard_driver_t *transports[] = { &can_driver, &serial_driver, &rdev_driver, 0 };

int main(int argc, char **argv) {
	si_session_t *s;
	int param,list,all;
	char configfile[256],cantpinfo[256],smatpinfo[256],chanpath[256],filename[256];
	int start_flag,stop_flag,ver_flag,quiet_flag;
	opt_proctab_t opts[] = {
                /* Spec, dest, type len, reqd, default val, have */
		{ "-b",0,0,0,0,0 },
		{ "-s|Start Sunny Island",&start_flag,DATA_TYPE_BOOL,0,0,"N" },
		{ "-S|Stop Sunny Island",&stop_flag,DATA_TYPE_BOOL,0,0,"N" },
		{ "-l|list",&list,DATA_TYPE_BOOL,0,0,"N" },
		{ "-p|list params",&param,DATA_TYPE_BOOL,0,0,"N" },
		{ "-a|list all",&all,DATA_TYPE_BOOL,0,0,"N" },
		{ "-f:%|get/set vals in file",&filename,DATA_TYPE_STRING,sizeof(filename)-1,0,"" },
		{ "-v|verify settings in file",&ver_flag,DATA_TYPE_BOOL,0,0,"N" },
		{ "-q|dont list changes",&quiet_flag,DATA_TYPE_BOOL,0,0,"N" },
		{ "-t::|CAN <transport,target,topts>",&cantpinfo,DATA_TYPE_STRING,sizeof(cantpinfo)-1,0,"" },
		{ "-u::|SMANET <transport,target,topts>",&smatpinfo,DATA_TYPE_STRING,sizeof(smatpinfo)-1,0,"" },
		{ "-c::|configfile",&configfile,DATA_TYPE_STRING,sizeof(configfile)-1,0,"" },
		{ "-p::|channels path",&chanpath,DATA_TYPE_STRING,sizeof(chanpath)-1,0,"" },
		OPTS_END
	};
	char can_transport[SOLARD_TRANSPORT_LEN],can_target[SOLARD_TARGET_LEN],can_topts[SOLARD_TOPTS_LEN];
	char sma_transport[SOLARD_TRANSPORT_LEN],sma_target[SOLARD_TARGET_LEN],sma_topts[SOLARD_TOPTS_LEN];
	int logopts = LOG_INFO|LOG_WARNING|LOG_ERROR|LOG_SYSERR|_ST_DEBUG;
	int type,i,action,source;
	cfg_info_t *cfg;
	smanet_channel_t *c;
	solard_driver_t *tp;
	void *tp_handle;
	si_info_t info;

	log_open("siutil",0,logopts);

	all = list = param = 0;
	*configfile = *cantpinfo = *smatpinfo = 0;
	if (solard_common_init(argc,argv,opts,logopts)) return 1;
	if (debug) logopts |= LOG_DEBUG|LOG_DEBUG2;
	log_open("siutil",0,logopts);

	argc -= optind;
	argv += optind;

	for(i=0; i < argc; i++) dprintf(1,"arg[%d]: %s\n",i,argv[i]);

	dprintf(1,"all: %d, list: %d, param: %d\n", all, list, param);
	if (all && !list) printf("info: all flag (-a) only applies to list (-l), ignored.\n");

	dprintf(1,"argc: %d\n", argc);
	source = 1;
	if (list) {
		action = ACTION_LIST;
		if (all) {
			type = 3;
		} else if (param) {
			type = 1;
		} else {
			type = 0;
		}
	} else if (argc == 0) {
		if (strlen(filename))
			action = ACTION_FILE;
		else {
			action = ACTION_INFO;
			source = 2;
		}
	} else if (argc > 1) {
		action = ACTION_SET;
	} else if (argc > 0) {
		action = ACTION_GET;
	}
	if (start_flag || stop_flag) source = 2;
	dprintf(1,"source: %d, action: %d, list: %d, all: %d, type: %d\n", source, action, list, all, type);

	/* Read config */
	if (!strlen(configfile)) find_config_file("siutil.conf",configfile,sizeof(configfile)-1);
	if (strlen(configfile)) {
		cfg = cfg_read(configfile);
		if (!cfg) {
			log_write(LOG_SYSERROR,"error reading configfile '%s'", configfile);
			return 1;
		}
	}

	/* -t takes precedence over config */
	cfg = 0;
	dprintf(1,"cantpinfo: %s, smatpinfo: %s, configfile: %s\n", cantpinfo, smatpinfo, configfile);
	*can_transport = *can_target = *can_topts = 0;
	if (strlen(cantpinfo)) {
		strncat(can_transport,strele(0,",",cantpinfo),sizeof(can_transport)-1);
		strncat(can_target,strele(1,",",cantpinfo),sizeof(can_target)-1);
		strncat(can_topts,strele(2,",",cantpinfo),sizeof(can_topts)-1);
	} else {
		cfg_proctab_t tab[] = {
			{ "siutil","can_transport","CAN transport",DATA_TYPE_STRING,can_transport,SOLARD_TRANSPORT_LEN-1, "" },
			{ "siutil","can_target","CAN target",DATA_TYPE_STRING,can_target,SOLARD_TARGET_LEN-1, "" },
			{ "siutil","can_topts","CAN transport options",DATA_TYPE_STRING,can_topts,SOLARD_TOPTS_LEN-1, "" },
			CFG_PROCTAB_END
		};
		if (cfg) {
			cfg_get_tab(cfg,tab);
			if (debug) cfg_disp_tab(tab,"can",0);
		}
	}
        if (!strlen(can_transport) || !strlen(can_target)) {
#if defined(__WIN32) || defined(__WIN64)
                strcpy(can_transport,"serial");
                strcpy(can_target,"COM1");
                strcpy(can_topts,"9600");
#else
		strcpy(can_transport,"can");
		strcpy(can_target,"can0");
		strcpy(can_topts,"500000");
#endif
	}

	tp = tp_handle = 0;
	if (source == 2) {
		/* Load the can driver */
		dprintf(1,"can_transport: %s, can_target: %s, can_topts: %s\n", can_transport, can_target, can_topts);
		tp = find_driver(transports,can_transport);
		if (!tp) return 1;
		tp_handle = tp->new(0,can_target,can_topts);
		if (!tp_handle) return 1;
	}

	/* Init the SI driver */
	s = si_driver.new(0,tp,tp_handle);
	/* Only an error if we need can */
	if (!s && source == 2) return 1;

	*sma_transport = *sma_target = *sma_topts = 0;
	if (strlen(smatpinfo)) {
		strncat(sma_transport,strele(0,",",smatpinfo),sizeof(sma_transport)-1);
		strncat(sma_target,strele(1,",",smatpinfo),sizeof(sma_target)-1);
		strncat(sma_topts,strele(2,",",smatpinfo),sizeof(sma_topts)-1);
	} else {
		cfg_proctab_t tab[] = {
			{ "siutil","smanet_transport","SMANET transport",DATA_TYPE_STRING,sma_transport,SOLARD_TRANSPORT_LEN-1, "" },
			{ "siutil","smanet_target","SMANET target",DATA_TYPE_STRING,sma_target,SOLARD_TARGET_LEN-1, "" },
			{ "siutil","smanet_topts","SMANET transport options",DATA_TYPE_STRING,sma_topts,SOLARD_TOPTS_LEN-1, "" },
			CFG_PROCTAB_END
		};
		if (cfg) {
			cfg_get_tab(cfg,tab);
			if (debug) cfg_disp_tab(tab,"can",0);
		}
	}
        if (!strlen(sma_transport) || !strlen(sma_target)) {
#if defined(__WIN32) || defined(__WIN64)
                strcpy(sma_transport,"serial");
                strcpy(sma_target,"COM2");
                strcpy(sma_topts,"9600");
#else
		strcpy(sma_transport,"serial");
		strcpy(sma_target,"/dev/ttyS0");
		strcpy(sma_topts,"19200");
#endif
        }

	tp = tp_handle = 0;
	if (source == 1) {
		/* Load the smanet driver */
		dprintf(1,"sma_transport: %s, sma_target: %s, sma_topts: %s\n", sma_transport, sma_target, sma_topts);
		tp = find_driver(transports,sma_transport);
		if (!tp) return 1;
		tp_handle = tp->new(0,sma_target,sma_topts);
		if (!tp_handle) return 1;

		s->smanet = smanet_init(tp, tp_handle);
		if (!s->smanet) return 1;
	}

	if (start_flag) return si_startstop(s,1);
	else if (stop_flag) return si_startstop(s,0);

	if (s->smanet) {
		if (strlen(chanpath)) smanet_set_chanpath(s->smanet,chanpath);
		if (smanet_load_channels(s->smanet) != 0) {
			dprintf(1,"count: %d\n", list_count(s->smanet->channels));
			if (!list_count(s->smanet->channels)) {
				log_write(LOG_INFO,"Downloading channels...\n");
				smanet_read_channels(s->smanet);
				smanet_save_channels(s->smanet);
			}
		}
	}
#if 0

	outfile = 0;
	if (outfile) {
		dprintf(1,"outfile: %s\n", outfile);
		outfp = fopen(outfile,"w+");
		if (!outfp) {
			perror("fopen outfile");
			return 1;
		}
	} else {
#endif
		outfp = fdopen(1,"w");
#if 0
	}
	dprintf(1,"outfp: %p\n", outfp);
#endif

#if 0
	if (outfmt == 2) {
		root_value = json_value_init_object();
		root_object = json_value_get_object(root_value);
	}
#endif

	dprintf(1,"action: %d\n", action);
	switch(action) {
	case ACTION_FILE:
		{
			FILE *fp;
			char line[256],label[64],value[128];
			double d,dv;
			char *text;


			fp = fopen(filename,"r");
			if (!fp) {
				log_syserror("fopen(%s,r)",filename);
				return 1;
			}
			while(fgets(line,sizeof(line)-1,fp)) {
				dprintf(1,"line: %s\n", line);
				if (*line == '#') continue;
				strncpy(label,strele(0," ",line),sizeof(label)-1);
				strncpy(value,strele(1," ",line),sizeof(value)-1);
				dprintf(1,"label: %s, value: %s\n", label, value);
				if (smanet_get_value(s->smanet,label,&d,&text)) {
					log_write(LOG_ERROR,"%s: error getting value\n",label);
					return 1;
				}
				if (strlen(value)) {
					if (!text) {
						dv = atof(value);
						dprintf(1,"dv: %f, d: %f\n", dv, d);
						if (dv < d || dv > d) {
							if (ver_flag) printf("%s %f != %f\n",label,d,dv);
							else if (smanet_set_value(s->smanet,label,dv,0))
								printf("%s: error setting value: %f\n", label, dv);
							else if (!quiet_flag) printf("%s %f -> %f\n",label,d,dv);
						}
					} else {
						dprintf(1,"value: %s, text: %s\n", value, text);
						if (strcmp(value,text) != 0) {
							if (ver_flag) printf("%s %s != %s\n",label,text,value);
							else if (smanet_set_value(s->smanet,label,0,value))
								printf("%s: error setting value: %f\n", label, dv);
							else if (!quiet_flag) printf("%s %s -> %s\n",label,text,value);
						}
					}
				} else {
					if (text) printf("%s %s\n",label,text);
					else if ((int)d == d) printf("%s %d\n",label,(int)d);
					else printf("%s %f\n",label,d);
				}
			}
			fclose(fp);
		}
		break;
	case ACTION_INFO:
		memset(&info,0,sizeof(info));
		si_driver.open(s);
		if (si_get_info(s,&info)) return 1;
		display_info(&info);
		break;
	case ACTION_LIST:
		if (argc > 0) {
			char *p;

			dprintf(1,"argv[0]: %s\n", argv[0]);
			c = smanet_get_channel(s->smanet,argv[0]);
			if (!c) {
				log_error("%s: channel not found\n",argv[0]);
				return 1;
			}
			list_reset(c->strings);
			while((p = list_get_next(c->strings)) != 0) printf("%s\n", p);
		} else {
			list_reset(s->smanet->channels);
			while((c = list_get_next(s->smanet->channels)) != 0) printf("%s\n",c->name);
		}
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

	dprintf(1,"done!\n");
	return 0;
}

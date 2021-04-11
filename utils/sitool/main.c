
/*
Copyright (c) 2021, Stephen P. Shoecraft
All rights reserved.

This source code is licensed under the BSD-style license found in the
LICENSE file in the root directory of this source tree.
*/

struct mybmm_config;
typedef struct mybmm_config mybmm_config_t;

#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <stdarg.h>
#include <errno.h>
#include <ctype.h>
#include <dlfcn.h>
//#include "util.h"
//#include "parson.h"
#include "mybmm.h"
#include "inverter.h"
#include "si_info.h"
#include <linux/can.h>

int debug = 0;

int outfmt = 0;
FILE *outfp;
char sepch;
char *sepstr;

//JSON_Value *root_value;
//JSON_Object *root_object;
//char *serialized_string = NULL;

#define _getshort(p) (short)(*(p) | (*((p)+1) << 8))
#define _getint(p) (int)(*(p) | (*((p)+1) << 8) | (*((p)+2) << 16) | (*((p)+3) << 24))

enum SI_PARM_DT {
	SI_PARM_DT_UNK,
	SI_PARM_DT_INT,			/* Std int/number */
	SI_PARM_DT_FLOAT,		/* floating pt */
	SI_PARM_DT_STR,			/* string */
	SI_PARM_DT_TEMP,		/* temp */
	SI_PARM_DT_DATE,		/* date */
	SI_PARM_DT_PCT,			/* % */
	SI_PARM_DT_FUNC,		/* function bits */
	SI_PARM_DT_NTC,			/* ntc bits */
	SI_PARM_DT_B0,			/* byte 0 */
	SI_PARM_DT_B1,			/* byte 1 */
};

struct si_params {
	uint8_t reg;
	char *label;
	int dt;
} params[] = {
	{ 0,0,0 }
};
typedef struct si_params si_params_t;

struct si_params *_getp(char *label) {
	register struct si_params *pp;

	dprintf(3,"label: %s\n", label);
	for(pp = params; pp->label; pp++) {
		dprintf(3,"pp->label: %s\n", pp->label);
		if (strcmp(pp->label,label)==0) {
			return pp;
		}
	}
	return 0;
}

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

#if 0
void disp(char *label, int dt, ...) {
	va_list ap;

	dprintf(3,"label: %s, dt: %d\n", label, dt);

	va_start(ap,dt);
	switch(dt) {
	default:
	case SI_PARM_DT_INT:
		_dint(label,va_arg(ap,int));
		break;
	case SI_PARM_DT_FLOAT:
		_dfloat(label,va_arg(ap,double));
		break;
	case SI_PARM_DT_STR:
		_dstr(label,va_arg(ap,char *));
		break;
	case SI_PARM_DT_TEMP:
		_dint(label,va_arg(ap,int));
		break;
	case SI_PARM_DT_DATE:
		_dint(label,va_arg(ap,int));
		break;
	case SI_PARM_DT_FUNC:
		_dint(label,va_arg(ap,int));
		break;
	case SI_PARM_DT_NTC:
		_dint(label,va_arg(ap,int));
		break;
        case SI_PARM_DT_B0:
		_dint(label,va_arg(ap,int));
		break;
        case SI_PARM_DT_B1:
		_dint(label,va_arg(ap,int));
		break;
	}
}

void pdisp(char *label, int dt, uint8_t *data, int len) {
	dprintf(3,"label: %s, dt: %d\n", label, dt);
	switch(dt) {
	case SI_PARM_DT_INT:
	case SI_PARM_DT_TEMP:
	case SI_PARM_DT_DATE:
	case SI_PARM_DT_PCT:
	case SI_PARM_DT_FUNC:
	case SI_PARM_DT_NTC:
		_dint(label,(int)_getshort(data));
		break;
	case SI_PARM_DT_B0:
		_dint(label,data[0]);
		break;
	case SI_PARM_DT_B1:
		_dint(label,data[1]);
		break;
	case SI_PARM_DT_FLOAT:
		_dfloat(label,(float)_getshort(data));
		break;
	case SI_PARM_DT_STR:
		data[len] = 0;
		trim((char *)data);
		_dstr(label,(char *)data);
		break;
	}
}
#endif

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
		printf("%s,%s\n",label,str);
		break;
	default:
		printf("%-25s %s\n",label,str);
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
		printf("%s,%s\n",label,str);
		break;
	default:
		printf("%-25s %s\n",label,str);
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

int init_inv(mybmm_inverter_t *inv, mybmm_config_t *c, char *type, char *transport, char *target, char *opts, mybmm_module_t *cp, mybmm_module_t *tp) {
	memset(inv,0,sizeof(*inv));
	strcpy(inv->type,type);
	if (transport) strcpy(inv->transport,transport);
	if (target) strcpy(inv->target,target);
//	if (opts) strcpy(inv->opts,opts);
        inv->open = cp->open;
        inv->read = cp->read;
        inv->close = cp->close;
        inv->handle = cp->new(c,inv,tp);
	dprintf(1,"handle: %p\n", inv->handle);
        return 0;
}

int si_start(si_session_t *s) {
	int retries;
	unsigned char data[8];
	struct can_frame wr_frame, rd_frame;

	memset(&wr_frame,0,sizeof(wr_frame));
	wr_frame.can_id = 0x35c;
	wr_frame.can_dlc = 1;
	wr_frame.data[0] = 0x01;

	memset(&rd_frame,0,sizeof(rd_frame));
	rd_frame.can_id = 0x307;

	retries=10;
	while(retries--) {
		dprintf(1,"writing...\n");
		s->tp->write(s->tp_handle,&wr_frame,sizeof(wr_frame));
		dprintf(1,"reading...\n");
		if (s->get_data(s,0x307,data,8) == 0) {
			if (debug >= 3) bindump("data",data,8);
			if (data[3] & 0x0002) return 0;
		}
		sleep(1);
	}
	if (retries < 0) printf("start failed.\n");
	return (retries < 0 ? 1 : 0);
}

int si_stop(si_session_t *s) {
	int retries;
	unsigned char data[8];
	struct can_frame wr_frame;

	memset(&wr_frame,0,sizeof(wr_frame));
	wr_frame.can_id = 0x35c;
	wr_frame.can_dlc = 1;
	wr_frame.data[0] = 0x02;

	retries=10;
	while(retries--) {
		s->tp->write(s->tp_handle,&wr_frame,sizeof(wr_frame));
		if (s->get_data(s,0x307,data,8) == 0) {
			if (debug >= 3) bindump("data",data,8);
			if ((data[3] & 0x0002) == 0) return 0;
		}
		sleep(1);
	}
	if (retries < 0) printf("stop failed.\n");
	return (retries < 0 ? 1 : 0);
}

void usage(char *name) {
	printf("usage: %s [-acjJrwlh] [-f filename] [-b <bluetooth mac addr | -i <ip addr>] [-o output file]\n",name);
	printf("arguments:\n");
#ifdef DEBUG
	printf("  -d <#>		debug output\n");
#endif
	printf("  -j		JSON output\n");
	printf("  -J		JSON output pretty print\n");
	printf("  -s		Start SI\n");
	printf("  -S		Stop SI\n");
	printf("  -h		this output\n");
	printf("  -o <filename>	output filename\n");
	printf("  -t <transport:target> transport & target\n");
	printf("  -e <opts>	transport-specific opts\n");
}

enum SITOOL_ACTION {
	SITOOL_ACTION_INFO=0,
	SITOOL_ACTION_START,
	SITOOL_ACTION_STOP
};

int main(int argc, char **argv) {
	int opt,action;
	char *transport,*target,*topts,*outfile,*opts;
	mybmm_config_t *conf;
	mybmm_module_t *cp,*tp;
	si_info_t info;
	mybmm_inverter_t inv;
	si_session_t *s;

	outfmt = 0;
	sepch = ',';
	sepstr = ",";
	action = SITOOL_ACTION_INFO;
	transport = target = topts = outfile = opts = 0;
	while ((opt=getopt(argc, argv, "d:sSt:e:jJo:h")) != -1) {
		switch (opt) {
		case 'd':
			debug = atoi(optarg);
			break;
                case 't':
			transport = optarg;
			target = strchr(transport,':');
			if (!target) {
				printf("error: format is transport:target\n");
				usage(argv[0]);
				return 1;
			}
			*target = 0;
			target++;
			break;
		case 'e':
			opts = optarg;
			break;
#if 0
		case 'j':
			outfmt=2;
			pretty = 0;
			break;
		case 'J':
			outfmt=2;
			pretty = 1;
			break;
#endif
		case 's':
			action = SITOOL_ACTION_START;
			break;
		case 'S':
			action = SITOOL_ACTION_STOP;
			break;
		case 'h':
		default:
			usage(argv[0]);
			exit(0);
                }
        }
	dprintf(1,"transport: %p, target: %p\n", transport, target);
	if (!transport) transport = "can";
	if (!target) target = "can0,500000";

        argc -= optind;
        argv += optind;
        optind = 0;

#if 0
	if ((action == SITOOL_ACTION_READ || action == SITOOL_ACTION_WRITE) && !filename && !argc && !all && !reg && !dump) {
		printf("error: a filename or parameter name or all (a) must be specified.\n");
		usage(argv[0]);
		return 1;
	}
#endif

	conf = calloc(sizeof(*conf),1);
	if (!conf) {
		perror("calloc conf");
		return 1;
	}
	conf->modules = list_create();

	dprintf(2,"transport: %s\n", transport);

	tp = mybmm_load_module(conf,transport,MYBMM_MODTYPE_TRANSPORT);
	if (!tp) return 1;
	cp = mybmm_load_module(conf,"si",MYBMM_MODTYPE_INVERTER);
	if (!cp) return 1;

	/* Init the inverter */
	if (init_inv(&inv,conf,"si",transport,target,opts,cp,tp)) return 1;
	s = inv.handle;

	if (outfile) {
		dprintf(1,"outfile: %s\n", outfile);
		outfp = fopen(outfile,"w+");
		if (!outfp) {
			perror("fopen outfile");
			return 1;
		}
	} else {
		outfp = fdopen(1,"w");
	}
	dprintf(1,"outfp: %p\n", outfp);

#if 0
	if (outfmt == 2) {
		root_value = json_value_init_object();
		root_object = json_value_get_object(root_value);
	}
#endif

	switch(action) {
	case SITOOL_ACTION_INFO:
		if (inv.open(inv.handle)) return 1;
		si_get_info(s,&info);
		display_info(&info);
		inv.close(inv.handle);
		break;
	case SITOOL_ACTION_START:
		if (inv.open(inv.handle)) return 1;
		si_start(s);
		inv.close(inv.handle);
		break;
	case SITOOL_ACTION_STOP:
		if (inv.open(inv.handle)) return 1;
		si_stop(s);
		inv.close(inv.handle);
		break;
	}
#if 0
	if (outfmt == 2) {
		if (pretty)
	    		serialized_string = json_serialize_to_string_pretty(root_value);
		else
    			serialized_string = json_serialize_to_string(root_value);
		fprintf(outfp,"%s",serialized_string);
		json_free_serialized_string(serialized_string);
		json_value_free(root_value);
	}
#endif
	fclose(outfp);

	return 0;
}

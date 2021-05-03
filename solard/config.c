
/*
Copyright (c) 2021, Stephen P. Shoecraft
All rights reserved.

This source code is licensed under the BSD-style license found in the
LICENSE file in the root directory of this source tree.
*/

#include "solard.h"
#include <sys/signal.h>

int solard_read_config(solard_config_t *conf) {
	solard_agentinfo_t newinfo;
	char temp[16], *sname, *p;
	int i,count;
	cfg_proctab_t mytab[] = {
		{ "solard", "agent_error", 0, DATA_TYPE_INT, &conf->agent_error, 0, "300" },
		{ "solard", "agent_warning", 0, DATA_TYPE_INT, &conf->agent_warning, 0, "120" },
		{ "solard", "interval", "Agent check interval", DATA_TYPE_INT, &conf->interval, 0, "15" },
		CFG_PROCTAB_END
	};

	cfg_get_tab(conf->c->cfg,mytab);
	if (debug) cfg_disp_tab(mytab,"info",0);

	/* Get agent count */
	p = cfg_get_item(conf->c->cfg, "agents", "count");
	if (p) {
		/* Get agents */
		count = atoi(p);
		for(i=0; i < count; i++) {
			sprintf(temp,"A%02d",i);
			dprintf(1,"num temp: %s\n",temp);
			sname = cfg_get_item(conf->c->cfg,"agents",temp);
			if (sname) {
				memset(&newinfo,0,sizeof(newinfo));
				agentinfo_getcfg(conf->c->cfg, sname, &newinfo);
				agentinfo_add(conf,&newinfo);
			}
		}
	}
	return 0;
}

int solard_write_config(solard_config_t *conf) {
	solard_agentinfo_t *info;
	char temp[16];
	int i;

	dprintf(1,"writing config...\n");

	if (!conf->c->cfg) {
		char configfile[256];
		sprintf(configfile,"%s/solard.conf",SOLARD_ETCDIR);
		dprintf(1,"configfile: %s\n", configfile);
		conf->c->cfg = cfg_create(configfile);
		if (!conf->c->cfg) return 1;
	}

	i = 0;
	dprintf(1,"agents count: %d\n", list_count(conf->agents));
	list_reset(conf->agents);
	while((info = list_get_next(conf->agents)) != 0) {
		agentinfo_setcfg(conf->c->cfg,info->id,info);
		sprintf(temp,"A%02d",i++);
		dprintf(1,"num temp: %s\n",temp);
		cfg_set_item(conf->c->cfg, "agents", temp, 0, info->id);
	}
	sprintf(temp,"%d",list_count(conf->agents));
	dprintf(1,"count temp: %s\n",temp);
	cfg_set_item(conf->c->cfg, "agents", "count", 0, temp);
	cfg_write(conf->c->cfg);
	return 0;
}

#if 0
static json_descriptor_t battery_parms[] = {
	{ "batteries", DATA_TYPE_LIST, 0, 0, 0, 0, 0, 0, 0, 0 },
	{ "min_voltage", DATA_TYPE_FLOAT, "range", 3, (float []){ SI_VOLTAGE_MIN, SI_VOLTAGE_MAX, .1 }, 1, (char *[]){ "Min Battery Voltage" }, "V", 1, "%2.1f" },
	{ "charge_start_voltage", DATA_TYPE_FLOAT, "range", 3, (float []){ SI_VOLTAGE_MIN, SI_VOLTAGE_MAX, .1 }, 1, (char *[]){ "Charge start voltage" }, "V", 1, "%2.1f" },
	{ "charge_end_voltage", DATA_TYPE_FLOAT, "range", 3, (float []){ SI_VOLTAGE_MIN, SI_VOLTAGE_MAX, .1 }, 1, (char *[]){ "Charge end voltage" }, "V", 1, "%2.1f" },
	{ "charge_amps", DATA_TYPE_FLOAT, "range", 3, (float []){ 0, 5000, .1 }, 1, (char *[]){ "Charge Amps" }, "A", 1, "%.1f" },
	{ "charge_mode", DATA_TYPE_INT, "select", 3, (int []){ 0, 1, 2 }, 1, (char *[]){ "Off","CC","CV" }, 0, 1, 0 },
	{ "discharge_amps", DATA_TYPE_FLOAT, "range", 3, (float []){ 0, 5000, .1 }, 1, (char *[]){ "Discharge Amps" }, "A", 1, "%.1f" },
	{ "charge_min_amps", DATA_TYPE_FLOAT, "range", 3, (float []){ 0, 5000, .1 }, 1, (char *[]){ "Minimum charge amps" }, "A", 1, "%.1f" },
	{ "charge_focus_amps", DATA_TYPE_BOOL, 0, 0, 0, 1, (char *[]){ "Increase voltage to maintain charge amps" }, 0, 0 },
	{ "sim_step", DATA_TYPE_FLOAT, "range", 3, (float []){ 0, 5000, .1 }, 1, (char *[]){ "SIM Voltage Step" }, "V", 1, "%.1f" },
	{ "interval", DATA_TYPE_INT, "range", 3, (int []){ 0, 10000, 1 }, 0, 0, 0, 1, 0 },
};
#define NCHG (sizeof(charge_params)/sizeof(json_descriptor_t))

#if 0
static json_descriptor_t other_params[] = {
	{ "SenseResistor", DATA_TYPE_FLOAT, 0, 0, 0, 1, (char *[]){ "Current Res" }, "mR", 10, "%.1f" },
	{ "PackNum", DATA_TYPE_INT, "select",
		28, (int []){ 3,4,5,6,7,8,9,10,11,12,13,14,15,16,17,18,19,20,21,22,23,24,25,26,27,28,29,30 },
		1, (char *[]){ "mR PackNum" }, 0, 0 },
	{ "CycleCount", DATA_TYPE_INT, 0, 0, 0, 0, 0, 0, 0 },
	{ "SerialNumber", DATA_TYPE_INT, 0, 0, 0, 0, 0, 0, 0 },
	{ "ManufacturerName", DATA_TYPE_STRING, 0, 0, 0, 0, 0, 0, 0 },
	{ "DeviceName", DATA_TYPE_STRING, 0, 0, 0, 0, 0, 0, 0 },
	{ "ManufactureDate", DATA_TYPE_DATE, 0, 0, 0, 0, 0, 0, 0 },
	{ "BarCode", DATA_TYPE_STRING, 0, 0, 0, 0, 0, 0, 0 },
};
#define NOTHER (sizeof(other_params)/sizeof(json_descriptor_t))

static json_descriptor_t basic_params[] = {
	{ "CellOverVoltage", DATA_TYPE_FLOAT, "range", 3, (int []){ 0, 10000, 1 }, 1, (char *[]){ "COVP" }, "mV", 1000, "%.3f" },
	{ "CellOVRelease", DATA_TYPE_FLOAT, "range", 3, (int []){ 0, 10000, 1 }, 1, (char *[]){ "COVP Release" }, "mV", 1000, "%.3f" },
	{ "CellOVDelay", DATA_TYPE_INT, "range", 3, (int []){ 0, 999, 1 }, 1, (char *[]){ "COVP Delay" }, "S", 0 },
	{ "CellUnderVoltage", DATA_TYPE_FLOAT, "range", 3, (int []){ 0, 10000, 1 }, 1, (char *[]){ "CUVP" }, "mV", 1000, "%.3f" },
	{ "CellUVRelease", DATA_TYPE_FLOAT, "range", 3, (int []){ 0, 10000, 1 }, 1, (char *[]){ "CUVP Release" }, "mV", 1000, "%.3f" },
	{ "CellUVDelay", DATA_TYPE_INT, "range", 3, (int []){ 0, 999, 1 }, 1, (char *[]){ "CUVP Delay" }, "S", 0 },
	{ "PackOverVoltage", DATA_TYPE_FLOAT, "range", 3, (int []){ 0, 99999, 1 }, 1, (char *[]){ "POVP" }, "mV", 100, "%.1f" },
	{ "PackOVRelease", DATA_TYPE_FLOAT, "range", 3, (int []){ 0, 99999, 1 }, 1, (char *[]){ "POVP Release" }, "mV", 100, "%.1f" },
	{ "PackOVDelay", DATA_TYPE_INT, "range", 3, (int []){ 0, 999, 1 }, 1, (char *[]){ "POVP Delay" }, "S", 0 },
};
#endif
#endif

static struct parmdir {
	char *name;
	json_descriptor_t *parms;
	int count;
} allparms[] = {
#if 0
	{ "Charge parameters", charge_params, NCHG },
	{ "Function Configuration", func_params, NFUNC },
	{ "NTC Configuration", ntc_params, NNTC },
	{ "Balance Configuration", balance_params, NBAL },
	{ "GPS Configuration", gps_params, NBAL },
	{ "Other Configuration", other_params, NOTHER },
	{ "Basic Parameters", basic_params, NBASIC },
	{ "Hard Parameters", hard_params, NHARD },
	{ "Misc Parameters", misc_params, NMISC },
#endif
};
#define NALL (sizeof(allparms)/sizeof(struct parmdir))

json_descriptor_t *_getd(solard_config_t *s,char *name) {
	json_descriptor_t *dp;
	register int i,j;

	dprintf(1,"label: %s\n", name);

	for(i=0; i < NALL; i++) {
		dprintf(1,"section: %s\n", allparms[i].name);
		dp = allparms[i].parms;
		for(j=0; j < allparms[i].count; j++) {
			dprintf(1,"dp->name: %s\n", dp[j].name);
			if (strcmp(dp[j].name,name)==0)
				return &dp[j];
		}
	}

#if 0
	/* See if it's in our desc list */
	dprintf(1,"looking in desc list...\n");
	list_reset(s->desc);
	while((dp = list_get_next(s->desc)) != 0) {
		dprintf(1,"dp->name: %s\n", dp->name);
		if (strcmp(dp->name,name)==0)
			return dp;
	}
#endif

	return 0;
}

int solard_config_add_info(solard_config_t *s, json_value_t *j) {
	int x,y;
	json_value_t *ca,*o,*a;
	struct parmdir *dp;

	/* Configuration array */
	ca = json_create_array();

	for(x=0; x < NALL; x++) {
		o = json_create_object();
		dp = &allparms[x];
		if (dp->count > 1) {
			a = json_create_array();
			for(y=0; y < dp->count; y++) json_array_add_descriptor(a,dp->parms[y]);
			json_add_value(o,dp->name,a);
		} else if (dp->count == 1) {
			json_add_descriptor(o,dp->name,dp->parms[0]);
		}
		json_array_add_value(ca,o);
	}

	json_add_value(j,"configuration",ca);
	return 0;
}

#if 0
static json_proctab_t *_getinv(solard_config_t *s, char *name) {
//	solard_inverter_t *inv = s->ap->role_data;
	json_proctab_t params[] = {
#if 0
                { "max_voltage",DATA_TYPE_FLOAT,&inv->max_voltage,0 },
                { "min_voltage",DATA_TYPE_FLOAT,&inv->min_voltage,0 },
                { "charge_start_voltage",DATA_TYPE_FLOAT,&inv->charge_start_voltage,0 },
                { "charge_end_voltage",DATA_TYPE_FLOAT,&inv->charge_end_voltage,0 },
                { "charge_amps",DATA_TYPE_FLOAT,&inv->charge_amps,0 },
                { "charge_mode",DATA_TYPE_INT,&s->charge_mode,0 },
                { "discharge_amps",DATA_TYPE_FLOAT,&inv->discharge_amps,0 },
		{ "charge_min_amps", DATA_TYPE_FLOAT, &s->charge_min_amps,0 },
		{ "charge_focus_amps", DATA_TYPE_BOOL, &s->charge_creep, 0 },
		{ "sim_step", DATA_TYPE_FLOAT, &s->sim_step, 0 },
		{ "interval", DATA_TYPE_INT, &s->interval, 0 },
#endif
		JSON_PROCTAB_END
	};
	json_proctab_t *invp;

	dprintf(1,"name: %s\n", name);

	for(invp=params; invp->field; invp++) {
		dprintf(1,"invp->field: %s\n", invp->field);
		if (strcmp(invp->field,name)==0) {
			dprintf(1,"found!\n");
//			s->idata = *invp;
//			return &s->idata;
		}
	}
	dprintf(1,"NOT found\n");
	return 0;
}
#endif

void *_getp(solard_config_t *s, char *name) {
#if 0
	json_proctab_t *invp;
//	solard_param_t *pinfo;
	smanet_channel_t *c;

	dprintf(1,"name: %s\n", name);

	pinfo = &s->pdata;
	memset(pinfo,0,sizeof(*pinfo));

	/* Is it from the inverter struct? */
	invp = _getinv(s,name);
	if (invp) {
		uint8_t *bptr;
		short *sptr;
		long *lptr;
		float *fptr;
		int *iptr;

		dprintf(1,"inv: name: %s, type: %d, ptr: %p, len: %d\n", invp->field, invp->type, invp->ptr, invp->len);
		dprintf(1,"invp->type: %d\n", invp->type);

		strncat(pinfo->name,invp->field,sizeof(pinfo->name)-1);
		pinfo->type = invp->type;
		pinfo->source = 1;
		switch(invp->type) {
		case DATA_TYPE_BOOL:
			iptr = invp->ptr;
			pinfo->bval = *iptr;
			break;
		case DATA_TYPE_BYTE:
			bptr = invp->ptr;
			pinfo->bval = *bptr;
			break;
		case DATA_TYPE_SHORT:
			sptr = invp->ptr;
			pinfo->wval = *sptr;
			break;
		case DATA_TYPE_LONG:
			lptr = invp->ptr;
			pinfo->lval = *lptr;
			break;
		case DATA_TYPE_FLOAT:
			fptr = invp->ptr;
			pinfo->fval = *fptr;
			break;
		default: break;
		}
		dprintf(1,"found\n");
		return pinfo;
	}

	/* Is it from the SI? */
	if (!s->smanet) goto _getp_done;
	dprintf(1,"NOT found, checking smanet\n");
	c = smanet_get_channelbyname(s->smanet,name);
	if (c) {
		smanet_value_t *v;

		v = smanet_get_value(s->smanet,c);
		strncat(pinfo->name,c->name,sizeof(pinfo->name)-1);
		pinfo->source = 2;
		if (c->mask & CH_STATUS) {
			pinfo->type = DATA_TYPE_STRING;
			smanet_get_optionbyname(s->smanet,c->name,pinfo->sval,sizeof(pinfo->sval));
		} else {
			pinfo->type = v->type;
			switch(v->type) {
			case DATA_TYPE_BYTE:
				pinfo->bval = v->bval;
				break;
			case DATA_TYPE_SHORT:
				pinfo->wval = v->wval;
				break;
			case DATA_TYPE_LONG:
				pinfo->lval = v->lval;
				break;
			case DATA_TYPE_FLOAT:
				pinfo->fval = v->fval;
				break;
			default: break;
			}
		}
		dprintf(1,"found\n");
		return pinfo;
	}

_getp_done:
	dprintf(1,"NOT found\n");
#endif
	return 0;
}


static int solard_get_config(solard_config_t *s, void *pp, json_descriptor_t *dp) {
#if 0
	char topic[200];
	char temp[72];
	unsigned char bval;
	short wval;
	float fval;
	long lval;

	dprintf(1,"dp: name: %s, unit: %s, scale: %f\n", dp->name, dp->units, dp->scale);

	temp[0] = 0;
	dprintf(1,"pp->type: %d\n", pp->type);
	switch(pp->type) {
	case DATA_TYPE_BOOL:
		dprintf(1,"bval: %d\n", pp->bval);
		sprintf(temp,"%s",pp->bval ? "true" : "false");
		break;
	case DATA_TYPE_BYTE:
		bval = pp->bval;
		dprintf(1,"bval: %d\n", bval);
		dprintf(1,"scale: %f\n", dp->scale);
		if (dp->scale != 0.0) bval /= dp->scale;
		dprintf(1,"format: %s\n", dp->format);
		if (dp->format) sprintf(temp,dp->format,bval);
		else sprintf(temp,"%d",bval);
		break;
	case DATA_TYPE_SHORT:
		wval = pp->wval;
		dprintf(1,"wval: %d\n", wval);
		dprintf(1,"scale: %f\n", dp->scale);
		if (dp->scale != 0.0) wval /= dp->scale;
		dprintf(1,"format: %s\n", dp->format);
		if (dp->format) sprintf(temp,dp->format,wval);
		else sprintf(temp,"%d",wval);
		break;
	case DATA_TYPE_LONG:
		lval = pp->lval;
		dprintf(1,"ival: %d\n", lval);
		dprintf(1,"scale: %f\n", dp->scale);
		if (dp->scale != 0.0) lval /= dp->scale;
		dprintf(1,"format: %s\n", dp->format);
		if (dp->format) sprintf(temp,dp->format,lval);
		else sprintf(temp,"%ld",lval);
		break;
	case DATA_TYPE_FLOAT:
		fval = pp->fval;
		dprintf(1,"fval: %d\n", fval);
		dprintf(1,"scale: %f\n", dp->scale);
		if (dp->scale != 0.0) fval /= dp->scale;
		if (dp->format) sprintf(temp,dp->format,fval);
		else sprintf(temp,"%f",fval);
		break;
	case DATA_TYPE_STRING:
		temp[0] = 0;
		strncat(temp,pp->sval,sizeof(temp)-1);
		trim(temp);
		break;
	default:
		sprintf(temp,"unhandled switch for: %d",dp->type);
		break;
	}
	sprintf(topic,"%s/%s/%s/%s/%s",SOLARD_TOPIC_ROOT,s->ap->role->name,s->ap->instance_name,SOLARD_FUNC_CONFIG,pp->name);
	dprintf(1,"topic: %s, temp: %s\n", topic, temp);
	return mqtt_pub(s->ap->m,topic,temp,1);
#endif
	return 1;
}

static solard_agentinfo_t *agent_infobyname(solard_config_t *s, char *name) {
	register solard_agentinfo_t *info;

	list_reset(s->agents);
	while((info = list_get_next(s->agents)) != 0) {
		if (strcmp(info->name,name)==0)
			return info;
	}
	return 0;
}

static int solard_set_config(solard_config_t *s, json_value_t *v) {
	json_value_t *ov;
	json_object_t *o;
	char name[64];
	solard_agentinfo_t *info;
	int i,name_changed;
	char *p,*value;
	list lp;

	o = v->value.object;
	dprintf(1,"count: %d\n", o->count);
	for(i=0; i < o->count; i++) {
		ov = o->values[i];
		dprintf(1,"name: %s, type: %d (%s)\n", o->names[i], ov->type, json_typestr(ov->type));
		if (ov->type != JSONString) return 1;
		value = (char *)json_string(ov);
		dprintf(1,"value: %s\n", value);
		p = strchr(value,':');
		if (!p) return 1;
		*p = 0;
		/* Need to use direct ptrs as value may contain : */
		strncpy(name,value,sizeof(name)-1);
		dprintf(1,"name: %s\n", name);
		info = agent_infobyname(s,name);
		dprintf(1,"info: %p\n", info);
		if (!info) return 1;
		value = p+1;
		dprintf(1,"value: %s\n", value);
		conv_type(DATA_TYPE_LIST,&lp,0,DATA_TYPE_STRING,value,strlen(value));
		dprintf(1,"lp: %p\n", lp);
		if (!lp) return 1;
		dprintf(1,"count: %d\n", list_count(lp));
		name_changed = 0;
		list_reset(lp);
		while((p = list_get_next(lp)) != 0) {
			strncpy(name,strele(0,"=",p),sizeof(name)-1);
			value = strele(1,"=",p);
			dprintf(1,"name: %s, value: %s\n", name, value);
			cfg_set_item(s->c->cfg,info->id,name,0,value);
			if (strcmp(name,"name")==0) name_changed = 1;
		}
		dprintf(1,"name_changed: %d\n", name_changed);
		if (name_changed) {
			/* if we dont manage it, then cant change the name */
			if (!info->managed) return 1;
			if (info->pid > 0) {
				log_info("Killing %s/%s due to name change\n", info->role, info->name);
				kill(info->pid,SIGTERM);
			}
		}
		if (cfg_is_dirty(s->c->cfg)) {
			/* getcfg writes sname into id, get a local copy */
			strncpy(name,info->id,sizeof(name)-1);
			agentinfo_getcfg(s->c->cfg,name,info);
			solard_write_config(s);
		}
	}
	return 0;
#if 0
	char temp[72];

	dprintf(1,"dp: name: %s, unit: %s, scale: %f\n", dp->name, dp->units, dp->scale);

	/* We can use any field in the union as a source or dest */
	if (pp->source == SI_PARAM_SOURCE_INV) {
		json_proctab_t *invp;

		/* Update the inv struct directly */
		invp = _getinv(s,pp->name);
		conv_type(invp->type,invp->ptr,0,req->type,&req->sval,0);

		/* ALSO update this value in our config file */
		conv_type(DATA_TYPE_STRING,temp,sizeof(temp)-1,req->type,&req->sval,0);
		cfg_set_item(s->ap->cfg,s->ap->section_name,invp->field,"",temp);
		cfg_write(s->ap->cfg);
	} else { 
		smanet_value_t v;

		/* Update the SI */
		dprintf(1,"pp->type: %s\n", typestr(pp->type));
		if (pp->type == DATA_TYPE_STRING) {
			smanet_channel_t *c;
			int val;

			c = smanet_get_channelbyname(s->smanet, req->name);
			if (!c) return 1;
			val = smanet_get_optionval(s->smanet,c,req->sval);
			if (val < 0) return 1;
			v.type = DATA_TYPE_BYTE;
			v.bval = val;
		} else {
			v.type = pp->type;
			conv_type(pp->type,&v.bval,0,req->type,&req->bval,0);
		}
		smanet_set_valuebyname(s->smanet,pp->name,&v);
	}
	/* Re-get the param to update internal vars and publish */
	return solard_get_config(s,_getp(s,dp->name),dp);
#endif
}

int solard_config(solard_config_t *conf, solard_message_t *msg) {
	json_value_t *v;
	json_object_t *o;
	int i;
	json_descriptor_t *dp;
	void *pp;

	v = json_parse(msg->data);
	if (!v) return 1;
	pp = 0;
	dp = 0;
	if (strcmp(msg->action,"Get") == 0) {
		if (solard_get_config(conf,pp,dp)) return 1;
	} else if (strcmp(msg->action,"Set") == 0) {
		dprintf(1,"type: %d (%s)\n", v->type, json_typestr(v->type));
		if (solard_set_config(conf,v)) return 1;
	} else if (strcmp(msg->action,"Add") == 0) {
		dprintf(1,"type: %d (%s)\n", v->type, json_typestr(v->type));
		/* Must be an object */
		if (v->type != JSONObject) return 1;
		o = v->value.object;
		for(i=0; i < o->count; i++) {
			dprintf(1,"name[%d]: %s\n", i, o->names[i]);
			add_agent(conf,o->names[i],o->values[i]);
		}
	}
	json_destroy(v);
#if 0
	solard_config_t *conf = handle;
	void *pp;
	char message[128],*p;
	int status;
	long start;

	dprintf(1,"op: %s\n", op);

	status = 1;

	start = mem_used();
	list_reset(lp);
	if (strcmp(op,"Get") == 0) {
		while((p = list_get_next(lp)) != 0) {
			dprintf(1,"p: %s\n", p);
			sprintf(message,"%s: not found",p);
#if 0
			pp = _getp(conf,p);
			if (!pp) goto solard_config_error;
			dprintf(1,"pp: name: %s, type: %d\n", pp->name, pp->type);
			dp = _getd(conf,p);
			if (!dp) goto solard_config_error;
			dprintf(1,"dp: name: %s, unit: %s, scale: %f\n", dp->name, dp->units, dp->scale);
			if (solard_get_config(s,pp,dp)) goto solard_config_error;
#endif
		}
	} else {
		while((req = list_get_next(lp)) != 0) {
			dprintf(1,"req: name: %s, type: %d\n", req->name, req->type);
			dprintf(1,"req value: %s\n", req->sval);
			sprintf(message,"%s: not found",req->name);
#if 0
			pp = _getp(s,req->name);
			if (!pp) goto solard_config_error;
			dp = _getd(s,req->name);
			if (!dp) goto solard_config_error;
#endif
			if (solard_set_config(s,pp,dp,req)) goto solard_config_error;
		}
	}

	status = 0;
	strcpy(message,"success");
solard_config_error:
	/* If the agent is solard_control, dont send status */
//	if (strcmp(id,"solard_control")!=0) agent_send_status(conf->ap, s->ap->name, "Config", op, id, status, message);
	dprintf(1,"used: %ld\n", mem_used() - start);
	return status;
#endif
	return 1;
}

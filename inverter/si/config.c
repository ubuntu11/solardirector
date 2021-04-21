
/*
Copyright (c) 2021, Stephen P. Shoecraft
All rights reserved.

This source code is licensed under the BSD-style license found in the
LICENSE file in the root directory of this source tree.
*/

#include "si.h"

static json_descriptor_t charge_params[] = {
	{ "charge_voltage", DATA_TYPE_FLOAT, "range", 3, (float []){ SI_VOLTAGE_MIN, SI_VOLTAGE_MAX, .1 }, 1, (char *[]){ "Charge Voltage" }, "V", 1, "%2.1f" },
	{ "charge_amps", DATA_TYPE_FLOAT, "range", 3, (float []){ 0, 5000, .1 }, 1, (char *[]){ "Charge Amps" }, "A", 1, "%.1f" },
	{ "charge_max_voltage", DATA_TYPE_FLOAT, "range", 3, (float []){ SI_VOLTAGE_MIN, SI_VOLTAGE_MAX, .1 }, 1, (char *[]){ "Charge Maximum Voltage" }, "V", 1, "%2.1f" },
	{ "charge_at_max", DATA_TYPE_BOOL, 0, 0, 0, 1, (char *[]){ "Always charge at max voltage" }, 0, 0 },
	{ "discharge_voltage", DATA_TYPE_FLOAT, "range", 3, (float []){ SI_VOLTAGE_MIN, SI_VOLTAGE_MAX, .1 }, 1, (char *[]){ "Discharge Voltage" }, "V", 1, "%2.1f" },
	{ "discharge_amps", DATA_TYPE_FLOAT, "range", 3, (float []){ 0, 5000, .1 }, 1, (char *[]){ "Discharge Amps" }, "A", 1, "%.1f" },
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

static struct parmdir {
	char *name;
	json_descriptor_t *parms;
	int count;
} allparms[] = {
	{ "Charge parameters", charge_params, NCHG },
#if 0
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

json_descriptor_t *_getd(si_session_t *s,char *name) {
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

	/* See if it's in our desc list */
	dprintf(1,"looking in desc list...\n");
	list_reset(s->desc);
	while((dp = list_get_next(s->desc)) != 0) {
		dprintf(1,"dp->name: %s\n", dp->name);
		if (strcmp(dp->name,name)==0)
			return dp;
	}

	return 0;
}

static void _addchans(si_session_t *s, json_value_t *ca) {
	smanet_session_t *ss = s->smanet;
	smanet_channel_t *c;
	json_value_t *o,*a;
	json_descriptor_t newd;
	float step;

//	o = json_create_object();

	list_reset(ss->channels);
	while((c = list_get_next(ss->channels)) != 0) {
		dprintf(1,"adding chan: %s\n", c->name);
		memset(&newd,0,sizeof(newd));
		newd.name = c->name;
		step = 1;
		switch(c->format & 0xf) {
		case CH_BYTE:
			newd.type = DATA_TYPE_BYTE;
			break;
		case CH_SHORT:
			newd.type = DATA_TYPE_SHORT;
			break;
		case CH_LONG:
			newd.type = DATA_TYPE_LONG;
			break;
		case CH_FLOAT:
			newd.type = DATA_TYPE_FLOAT;
			step = .1;
			break;
		case CH_DOUBLE:
			newd.type = DATA_TYPE_DOUBLE;
			step = .1;
			break;
		default:
			dprintf(1,"unknown type: %d\n", c->format & 0xf);
			break;
		}
		if (c->mask & CH_PARA && c->mask & CH_ANALOG) {
			float *values;

			newd.scope = "range";
			values = malloc(3*sizeof(float));
			values[0] = c->gain;
			values[1] = c->offset;
			values[2] = step;
			newd.nvalues = 3;
			newd.values = values;
			dprintf(1,"adding range: 0: %f, 1: %f, 2: %f\n", values[0], values[1], values[2]);
		} else if (c->mask & CH_DIGITAL) {
			char **labels,*p;
			int i;

			newd.nlabels = 2;
 			labels = malloc(newd.nlabels*sizeof(char *));
			labels[0] = c->txtlo;
			labels[1] = c->txthi;
			newd.labels = labels;
		} else if (c->mask & CH_STATUS && list_count(c->strings)) {
			char **labels,*p;
			int i;

			newd.scope = "select";
			newd.nlabels = list_count(c->strings);
 			labels = malloc(newd.nlabels*sizeof(char *));
			i = 0;
			list_reset(c->strings);
			while((p = list_get_next(c->strings)) != 0) labels[i++] = p;
			newd.labels = labels;
		}
		newd.units = c->unit;
		newd.scale = 1.0;
		json_array_add_descriptor(ca,newd);
		/* Also add to our desc list */
		list_add(s->desc,&newd,sizeof(newd));
	}
}

int si_config_add_info(si_session_t *s, json_value_t *j) {
	int x,y;
	json_value_t *ca,*o,*a;
	struct parmdir *dp;

	/* Configuration array */
	ca = json_create_array();

	/* Do we have a saved rs485 config? */
	dprintf(1,"smanet: %p\n", s->smanet);
	if (s->smanet) {
		dprintf(1,"channels_path: %s\n", s->channels_path);
		if (!strlen(s->channels_path) && s->ap->cfg) {
			char *p;

			p = cfg_get_item(s->ap->cfg,"config","channels_path");
			if (p) strncat(s->channels_path,p,sizeof(s->channels_path)-1);
		}
		if (!strlen(s->channels_path)) sprintf(s->channels_path,"%s/%s.dat",LIBDIR,s->smanet->type);
		if (smanet_load_channels(s->smanet,s->channels_path) == 0) _addchans(s,ca);
	}

	/* Add the inverter config */
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

static json_proctab_t *_getinv(si_session_t *s, char *name) {
	solard_inverter_t *inv = s->ap->role_data;
	json_proctab_t params[] = {
                { "charge_voltage",DATA_TYPE_FLOAT,&inv->charge_voltage,0 },
                { "charge_amps",DATA_TYPE_FLOAT,&inv->charge_amps,0 },
                { "charge_max_voltage",DATA_TYPE_FLOAT,&inv->charge_max_voltage,0 },
                { "charge_at_max",DATA_TYPE_BOOL,&inv->charge_at_max,0 },
                { "discharge_voltage",DATA_TYPE_FLOAT,&inv->discharge_voltage,0 },
                { "discharge_amps",DATA_TYPE_FLOAT,&inv->discharge_amps,0 },
		JSON_PROCTAB_END
	};
	json_proctab_t *invp;

	dprintf(1,"name: %s\n", name);

	for(invp=params; invp->field; invp++) {
		dprintf(1,"invp->field: %s\n", invp->field);
		if (strcmp(invp->field,name)==0) {
			dprintf(1,"found!\n");
			s->idata = *invp;
			return &s->idata;
		}
	}
	dprintf(1,"NOT found\n");
	return 0;
}

si_param_t *_getp(si_session_t *s, char *name) {
	json_proctab_t *invp;
	si_param_t *pinfo;
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

//		strncat(pinfo->name,invp->name,sizeof(pinfo->name)-1);
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

	dprintf(1,"NOT found\n");
	return 0;
}


static int si_get_config(si_session_t *s, si_param_t *pp, json_descriptor_t *dp) {
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
	sprintf(topic,"%s/%s/%s/%s/%s",SOLARD_TOPIC_ROOT,s->ap->role->name,s->ap->name,SOLARD_FUNC_CONFIG,pp->name);
	dprintf(1,"topic: %s, temp: %s\n", topic, temp);
	return mqtt_pub(s->ap->m,topic,temp,1);
}

static int si_set_config(si_session_t *s, si_param_t *pp, json_descriptor_t *dp, solard_confreq_t *req) {
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
	return si_get_config(s,_getp(s,dp->name),dp);
}

int si_config(void *handle, char *op, char *id, list lp) {
	si_session_t *s = handle;
	solard_confreq_t *req;
	si_param_t *pp;
	json_descriptor_t *dp;
	char message[128],*p;
	int status;
	long start;

	dprintf(1,"op: %s\n", op);

	status = 1;

	start = mem_used();
	list_reset(lp);
	if (strcmp(op,"Get") == 0) {
		while((p = list_get_next(lp)) != 0) {
			sprintf(message,"%s: not found",p);
			dprintf(1,"p: %s\n", p);
			pp = _getp(s,p);
			if (!pp) goto si_config_error;
			dprintf(1,"pp: name: %s, type: %d\n", pp->name, pp->type);
			dp = _getd(s,p);
			if (!dp) goto si_config_error;
			dprintf(1,"dp: name: %s, unit: %s, scale: %f\n", dp->name, dp->units, dp->scale);
			if (si_get_config(s,pp,dp)) goto si_config_error;
		}
	} else {
		while((req = list_get_next(lp)) != 0) {
			dprintf(1,"req: name: %s, type: %d\n", req->name, req->type);
			dprintf(1,"req value: %s\n", req->sval);
			sprintf(message,"%s: not found",req->name);
			pp = _getp(s,req->name);
			if (!pp) goto si_config_error;
			dp = _getd(s,req->name);
			if (!dp) goto si_config_error;
			if (si_set_config(s,pp,dp,req)) goto si_config_error;
		}
	}

	status = 0;
	strcpy(message,"success");
si_config_error:
	/* If the agent is si_control, dont send status */
	if (strcmp(id,"si_control")!=0) agent_send_status(s->ap, s->ap->name, "Config", op, id, status, message);
	dprintf(1,"used: %ld\n", mem_used() - start);
	return status;
}

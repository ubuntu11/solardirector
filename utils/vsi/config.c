
/*
Copyright (c) 2021, Stephen P. Shoecraft
All rights reserved.

This source code is licensed under the BSD-style license found in the
LICENSE file in the root directory of this source tree.
*/

#include "si.h"

extern solard_driver_t si_driver;

static json_descriptor_t charge_params[] = {
	{ "charge", DATA_TYPE_STRING, "select", 3, (char *[]){ "Start","Stop","Pause" }, 1, 0, 0, 1, 0 },
	{ "charge_mode", DATA_TYPE_INT, "select", 3, (int []){ 0, 1, 2 }, 1, (char *[]){ "Off","On","CV" }, 0, 1, 0 },
	{ "max_voltage", DATA_TYPE_FLOAT, "range", 3, (float []){ SI_VOLTAGE_MIN, SI_VOLTAGE_MAX, .1 }, 1, (char *[]){ "Max Battery Voltage" }, "V", 1, "%2.1f" },
	{ "min_voltage", DATA_TYPE_FLOAT, "range", 3, (float []){ SI_VOLTAGE_MIN, SI_VOLTAGE_MAX, .1 }, 1, (char *[]){ "Min Battery Voltage" }, "V", 1, "%2.1f" },
	{ "charge_start_voltage", DATA_TYPE_FLOAT, "range", 3, (float []){ SI_VOLTAGE_MIN, SI_VOLTAGE_MAX, .1 }, 1, (char *[]){ "Charge start voltage" }, "V", 1, "%2.1f" },
	{ "charge_end_voltage", DATA_TYPE_FLOAT, "range", 3, (float []){ SI_VOLTAGE_MIN, SI_VOLTAGE_MAX, .1 }, 1, (char *[]){ "Charge end voltage" }, "V", 1, "%2.1f" },
	{ "charge_amps", DATA_TYPE_FLOAT, "range", 3, (float []){ 0, 5000, .1 }, 1, (char *[]){ "Charge Amps" }, "A", 1, "%.1f" },
	{ "discharge_amps", DATA_TYPE_FLOAT, "range", 3, (float []){ 0, 5000, .1 }, 1, (char *[]){ "Discharge Amps" }, "A", 1, "%.1f" },
	{ "charge_min_amps", DATA_TYPE_FLOAT, "range", 3, (float []){ 0, 5000, .1 }, 1, (char *[]){ "Minimum charge amps" }, "A", 1, "%.1f" },
	{ "charge_at_max", DATA_TYPE_BOOL, 0, 0, 0, 1, (char *[]){ "Charge at max_voltage" }, 0, 0 },
	{ "charge_creep", DATA_TYPE_BOOL, 0, 0, 0, 1, (char *[]){ "Increase voltage to maintain charge amps" }, 0, 0 },
	{ "sim_step", DATA_TYPE_FLOAT, "range", 3, (float []){ 0, 5000, .1 }, 1, (char *[]){ "SIM Voltage Step" }, "V", 1, "%.1f" },
	{ "readonly", DATA_TYPE_BOOL, 0, 0, 0, 1, (char *[]){ "Do not update" }, 0, 0 },
	{ "interval", DATA_TYPE_INT, "range", 3, (int []){ 0, 10000, 1 }, 0, 0, 0, 1, 0 },
	{ "can_transport", DATA_TYPE_STRING, 0, 0, 0, 0, 0, 0, 1, 0 },
	{ "can_target", DATA_TYPE_STRING, 0, 0, 0, 0, 0, 0, 1, 0 },
	{ "can_topts", DATA_TYPE_STRING, 0, 0, 0, 0, 0, 0, 1, 0 },
	{ "smanet_transport", DATA_TYPE_STRING, 0, 0, 0, 0, 0, 0, 1, 0 },
	{ "smanet_target", DATA_TYPE_STRING, 0, 0, 0, 0, 0, 0, 1, 0 },
	{ "smanet_topts", DATA_TYPE_STRING, 0, 0, 0, 0, 0, 0, 1, 0 },
	{ "smanet_channels_path", DATA_TYPE_STRING, 0, 0, 0, 0, 0, 0, 1, 0 },
	{ "run", DATA_TYPE_BOOL, 0, 0, 0, 1, (char *[]){ "run state" }, 0, 0 },
};
#define NCHG (sizeof(charge_params)/sizeof(json_descriptor_t))

static struct parmdir {
	char *name;
	json_descriptor_t *parms;
	int count;
} allparms[] = {
	{ "Charge parameters", charge_params, NCHG },
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

	o = json_create_object();
	a = json_create_array();

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
			char **labels;

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
		json_array_add_descriptor(a,newd);
		/* Also add to our desc list */
		list_add(s->desc,&newd,sizeof(newd));
	}
	json_add_value(o,"Sunny Island configuration",a);
	json_array_add_value(ca,o);
}

int si_config_add_info(si_session_t *s, json_value_t *j) {
	int x,y;
	json_value_t *ca,*o,*a;
	struct parmdir *dp;

	/* Configuration array */
	ca = json_create_array();

	/* Add the inverter config */
	for(x=0; x < NALL; x++) {
		o = json_create_object();
		dp = &allparms[x];
		/* We only do sections */
//		if (dp->count > 1) {
			a = json_create_array();
			for(y=0; y < dp->count; y++) json_array_add_descriptor(a,dp->parms[y]);
			json_add_value(o,dp->name,a);
#if 0
		} else if (dp->count == 1) {
			json_add_descriptor(o,dp->name,dp->parms[0]);
		}
#endif
		json_array_add_value(ca,o);
	}

	/* Add SMANET channels */
	dprintf(1,"smanet: %p\n", s->smanet);
	if (s->smanet) {
		dprintf(1,"channels_path: %s\n", s->channels_path);
		if (!strlen(s->channels_path)) sprintf(s->channels_path,"%s/%s.dat",SOLARD_LIBDIR,s->smanet->type);
		fixpath(s->channels_path, sizeof(s->channels_path)-1);
		dprintf(1,"channels_path: %s\n", s->channels_path);
		smanet_set_chanpath(s->smanet, s->channels_path);
		if (smanet_load_channels(s->smanet) == 0) _addchans(s,ca);
	}

	json_add_value(j,"configuration",ca);
	return 0;
}

static json_proctab_t *_getinv(si_session_t *s, char *name) {
	solard_inverter_t *inv = s->ap->role_data;
	json_proctab_t params[] = {
		{ "charge_mode",DATA_TYPE_INT,&s->charge_mode,0,0 },
		{ "max_voltage",DATA_TYPE_FLOAT,&inv->max_voltage,0,0 },
		{ "min_voltage",DATA_TYPE_FLOAT,&inv->min_voltage,0,0 },
		{ "charge_start_voltage",DATA_TYPE_FLOAT,&inv->charge_start_voltage,0,0 },
		{ "charge_end_voltage",DATA_TYPE_FLOAT,&inv->charge_end_voltage,0,0 },
		{ "charge_amps",DATA_TYPE_FLOAT,&inv->charge_amps,0,0 },
		{ "discharge_amps",DATA_TYPE_FLOAT,&inv->discharge_amps,0,0 },
		{ "charge_min_amps", DATA_TYPE_FLOAT, &s->charge_min_amps,0,0 },
		{ "charge_at_max", DATA_TYPE_BOOL, &inv->charge_at_max, 0,0 },
		{ "charge_creep", DATA_TYPE_BOOL, &s->charge_creep, 0,0 },
		{ "sim_step", DATA_TYPE_FLOAT, &s->sim_step, 0,0 },
		{ "interval", DATA_TYPE_INT, &s->interval, 0,0 },
		{ "can_transport", DATA_TYPE_STRING, &s->can_transport, sizeof(s->can_transport)-1,0 },
		{ "can_target", DATA_TYPE_STRING, &s->can_target, sizeof(s->can_target)-1,0 },
		{ "can_topts", DATA_TYPE_STRING, &s->can_topts, sizeof(s->can_topts)-1,0 },
		{ "smanet_transport", DATA_TYPE_STRING, &s->smanet_transport, sizeof(s->smanet_transport)-1,0 },
		{ "smanet_target", DATA_TYPE_STRING, &s->smanet_target, sizeof(s->smanet_target)-1,0 },
		{ "smanet_topts", DATA_TYPE_STRING, &s->smanet_topts, sizeof(s->smanet_topts)-1,0 },
		{ "smanet_channels_path", DATA_TYPE_STRING, &s->channels_path, sizeof(s->channels_path)-1,0 },
		{ "run", DATA_TYPE_BOOL, &s->run_state, 0,0 },
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

si_param_t *_getp(si_session_t *s, char *name, int invonly) {
	json_proctab_t *invp;
	si_param_t *pinfo;

	dprintf(1,"name: %s, invonly: %d\n", name, invonly);

	pinfo = &s->pdata;
	memset(pinfo,0,sizeof(*pinfo));

	/* Is it from the inverter struct? */
	invp = _getinv(s,name);
	if (invp) {
		uint8_t *bptr;
		short *wptr;
		int *iptr;
		long *lptr;
		float *fptr;
		double *dptr;

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
			wptr = invp->ptr;
			pinfo->wval = *wptr;
			break;
		case DATA_TYPE_LONG:
			lptr = invp->ptr;
			pinfo->lval = *lptr;
			break;
		case DATA_TYPE_FLOAT:
			fptr = invp->ptr;
			pinfo->fval = *fptr;
			break;
		case DATA_TYPE_DOUBLE:
			dptr = invp->ptr;
			pinfo->dval = *dptr;
			break;
		case DATA_TYPE_STRING:
			pinfo->sval = invp->ptr;
			break;
		default: break;
		}
		dprintf(1,"found\n");
		return pinfo;
	}
	if (invonly) goto _getp_done;

	if (s->smanet) {
		json_descriptor_t *dp;
		double value;
		char *text;

		dprintf(1,"NOT found, checking smanet\n");

		dp = _getd(s,name);
		if (!dp) {
			dprintf(1,"_getd returned null!\n");
			goto _getp_done;
		}
		if (smanet_get_value(s->smanet, name, &value, &text)) goto _getp_done;
		dprintf(1,"val: %f, sval: %s\n", value, text);
		strncat(pinfo->name,name,sizeof(pinfo->name)-1);
		pinfo->source = 2;
		if (text) {
			pinfo->sval = text;
			pinfo->type = DATA_TYPE_STRING;
		} else {
			conv_type(dp->type,&pinfo->bval,0,DATA_TYPE_DOUBLE,&value,0);
			pinfo->type = dp->type;
		}
		return pinfo;
	}

_getp_done:
	dprintf(1,"NOT found\n");
	return 0;
}

static int si_get_config(si_session_t *s, si_param_t *pp, json_descriptor_t *dp) {
	char topic[200];
	char temp[72];
	unsigned char bval;
	short wval;
	long lval;
	float fval;
	double dval;

	dprintf(1,"dp: name: %s, unit: %s, scale: %f\n", dp->name, dp->units, dp->scale);

	temp[0] = 0;
	dprintf(1,"pp->type: %d (%s)\n", pp->type, typestr(pp->type));
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
		dprintf(1,"fval: %f\n", fval);
		dprintf(1,"scale: %f\n", dp->scale);
		if (dp->scale != 0.0) fval /= dp->scale;
		if (dp->format) sprintf(temp,dp->format,fval);
		else sprintf(temp,"%f",fval);
		break;
	case DATA_TYPE_DOUBLE:
		dval = pp->dval;
		dprintf(1,"dval: %lf\n", dval);
		dprintf(1,"scale: %f\n", dp->scale);
		if (dp->scale != 0.0) dval /= dp->scale;
		if (dp->format) sprintf(temp,dp->format,dval);
		else sprintf(temp,"%f",dval);
		break;
	case DATA_TYPE_STRING:
		temp[0] = 0;
		strncat(temp,pp->sval,sizeof(temp)-1);
		trim(temp);
		break;
	default:
		sprintf(temp,"unhandled switch for: %d",dp->type);
		return 1;
		break;
	}
	sprintf(topic,"%s/%s/%s/%s/%s",SOLARD_TOPIC_ROOT,s->ap->role->name,s->ap->name,SOLARD_FUNC_CONFIG,pp->name);
	dprintf(1,"topic: %s, temp: %s\n", topic, temp);
	return mqtt_pub(s->ap->m,topic,temp,1,1);
	return 0;
}

int si_config_smanet(void *h) {
	si_session_t *s = h;
	char transport[SOLARD_TRANSPORT_LEN];
	char target[SOLARD_TARGET_LEN];
	char topts[SOLARD_TOPTS_LEN];
	cfg_proctab_t smanet_tab[] = {
		{ s->ap->section_name, "smanet_transport", 0, DATA_TYPE_STRING, &transport, sizeof(transport)-1,"" },
		{ s->ap->section_name, "smanet_target", 0, DATA_TYPE_STRING, &target, sizeof(target)-1,"" },
		{ s->ap->section_name, "smanet_topts", 0, DATA_TYPE_STRING, &topts, sizeof(topts)-1,"" },
		{ s->ap->section_name, "smanet_channels_path", 0, DATA_TYPE_STRING, &s->channels_path, sizeof(s->channels_path)-1,"" },
		CFG_PROCTAB_END
	};

	cfg_get_tab(s->ap->cfg,smanet_tab);
	if (debug) cfg_disp_tab(smanet_tab,"smanet",0);
	if (strlen(transport) && strlen(target)) {
		solard_driver_t *tp;
		void *tp_handle;
		int i;

		tp = 0;
		for(i=0; si_transports[i]; i++) {
			tp = si_transports[i];
			dprintf(1,"type: %d\n", tp->type);
			if (tp->type != SOLARD_DRIVER_TRANSPORT) continue;
			dprintf(1,"tp->name: %s\n", tp->name);
			if (strcmp(tp->name,transport) == 0) {
				tp_handle = tp->new(s->ap, target, topts);
				if (tp_handle) {
					s->smanet = smanet_init(tp,tp_handle);
				} else {
					log_write(LOG_ERROR,"error creating new %s transport instance", transport);
					return 1;
				}
			}
		}
		if (!tp) {
			log_write(LOG_ERROR,"unable to load smanet transport: %s",transport);
			return 1;
		}
	} else {
		log_write(LOG_INFO,"smanet transport or target null, skipping smanet init");
		return 1;
	}

	/* Load channels */
	if (!strlen(s->channels_path)) sprintf(s->channels_path,"%s/%s.dat",SOLARD_LIBDIR,s->smanet->type);
	smanet_set_chanpath(s->smanet,s->channels_path);
	if (access(s->channels_path,0) == 0) smanet_load_channels(s->smanet);

	return 0;
}


static int si_set_config(si_session_t *s, si_param_t *pp, json_descriptor_t *dp, char *value) {

	dprintf(1,"dp: name: %s, unit: %s, scale: %f\n", dp->name, dp->units, dp->scale);

	/* We can use any field in the union as a source or dest */
	if (pp->source == SI_PARAM_SOURCE_INV) {
		solard_inverter_t *inv = s->ap->role_data;
		json_proctab_t *invp;
		int oldval;

		invp = _getinv(s,pp->name);

		/* Save old vals */
		if (strcmp(invp->field,"run")==0) oldval = s->run_state;
		else if (strcmp(invp->field,"charge")==0) oldval = s->charge_mode;
		else if (strcmp(invp->field,"charge_at_max")==0) oldval = inv->charge_at_max;
		else oldval = -1;

		/* Update the inv struct directly */
		conv_type(invp->type,invp->ptr,0,DATA_TYPE_STRING,value,0);
		log_info("%s set to %s\n", pp->name, value);

		/* Update our local copy of charge amps too */
		if (strcmp(invp->field,"charge_amps")==0) s->charge_amps = *((float *)invp->ptr);

		if (strcmp(invp->field,"run")==0) {
			int r;

			printf("oldval: %d, state: %d\n", oldval, s->run_state);
			if (!oldval && s->run_state) {
				r = si_startstop(s,1);
				if (!r) log_write(LOG_INFO,"Started Sunny Island\n");
				else {
					log_error("start failed\n");
					s->run_state = 0;
				}
			}
			if (oldval && !s->run_state) {
				r = si_startstop(s,0);
				if (!r) {
					log_write(LOG_INFO,"Stopped Sunny Island\n");
					charge_stop(s,1);
				} else {
					log_error("stop failed\n");
					s->run_state = 0;
				}
			}
		} else if (strcmp(invp->field,"charge")==0) {
			charge_control(s,s->charge_mode,1);
#if 0
			switch(s->charge_mode) {
			case 0:
				charge_stop(s,1);
				break;
			case 1:
				charge_start(s,1);
				break;
			case 2:
				charge_start_cv(s,1);
				break;
			}
#endif
		} else if (strcmp(invp->field,"charge_at_max")==0) {
			printf("at_max: %d, oldval: %d\n", inv->charge_at_max, oldval);
			if (inv->charge_at_max && !oldval) charge_max_start(s);
			if (!inv->charge_at_max && oldval) charge_max_stop(s);
		}


		/* ALSO update this value in our config file */
//		conv_type(DATA_TYPE_STRING,temp,sizeof(temp)-1,DATA_TYPE_STRING,value,0);
		cfg_set_item(s->ap->cfg,s->ap->section_name,invp->field,"",value);
		cfg_write(s->ap->cfg);
	} else { 
		double dval;
		char *text;

		text = 0;
		dprintf(1,"pp->type: %s\n", typestr(pp->type));
		if (pp->type == DATA_TYPE_STRING) text = value;
		else conv_type(DATA_TYPE_DOUBLE,&dval,0,DATA_TYPE_STRING,value,strlen(value));
		if (smanet_set_value(s->smanet,dp->name,dval,text)) return 1;
	}
	/* Re-get the param to update internal vars and publish */
	return si_get_config(s,_getp(s,dp->name,0),dp);
//	return (pub ? si_get_config(s,_getp(s,dp->name,0),dp) : 0);
//	return 0;
}

#if 0
static void si_pubconfig(si_session_t *s) {
	json_descriptor_t *dp;
	si_param_t *pp;
	int i,j;

	/* Pub local */
	for(i=0; i < NALL; i++) {
		dprintf(1,"section: %s\n", allparms[i].name);
		dp = allparms[i].parms;
		for(j=0; j < allparms[i].count; j++) {
			dprintf(1,"dp->name: %s\n", dp[j].name);
			pp = _getp(s,dp[j].name,1);
			if (pp) si_get_config(s,pp,&dp[j]);
		}
	}
	/* XXX do not pub smanet values */
//	return;

	/* Pub smanet */
	list_reset(s->desc);
	while((dp = list_get_next(s->desc)) != 0) {
		dprintf(1,"dp->name: %s\n", dp->name);
	}
}
#endif

static int si_doconfig(void *ctx, char *action, char *label, char *value, char *errmsg) {
	si_session_t *s = ctx;
	si_param_t *pp;
	json_descriptor_t *dp;
	int r;

	r = 1;
	dprintf(1,"label: %s, value: %s\n", label, value);
	if (!label) {
		strcpy(errmsg,"invalid request");
		goto si_doconfig_error;
	}
	pp = _getp(s,label,0);
	if (!pp) {
		sprintf(errmsg,"%s: not found",label);
		goto si_doconfig_error;
	}
	dp = _getd(s,label);
	if (!dp) {
		sprintf(errmsg,"%s: not found",label);
		goto si_doconfig_error;
	}

	dprintf(1,"dp: name: %s, unit: %s, scale: %f\n", dp->name, dp->units, dp->scale);
	dprintf(1,"action: %s\n", action);
	if (strcmp(action,"Get")==0) {
		r = si_get_config(s,pp,dp);
	} else if (strcmp(action,"Set")==0) {
		if (!value) {
			strcpy(errmsg,"invalid request (Set requested but no value passed)");
			goto si_doconfig_error;
		}
		r = si_set_config(s,pp,dp,value);
	} else {
		strcpy(errmsg,"invalid request");
		goto si_doconfig_error;
	}

si_doconfig_error:
	dprintf(1,"returning: %d\n",r);
	return r;
}

static int si_config_getmsg(si_session_t *s, solard_message_t *msg) {
	char errmsg[SOLARD_ERRMSG_LEN];
	int status;
	long start;

	start = mem_used();
	status = agent_config_process(msg,si_doconfig,s,errmsg);
	if (status) goto si_config_error;

	status = 0;
	strcpy(errmsg,"success");

si_config_error:
	dprintf(1,"msg->replyto: %s", msg->replyto);
	if (msg->replyto) agent_reply(s->ap, msg->replyto, status, errmsg);
	dprintf(1,"used: %ld\n", mem_used() - start);
	return status;
}

static void getconf(si_session_t *s) {
	cfg_proctab_t mytab[] = {
		{ s->ap->section_name, "readonly", 0, DATA_TYPE_LOGICAL, &s->readonly, 0, "N" },
		{ s->ap->section_name, "soc", 0, DATA_TYPE_FLOAT, &s->user_soc, 0, "-1" },
		{ s->ap->section_name, "startup_charge_mode", 0, DATA_TYPE_INT, &s->startup_charge_mode, 0, "1" },
		{ s->ap->section_name, "grid_min_amps", 0, DATA_TYPE_FLOAT, &s->grid_min_amps, 0, "0.5" },
		{ s->ap->section_name, "charge_min_amps", 0, DATA_TYPE_FLOAT, &s->charge_min_amps, 0, "0.1" },
		{ s->ap->section_name, "charge_creep", "Increase charge voltage in order to maintain charge amps", DATA_TYPE_BOOL, &s->charge_creep, 0, "no" },
		{ s->ap->section_name, "sim_step", 0, DATA_TYPE_FLOAT, &s->sim_step, 0, "0.1" },
		{ s->ap->section_name, "interval", 0, DATA_TYPE_INT, &s->interval, 0, "10" },
		{ s->ap->section_name, "charge_method", 0, DATA_TYPE_INT, &s->charge_method, 0, "1" },
		{ s->ap->section_name, "cv_method", 0, DATA_TYPE_INT, &s->cv_method, 0, "1" },
		{ s->ap->section_name, "cv_time", 0, DATA_TYPE_INT, &s->cv_time, 0, "90" },
		{ s->ap->section_name, "cv_cutoff", 0, DATA_TYPE_INT, &s->cv_cutoff, 0, "7" },
		{ s->ap->section_name, "gen_start_timeout", 0, DATA_TYPE_INT, &s->gen_start_timeout, 0, "30" },
		{ s->ap->section_name, "notify", 0, DATA_TYPE_STRING, &s->notify_path, sizeof(s->notify_path)-1, "" },
		CFG_PROCTAB_END
	};

	cfg_get_tab(s->ap->cfg,mytab);
	if (debug) cfg_disp_tab(mytab,"si",1);
	if (!strlen(s->notify_path)) sprintf(s->notify_path,"%s/notify",SOLARD_BINDIR);

	dprintf(1,"done\n");
	return;

}

int si_config(void *h, int req, ...) {
	si_session_t *s = h;
	va_list va;
	char *p;
	void **vp;
	int r;

	r = 1;
	va_start(va,req);
	dprintf(1,"req: %d\n", req);
	switch(req) {
	case SOLARD_CONFIG_INIT:
		dprintf(1,"**** CONFIG INIT *******\n");
		/* Read our specific config */
		getconf(s);
		si_config_smanet(s);
		if (s->smanet) {
			if (smanet_get_value(s->smanet,"ExtSrc",0,&p) == 0) {
				strncpy(s->ExtSrc,p,sizeof(s->ExtSrc)-1);
				dprintf(1,"s->ExtSrc: %s\n", s->ExtSrc);
			}
		}

		/* Readonly? */
		dprintf(1,"readonly: %d\n", s->readonly);
		if (s->readonly)
			si_driver.write  = 0;
		else
			charge_init(s);

		/* Interval cannot be more than 30s */
		if (s->interval > 30) {
			log_warning("interval reduced to 30s (was: %d)\n",s->interval);
			s->interval = 30;
		}

		/* Read and write intervals are the same */
		s->ap->read_interval = s->ap->write_interval = s->interval;

		/* We are starting up */
		s->startup = 1;

		/* Publish our info */
//		pubinfo(s);

		/* Publish our config */
//		if (!s->ap->config_from_mqtt) si_pubconfig(s);

		/* Init JS */
		dprintf(1,"**** CALLING JSINIT *******\n");
		si_jsinit(s);
		break;
	case SOLARD_CONFIG_MESSAGE:
		r = si_config_getmsg(s, va_arg(va,solard_message_t *));
		break;
	case SOLARD_CONFIG_GET_INFO:
		{
			json_value_t **vp = va_arg(va,json_value_t **);
			dprintf(1,"vp: %p\n", vp);
			if (vp) {
				*vp = si_info(s);
				r = 0;
			}
		}
		break;
	case SOLARD_CONFIG_GET_DRIVER:
		vp = va_arg(va,void **);
		if (vp) {
			*vp = s->can;
			r = 0;
		}
		break;
	case SOLARD_CONFIG_GET_HANDLE:
		vp = va_arg(va,void **);
		if (vp) {
			*vp = s->can_handle;
			r = 0;
		}
		break;
	}
	dprintf(1,"returning: %d\n", r);
	va_end(va);
	return r;
}

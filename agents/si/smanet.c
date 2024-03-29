
#include "si.h"

#define dlevel 4

void get_runstate(si_session_t *s, smanet_multreq_t *mr) {
	if (mr->text && strcmp(mr->text,"Run") == 0) s->data.Run = true;
	else s->data.Run = false;
	return;
}

void get_genstate(si_session_t *s, smanet_multreq_t *mr) {
	if (mr->text && strcmp(mr->text,"Run") == 0) s->data.GnOn = true;
	else s->data.GnOn = false;
	return;
}

int si_smanet_read_data(si_session_t *s) {
	config_section_t *sec;
	config_property_t *p;
	struct {
		char *smanet_name;
		char *data_name;
		float mult;
		int can;
		void (*cb)(si_session_t *, smanet_multreq_t *);
	} *pp, parminfo[] = {
		{ "ExtVtg", "ac2_voltage_l1", 1, 1 },
		{ "ExtVtgSlv1", "ac2_voltage_l2", 1, 1 },
		{ "ExtVtgSlv2", "ac2_voltage_l3", 1, 1 },
		{ "ExtFrq", "ac2_frequency", 1, 1 },
		{ "TotExtCur", "ac2_current", 1, 1 },
		{ "InvVtg", "ac1_voltage_l1", 1, 1 },
		{ "InvVtgSlv1", "ac1_voltage_l2", 1, 1 },
		{ "InvVtgSlv2", "ac1_voltage_l3", 1, 1 },
		{ "InvFrq", "ac1_frequency", 1, 1 },
//		{ "TotInvPwrAt", "ac1_power", 1, 1 },
		{ "TotInvCur", "ac1_current", 1, 1 },
		{ "BatVtg", "battery_voltage", 1, 1 },
		{ "TotBatCur", "battery_current", 1, 1 },
		{ "BatTmp", "battery_temp", 1, 1 },
		{ "TotLodPwr", "TotLodPwr", 1, 1 },
		{ "Msg", "errmsg", 1, 1 },
		{ "GnStt", "GnOn", 1, 1, get_genstate },
		{ "InvOpStt", "GnOn", 1, 1, get_runstate },
		{ 0, 0, 0 }
	};
	smanet_multreq_t *mr;
	int count,mr_size,i;


	if (!s->smanet_connected) return 0;

	sec = config_get_section(s->ap->cp,"si_data");
	dprintf(dlevel,"sec: %p\n",sec);
	if (!sec) return 1;

	count = 0;
	for(pp = parminfo; pp->smanet_name; pp++) {
		dprintf(dlevel,"name: %s, can: %d, can_connected: %d\n", pp->smanet_name, pp->can, s->can_connected);
		if (pp->can && s->can_connected) continue;
		count++;
	}
	dprintf(1,"input.source: %d, output.source: %d\n", s->input.source, s->output.source);
	if (s->input.source == CURRENT_SOURCE_SMANET) count++;
	if (s->output.source == CURRENT_SOURCE_SMANET) count++;
	dprintf(1,"count: %d\n", count);
	if (!count) return 0;
	mr_size = count * sizeof(smanet_multreq_t);
	dprintf(dlevel,"mr_size: %d\n", mr_size);
	mr = malloc(mr_size);
	if (!mr) return 1;

	i = 0;
	for(pp = parminfo; pp->smanet_name; pp++) {
		if (pp->can && s->can_connected) continue;
		mr[i++].name = pp->smanet_name;
	}
	if (s->input.source == CURRENT_SOURCE_SMANET) mr[i++].name = s->input.name;
	if (s->output.source == CURRENT_SOURCE_SMANET) mr[i++].name = s->output.name;
//	for(i=0; i < count; i++) dprintf(0,"mr[%d]: %s\n", i, mr[i].name);
	if (smanet_get_multvalues(s->smanet,mr,count)) {
		dprintf(dlevel,"smanet_get_multvalues error");
		dprintf(0,"===> setting smanet_connected to false\n");
		s->smanet_connected = false;
		free(mr);
		return 1;
	}
	for(i=0; i < count; i++) {
		dprintf(1,"mr[%d]: %s\n", i, mr[i].name);
		if (s->input.source == CURRENT_SOURCE_SMANET && strcmp(mr[i].name,s->input.name) == 0) {
			if (s->input.type == CURRENT_TYPE_WATTS) {
				s->data.ac2_power = mr[i].value;
				s->data.ac2_current = s->data.ac2_power / s->data.ac2_voltage_l1;
			} else {
				s->data.ac2_current = mr[i].value;
				s->data.ac2_power = s->data.ac2_current * s->data.ac2_voltage_l1;
			}
			dprintf(1,"ac2_current: %.1f, ac2_power: %.1f\n", s->data.ac2_current, s->data.ac2_power);
			continue;
		}
		if (s->output.source == CURRENT_SOURCE_SMANET && strcmp(mr[i].name,s->output.name) == 0) {
			if (s->output.type == CURRENT_TYPE_WATTS) {
				s->data.ac1_power = mr[i].value;
				s->data.ac1_current = s->data.ac1_power / s->data.ac1_voltage_l1;
			} else {
				s->data.ac1_current = mr[i].value;
				s->data.ac1_power = s->data.ac1_current * s->data.ac1_voltage_l1;
			}
			dprintf(1,"ac1_current: %.1f, ac1_power: %.1f\n", s->data.ac1_current, s->data.ac1_power);
			continue;
		}
		for(pp = parminfo; pp->smanet_name; pp++) {
			if (strcmp(pp->smanet_name, mr[i].name) == 0) {
				if (pp->cb) {
					pp->cb(s, &mr[i]);
				} else {
					p = config_section_get_property(sec, pp->data_name);
					if (!p) break;
					dprintf(1,"mr[%d]: value: %f, text: %s\n", i, mr[i].value, mr[i].text);
					if (mr[i].text) 
						p->len = conv_type(p->type, p->dest, p->dsize, DATA_TYPE_STRING,
							mr[i].text, strlen(mr[i].text) );
					else  {
						double d = mr[i].value * pp->mult;
						p->len = conv_type(p->type, p->dest, p->dsize, DATA_TYPE_DOUBLE, &d, 0 );
					}
					dprintf(1,"%s: %.1f\n", p->name, *((float *)p->dest));
				}
				break;
			}
		}
	}
	if (!s->can_connected) {
		s->data.battery_soc = s->soc;
		dprintf(4,"Battery level: %.1f\n",s->data.battery_soc);

		s->data.battery_power = s->data.battery_voltage * s->data.battery_current;
		dprintf(4,"Battery power: %.1f\n",s->data.battery_power);

		s->data.ac1_voltage = s->data.ac1_voltage_l1 + s->data.ac1_voltage_l2;
		dprintf(4,"ac1_voltage: %.1f, ac1_frequency: %.1f\n",s->data.ac1_voltage,s->data.ac1_frequency);
		s->data.ac1_power = s->data.ac1_current * s->data.ac1_voltage_l1;
		dprintf(4,"ac1_current: %.1f, ac1_power: %.1f\n",s->data.ac1_current,s->data.ac1_power);

		s->data.ac2_voltage = s->data.ac2_voltage_l1 + s->data.ac2_voltage_l2;
		dprintf(4,"ac2: voltage: %.1f, frequency: %.1f\n",s->data.ac2_voltage,s->data.ac2_frequency);
		s->data.ac2_power = s->data.ac2_current * s->data.ac2_voltage_l1;
		dprintf(4,"ac2_current: %.1f, ac2_power: %.1f\n",s->data.ac2_current,s->data.ac2_power);

		s->data.TotLodPwr *= 1000.0;

		/* I was not able to figure out a way to tell if the grid is "connected" using just SMANET parms */
		if ((s->data.ac2_voltage > 10 && s->data.ac2_frequency > 10) && s->data.ac2_power > 100) s->data.GdOn = true;
		else s->data.GdOn = false;
	}
	free(mr);
	dprintf(dlevel,"done\n");
	return 0;
}

#if 0
static void *_get_smanet_info(void *ctx) {
	si_session_t *s = ctx;
	char *p;
	double d;

	if (!s->smanet) return 0;

	/* Lock it */
	dprintf(1,"locking...\n");
	smanet_lock(s->smanet);

	/* Get our ext sources */
	dprintf(4,"have_grid: %d, have_gen: %d\n", s->have_grid, s->have_gen);
	if (s->have_grid < 0 || s->have_gen < 0) {
		if (smanet_get_value(s->smanet,"ExtSrc",0,&p) == 0) {
			dprintf(1,"ExtSrc: %s\n", p);
			if (s->have_grid < 0 && strstr(p,"Grid")) s->have_grid = 1;
			if (s->have_gen < 0 && strstr(p,"Gen")) s->have_gen = 1;
		}
	}
	dprintf(4,"have_grid: %d, have_gen: %d\n", s->have_grid, s->have_gen);

	/* Grid as a generator? */
	if (s->have_grid == 1) {
		/* Set? */
		if (s->grid_soc_enabled < 0) {
			/* Try to get the value */
			if (smanet_get_value(s->smanet,"GdSocEna",0,&p) == 0) {
				dprintf(1,"GdSocEna: %s\n", p);
				if (strcmp(p,"Enable") == 0) s->grid_soc_enabled = 1;
			}
		} else {
			s->grid_soc_enabled = 0;
		}
		if (s->grid_soc_enabled && s->grid_soc_stop < 0) {
			/* Get the stop value */
			if (smanet_get_value(s->smanet,"GdSocTm1Stp",&d,0) == 0) {
				dprintf(1,"GdSocTm1Stp: %.1f\n", d);
				s->grid_soc_stop = d;
			}
		}
	}
	/* Gen? */
	if (s->have_gen == 1) {
		if (s->gen_soc_stop < 0) {
			/* Get the stop value */
			if (smanet_get_value(s->smanet,"GnSocTm1Stp",&d,0) == 0) {
				dprintf(1,"GnSocTm1Stp: %.1f\n", d);
				s->gen_soc_stop = d;
			}
		}
	}
	dprintf(1,"Grid/Gen values:\n");
	dprintf(1,"have_grid: %d, grid_soc_enabled: %d, grid_soc_stop: %.1f\n",
		s->have_grid, s->grid_soc_enabled, s->grid_soc_stop);
	dprintf(1,"have_gen: %d, gen_soc_stop: %.1f\n", s->have_gen, s->gen_soc_stop);

#if 0
	dprintf(1,"unlocking...\n");
	smanet_unlock(s->smanet);
#endif
	return 0;
}
#endif

static void _addchans(si_session_t *s) {
	smanet_session_t *ss = s->smanet;
	smanet_channel_t *c;
	config_section_t *sec;
	config_property_t newp;
	float step;
	register int i;

	dprintf(dlevel,"adding channels...\n");
	sec = config_create_section(s->ap->cp,"smanet",0);
	if (!sec) return;

#if 0
	dprintf(dlevel+1,"locking...\n");
	smanet_lock(s->smanet);
#endif

	dprintf(dlevel,"chancount: %d\n", ss->chancount);
	for(i=0; i < ss->chancount; i++) {
		c = &ss->chans[i];
		dprintf(dlevel,"c->mask: %04x, CH_PARA: %04x\n", c->mask, CH_PARA);
		if ((c->mask & CH_PARA) == 0) continue;
//		dprintf(dlevel+1,"adding chan: %s\n", c->name);
		memset(&newp,0,sizeof(newp));
		newp.name = c->name;
		newp.flags = SI_CONFIG_FLAG_SMANET;
		step = 1;
		dprintf(dlevel+1,"c->format: %08x\n", c->format);
		switch(c->format & 0xf) {
		case CH_BYTE:
			newp.type = DATA_TYPE_BYTE;
			break;
		case CH_SHORT:
			newp.type = DATA_TYPE_SHORT;
			break;
		case CH_LONG:
			newp.type = DATA_TYPE_LONG;
			break;
		case CH_FLOAT:
			newp.type = DATA_TYPE_FLOAT;
			step = .1;
			break;
		case CH_DOUBLE:
			newp.type = DATA_TYPE_DOUBLE;
			step = .1;
			break;
#if 0
		case CH_ARRAY | CH_BYTE:
			newp.type = DATA_TYPE_U8_ARRAY;
			break;
		case CH_ARRAY | CH_SHORT:
			newp.type = DATA_TYPE_U16_ARRAY;
			break;
		case CH_ARRAY | CH_LONG:
			newp.type = DATA_TYPE_U32_ARRAY;
			break;
		case CH_ARRAY | CH_FLOAT:
			newp.type = DATA_TYPE_F32_ARRAY;
			step = .1;
			break;
		case CH_ARRAY | CH_DOUBLE:
			newp.type = DATA_TYPE_F64_ARRAY;
			step = .1;
			break;
#endif
		default:
			printf("SMANET: unknown format: %04x\n", c->format & 0xf);
			break;
		}
		dprintf(dlevel+1,"c->mask: %08x\n", c->mask);
		if (c->mask & CH_PARA && c->mask & CH_ANALOG) {
			/* XXX values must be same type as item */
			float tmp[3];
			void *values;

			newp.scope = "range";
			tmp[0] = c->gain;
			tmp[1] = c->offset;
			tmp[2] = step;
//			dprintf(dlevel,"adding range: 0: %f, 1: %f, 2: %f\n", tmp[0], tmp[1], tmp[2]);
			values = malloc(3*typesize(newp.type));
			conv_type(newp.type | DATA_TYPE_ARRAY,values,3,DATA_TYPE_FLOAT_ARRAY,tmp,3);
			newp.nvalues = 3;
			newp.values = values;
		} else if (c->mask & CH_DIGITAL) {
			char **labels;

			printf("digital: %s\n", newp.name);
			newp.nlabels = 2;
			labels = malloc(newp.nlabels*sizeof(char *));
			labels[0] = c->txtlo;
			labels[1] = c->txthi;
			newp.labels = labels;
		} else if (c->mask & CH_STATUS && list_count(c->strings)) {
			char **labels,*p;
			int i;

			newp.type = DATA_TYPE_STRING;
			newp.scope = "select";
			newp.nlabels = list_count(c->strings);
			labels = malloc(newp.nlabels*sizeof(char *));
			i = 0;
			list_reset(c->strings);
			while((p = list_get_next(c->strings)) != 0) labels[i++] = p;
			newp.labels = labels;
		}
		newp.units = c->unit;
		newp.scale = 1.0;
		config_section_add_property(s->ap->cp, sec, &newp, SI_CONFIG_FLAG_SMANET);
	}

	/* Re-create info and publish it */
	if (s->ap->info) json_destroy_value(s->ap->info);
	s->ap->info = si_get_info(s);
#ifdef MQTT
	agent_pubinfo(s->ap,0);
#endif

	if (0) {
		config_section_t *sec;
		config_property_t *p;

		sec = config_get_section(s->ap->cp,"smanet");
		dprintf(dlevel+1,"sec: %p\n", sec);
		if (!sec) exit(1);
		list_reset(sec->items);
		while((p = list_get_next(sec->items)) != 0) {
			dprintf(dlevel,"p: name: %s, flags: %x\n", p->name, p->flags);
		}
//		exit(0);
	}
#if 0
	dprintf(dlevel+1,"unlocking...\n");
	smanet_unlock(s->smanet);
#endif
}

/* Setup SMANET - runs 1 time per connection */
int si_smanet_setup(si_session_t *s) {
//	pthread_attr_t attr;
//	pthread_t th;

	/* Load channels */
	if (!strlen(s->smanet_channels_path)) sprintf(s->smanet_channels_path,"%s/%s_channels.json",SOLARD_LIBDIR,s->smanet->type);
	if (access(s->smanet_channels_path,0)) {
		log_warning("unable to access smanet_channels_path: %s, not loading!\n", s->smanet_channels_path);
		return 1;
	}

        /* Add SMANET channels to config */
	if (!strlen(s->smanet_channels_path)) sprintf(s->smanet_channels_path,"%s/%s_channels.json",SOLARD_LIBDIR,s->smanet->type);
	fixpath(s->smanet_channels_path, sizeof(s->smanet_channels_path)-1);
	dprintf(1,"smanet_channels_path: %s\n", s->smanet_channels_path);
	if (smanet_load_channels(s->smanet, s->smanet_channels_path)) {
		log_error("unable to load SMANET channels: %s\n", s->smanet->errmsg);
		return 1;
	}
	_addchans(s);
#ifdef JS
	if (s->ap && s->ap->js) smanet_jsinit(s->ap->js);
#endif

	s->smanet_added = 1;
	return 0;
}

int si_smanet_init(si_session_t *s) {
	dprintf(1,"sma_transport: %s, sma_target: %s, sma_topts: %s\n", s->smanet_transport, s->smanet_target, s->smanet_topts);

	/* Init SMANET */
	if (s->smanet) smanet_destroy(s->smanet);
	s->smanet = smanet_init(s->smanet_transport, s->smanet_target, s->smanet_topts);
	dprintf(1,"s->smanet: %p\n", s->smanet);
	if (!s->smanet) {
		sprintf(s->errmsg,"smanet_init: %s", strerror(errno));
		return 1;
	}
	s->smanet_connected = s->smanet->connected;

	dprintf(1,"returning 0\n");
	return 0;
}

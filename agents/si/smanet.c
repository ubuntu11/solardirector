
#include "si.h"

int si_smanet_read_data(si_session_t *s) {
	config_section_t *sec;
	config_property_t *p;
	struct {
		char *smanet_name;
		char *data_name;
		float mult;
		int can;
	} *pp, parminfo[] = {
		{ "ExtVtg", "ac2_voltage_l1", 1, 1 },
		{ "ExtVtgSlv1", "ac2_voltage_l2", 1, 1 },
		{ "ExtVtgSlv2", "ac2_voltage_l3", 1, 1 },
		{ "ExtFrq", "ac2_frequency", 1, 1 },
		{ "TotExtCur", "ac2_current", 1, 0 },
		{ "InvVtg", "ac1_voltage_l1", 1, 1 },
		{ "InvVtgSlv1", "ac1_voltage_l2", 1, 1 },
		{ "InvVtgSlv2", "ac1_voltage_l3", 1, 1 },
		{ "InvFrq", "ac1_frequency", 1, 1 },
		{ "TotInvCur", "ac1_current", 1, 0 },
		{ "BatVtg", "battery_voltage", 1, 1 },
		{ "TotBatCur", "battery_current", 1, 1 },
		{ "BatTmp", "battery_temp", 1 },
		{ 0, 0, 0 }
	};
	smanet_multreq_t *mr;
	int count,mr_size,i;


	sec = config_get_section(s->ap->cp,"si_data");
	dprintf(0,"sec: %p\n",sec);
	if (!sec) return 1;

	count = 0;
	for(pp = parminfo; pp->smanet_name; pp++) {
		if (pp->can && s->can_connected) continue;
		count++;
	}
	mr_size = count * sizeof(smanet_multreq_t);
	dprintf(0,"mr_size: %d\n", mr_size);
	mr = malloc(mr_size);
	if (!mr) return 1;

	i = 0;
	for(pp = parminfo; pp->smanet_name; pp++) {
		if (pp->can && s->can_connected) continue;
		mr[i++].name = pp->smanet_name;
	}
	for(i=0; i < count; i++) dprintf(1,"mr[%d]: %s\n", i, mr[i].name);
	if (smanet_get_multvalues(s->smanet,mr,count)) {
		dprintf(0,"smanet_get_multvalues error");
		free(mr);
		return 1;
	}
	i = 0;
	for(pp = parminfo; pp->smanet_name; pp++) {
		if (pp->can && s->can_connected) continue;
		p = config_section_get_property(sec, pp->data_name);
		if (!p) break;
		dprintf(1,"mr[%d]: value: %f, text: %s\n", i, mr[i].value, mr[i].text);
		if (mr[i].text) 
			p->len = conv_type(p->type, p->dest, p->dsize, DATA_TYPE_STRING, mr[i].text, strlen(mr[i].text) );
		else  {
			double d = mr[i].value * pp->mult;
			p->len = conv_type(p->type, p->dest, p->dsize, DATA_TYPE_DOUBLE, &d, 0 );
		}
		i++;
		dprintf(0,"%s: %.1f\n", p->name, *((float *)p->dest));
	}
	if (!s->can_connected) {
		s->data.ac1_voltage = s->data.ac1_voltage_l1 + s->data.ac1_voltage_l2;
		dprintf(4,"ac1_voltage: %.1f, ac1_frequency: %.1f\n",s->data.ac1_voltage,s->data.ac1_frequency);
		s->data.ac2_voltage = s->data.ac2_voltage_l1 + s->data.ac2_voltage_l2;
		dprintf(4,"ac2: voltage: %.1f, frequency: %.1f\n",s->data.ac2_voltage,s->data.ac2_frequency);
	}
	if (s->input_current_type == 0) {
		s->data.ac1_power = s->data.ac1_current * s->data.ac1_voltage_l1;
		s->data.ac2_power = s->data.ac2_current * s->data.ac2_voltage_l1;
	}
	free(mr);
	dprintf(0,"done\n");
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

	dprintf(1,"unlocking...\n");
	smanet_unlock(s->smanet);
	return 0;
}
#endif

static void _addchans(si_session_t *s) {
	smanet_session_t *ss = s->smanet;
	smanet_channel_t *c;
	config_section_t *sec;
	config_property_t newp;
	float step;

	sec = config_create_section(s->ap->cp,"smanet",0);
	printf("===> newsec: %p\n", sec);
	if (!sec) return;

	dprintf(1,"locking...\n");
	smanet_lock(s->smanet);

	list_reset(ss->channels);
	while((c = list_get_next(ss->channels)) != 0) {
		dprintf(0,"c->mask: %04x, CH_PARA: %04x\n", c->mask, CH_PARA);
		if ((c->mask & CH_PARA) == 0) continue;
		dprintf(1,"adding chan: %s\n", c->name);
		memset(&newp,0,sizeof(newp));
		newp.name = c->name;
		step = 1;
		dprintf(1,"c->format: %08x\n", c->format);
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
		dprintf(1,"c->mask: %08x\n", c->mask);
		if (c->mask & CH_PARA && c->mask & CH_ANALOG) {
			/* XXX values must be same type as item */
			float tmp[3];
			void *values;

			newp.scope = "range";
			tmp[0] = c->gain;
			tmp[1] = c->offset;
			tmp[2] = step;
//			dprintf(0,"adding range: 0: %f, 1: %f, 2: %f\n", tmp[0], tmp[1], tmp[2]);
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
		config_section_add_property(s->ap->cp, sec, &newp, CONFIG_FLAG_VOLATILE);
//		if (strcmp(c->name,"AutoStr")==0) break;
	}

	/* Re-create info and publish it */
	if (s->ap->driver_info) json_destroy_value(s->ap->driver_info);
	s->ap->driver_info = si_get_info(s);
	agent_pubinfo(s->ap,0);

	dprintf(1,"unlocking...\n");
	smanet_unlock(s->smanet);
}

/* Setup SMANET - runs 1 time per connection */
int si_smanet_setup(si_session_t *s) {
//	pthread_attr_t attr;
//	pthread_t th;

	/* Load channels */
	if (!strlen(s->smanet_channels_path)) sprintf(s->smanet_channels_path,"%s/%s.dat",SOLARD_LIBDIR,s->smanet->type);
	if (access(s->smanet_channels_path,0) == 0) {
		dprintf(1,"loading smanet channels from: %s\n", s->smanet_channels_path);
		smanet_load_channels(s->smanet,s->smanet_channels_path);
	} else {
		log_warning("unable to access smanet_channels_path: %s, not loading!\n", s->smanet_channels_path);
	}

#if 0
	/* Create a detached thread to get the info */
	if (pthread_attr_init(&attr)) {
		sprintf(s->errmsg,"pthread_attr_init: %s",strerror(errno));
		goto si_smanet_init_error;
	}
	if (pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_DETACHED)) {
		sprintf(s->errmsg,"pthread_attr_setdetachstate: %s",strerror(errno));
		goto si_smanet_init_error;
	}
	dprintf(1,"creating thread...\n");
	if (pthread_create(&th,&attr,&_get_smanet_info,s)) {
		sprintf(s->errmsg,"pthread_create: %s",strerror(errno));
		goto si_smanet_init_error;
	}
#endif

        /* Add SMANET channels to config */
        dprintf(1,"smanet: %p\n", s->smanet);
	dprintf(1,"smanet_channels_path: %s\n", s->smanet_channels_path);
	if (!strlen(s->smanet_channels_path)) sprintf(s->smanet_channels_path,"%s/%s.dat",SOLARD_LIBDIR,s->smanet->type);
	fixpath(s->smanet_channels_path, sizeof(s->smanet_channels_path)-1);
	dprintf(1,"smanet_channels_path: %s\n", s->smanet_channels_path);
	if (smanet_load_channels(s->smanet, s->smanet_channels_path) == 0) _addchans(s);
	if (s->ap && s->ap->js) smanet_jsinit(s->ap->js);

	s->smanet_added = 1;
	return 0;
}

/* Do the SMA init/collection in a thread as it may take awhile (dont want it to slow startup) */
int si_smanet_init(si_session_t *s) {
	dprintf(1,"sma_transport: %s, sma_target: %s, sma_topts: %s\n", s->smanet_transport, s->smanet_target, s->smanet_topts);

	/* Init SMANET */
	if (s->smanet) smanet_destroy(s->smanet);
#if 0
	dprintf(1,"startup: %d, can_connected: %d\n", s->startup, s->can_connected);
	if (s->startup == 0 || s->can_connected == 0) si_smanet_init(s);
		dprintf(1,"smanet: %p, smanet_connected: %d\n", s->smanet, s->smanet_connected);
		if (!s->smanet_connected) {
			dprintf(1,"smanet_transport: %s, smanet_target: %s, smanet_topts: %s\n",
				s->smanet_transport, s->smanet_target, s->smanet_topts);
			if (!s->smanet) {
				if (strlen(s->smanet_transport) && strlen(s->smanet_target)) {
					si_smanet_init(s);
					s->smanet_connected = s->smanet->connected;
				}
			} else if (strlen(s->smanet_transport) && strlen(s->smanet_target)) {
				if (smanet_connect(s->smanet, s->smanet_transport, s->smanet_target, s->smanet_topts))
					log_error(smanet_get_errmsg(s->smanet));
				s->smanet_connected = s->smanet->connected;
			}
			if (s->smanet_connected && !s->smanet_added) si_smanet_setup(s);
		}
	}
#endif
	s->smanet = smanet_init(s->smanet_transport, s->smanet_target, s->smanet_topts);
	dprintf(1,"s->smanet: %p\n", s->smanet);
	if (!s->smanet) {
		sprintf(s->errmsg,"smanet_init: %s", strerror(errno));
		return 1;
	}
	s->smanet_connected = s->smanet->connected;

//	if (s->smanet_connected && !s->smanet_added) si_smanet_setup(s);

	dprintf(1,"returning 0\n");
	return 0;

//si_smanet_init_error:
//	return 1;
}

int si_start_grid(si_session_t *s, int wait) {
	char *p;
	time_t start,now;
	int diff;

	dprintf(1,"smanet: %p\n", s->smanet);
	if (!s->smanet) {
		sprintf(s->errmsg,"no SMANET");
		return 1;
	}

	/* Get the current val */
	if (smanet_get_value(s->smanet,"GdManStr",0,&p) != 0) {
		sprintf(s->errmsg,"unable to get current value of GdManStr: %s",smanet_get_errmsg(s->smanet));
		return 1;
	}
	if (strcmp(p,"Start")==0) {
		dprintf(1,"already started\n");
		return 0;
	}

	/* Set start */
	if (smanet_set_value(s->smanet,"GdManStr",0,"Start") != 0) {
		sprintf(s->errmsg,"unable to start grid: %s",smanet_get_errmsg(s->smanet));
		return 1;
	}
	dprintf(1,"wait: %d\n", wait);
	if (!wait) return 0;

	if (s->grid_start_timeout < 0) {
		log_warning("smanet_start_grid wait requested, but grid_start_timeout unset, not waiting!\n");
		return 0;
	}

	time(&start);
	while(1) {
		if (s->data.GdOn) break;
		time(&now);
		diff = now - start;
		dprintf(1,"diff: %d\n", diff);
		if (diff > s->grid_start_timeout) {
			sprintf(s->errmsg,"timeout waiting for grid to start");
			return 1;
		}
	}

	dprintf(1,"done!\n");
	return 0;
}

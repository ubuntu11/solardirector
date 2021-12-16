
#include "si.h"

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

/* Do the SMA init/collection in a thread as it may take awhile (dont want it to slow startup) */
int si_smanet_init(si_session_t *s) {
	solard_driver_t *tp;
	void *tp_handle;
	pthread_attr_t attr;
	pthread_t th;

	dprintf(1,"sma_transport: %s, sma_target: %s, sma_topts: %s\n", s->smanet_transport, s->smanet_target, s->smanet_topts);

	/* Find the driver */
	tp = find_driver(si_transports,s->smanet_transport);
	if (!tp) {
		sprintf(s->errmsg,"unable to find smanet_transport: %s", s->smanet_transport);
		return 0;
	}

	/* Create a new driver instance */
	tp_handle = tp->new(s->ap, s->smanet_target, s->smanet_topts);
	if (!tp_handle) {
		sprintf(s->errmsg,"%s_new: %s", s->smanet_transport, strerror(errno));
		return 0;
	}

	/* Init SMANET */
	s->smanet = smanet_init(tp,tp_handle);
	if (!s->smanet) {
		sprintf(s->errmsg,"smanet_init: %s", strerror(errno));
		return 0;
	}

	/* Load channels */
	if (!strlen(s->smanet_channels_path)) sprintf(s->smanet_channels_path,"%s/%s.dat",SOLARD_LIBDIR,s->smanet->type);
	if (access(s->smanet_channels_path,0) == 0) {
		dprintf(1,"loading smanet channels from: %s\n", s->smanet_channels_path);
		smanet_set_chanpath(s->smanet,s->smanet_channels_path);
		smanet_load_channels(s->smanet);
	} else {
		log_warning("unable to access smanet_channels_path: %s, not loading!\n", s->smanet_channels_path);
	}

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
	return 0;

si_smanet_init_error:
	return 1;
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
		if (s->info.GdOn) break;
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

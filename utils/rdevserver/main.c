
/*
Copyright (c) 2021, Stephen P. Shoecraft
All rights reserved.

This source code is licensed under the BSD-style license found in the
LICENSE file in the root directory of this source tree.
*/

#include "rdevserver.h"
#include <pthread.h>
#include "transports.h"

#define TESTING 0
#define DEBUG_STARTUP 0

#if DEBUG_STARTUP
#define _ST_DEBUG LOG_DEBUG
#else
#define _ST_DEBUG 0
#endif

static void *rdev_recv_thread(void *handle) {
	rdev_config_t *conf = handle;
	struct can_frame frame;
	int bytes,idx;
	uint32_t mask;

	dprintf(3,"thread started!\n");
	while(solard_check_state(conf,RDEV_STATE_OPEN)) {
		bytes = conf->can->read(conf->can_handle,&frame,0xffff);
		dprintf(7,"bytes: %d\n", bytes);
		if (bytes < 0) {
			memset(&conf->bitmaps,0,sizeof(conf->bitmaps));
			sleep(1);
			continue;
		}
 		if (bytes != sizeof(frame)) continue;
		dprintf(7,"frame.can_id: %03x\n",frame.can_id);
		if (frame.can_id >= NFRAMES) continue;
		idx = frame.can_id / 32;
		mask = 1 << (frame.can_id % 32);
		dprintf(7,"idx: %d, mask: %08x\n",idx,mask);
		memcpy(&conf->frames[frame.can_id],&frame,sizeof(frame));
		conf->bitmaps[idx] |= mask;
	}
	dprintf(3,"thread returning!\n");
	return 0;
}

static int rdev_can_open(void *handle) {
	rdev_config_t *conf = handle;
	pthread_attr_t attr;
	int r;

	r = 1;

	r = conf->can->open(conf->can_handle);
	if (r) return r;
	solard_set_state(conf,RDEV_STATE_OPEN);

	/* Start the CAN read thread */
	if (pthread_attr_init(&attr)) {
		log_write(LOG_SYSERR,"pthread_attr_init");
		goto rdev_can_open_error;
	}
	if (pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_DETACHED)) {
		log_write(LOG_SYSERR,"pthread_attr_setdetachstate");
		goto rdev_can_open_error;
	}
	if (pthread_create(&conf->th,&attr,&rdev_recv_thread,conf)) {
		log_write(LOG_SYSERR,"pthread_create");
		goto rdev_can_open_error;
	}
	r = 0;
rdev_can_open_error:
	return r;
}

static int rdev_can_read(void *handle, void *buf, int buflen) {
	rdev_config_t *conf = handle;
	char what[16];
	uint32_t mask;
	int id,idx,retries;

	dprintf(5,"buf: %p, buflen: %d\n", buf, buflen);

	/* buflen is the can id */

	id = buflen;
	dprintf(5,"id: %03x\n", id);
	if (id >= NFRAMES) return 0;

	idx = id / 32;
	mask = 1 << (id % 32);
	dprintf(5,"mask: %08x, bitmaps[%d]: %08x\n", mask, idx, conf->bitmaps[idx]);
	retries=5;
	while(retries--) {
		if ((conf->bitmaps[idx] & mask) == 0) {
			dprintf(5,"bit not set, sleeping...\n");
			sleep(1);
			continue;
		}
		sprintf(what,"%03x", id);
		if (debug >= 5) bindump(what,&conf->frames[id],conf->frames[idx].can_dlc);
		memcpy(buf,&conf->frames[id],sizeof(struct can_frame));
		return sizeof(struct can_frame);
	}
	return 0;
}

static int rdev_can_write(void *handle, void *buf, int buflen) {
	rdev_config_t *conf = handle;
	return conf->can->write(conf->can_handle,buf,buflen);
}

static int rdev_can_close(void *handle) {
	rdev_config_t *conf = handle;
	int r;

	dprintf(1,"closing...\n");
	r = conf->can->close(conf->can_handle);
	if (!r) solard_clear_state(conf,RDEV_STATE_OPEN);
	return r;
}

#if defined(__WIN32) || defined(__WIN64)
static solard_driver_t *transports[] = { &serial_driver, 0 };
#else
#ifdef BLUETOOTH
static solard_driver_t *transports[] = { &bt_driver, &can_driver, &serial_driver, 0 };
#else
static solard_driver_t *transports[] = { &can_driver, &serial_driver, 0 };
#endif
#endif

int add_device(rdev_config_t *conf, char *name) {
	char transport[SOLARD_TRANSPORT_LEN],target[SOLARD_TARGET_LEN],topts[SOLARD_TOPTS_LEN];
	cfg_proctab_t tab[] = {
		{ name,"transport","Device transport",DATA_TYPE_STRING,&transport,sizeof(transport)-1, "" },
		{ name,"target","Device target",DATA_TYPE_STRING,&target,sizeof(target)-1, "" },
		{ name,"topts","Device transport options",DATA_TYPE_STRING,&topts,sizeof(topts)-1, "" },
		CFG_PROCTAB_END
	};
	devserver_io_t dev;
	solard_driver_t *mp,*tp;
	void *mp_handle;
	int i;

	dprintf(1,"reading...\n");
	cfg_get_tab(conf->cfg,tab);
	if (debug) cfg_disp_tab(tab,name,0);

	dprintf(1,"transport: %s, target: %s, topts: %s\n", transport, target, topts);
	if (!strlen(transport) || !strlen(target)) {
		log_write(LOG_ERROR,"[%s] must have both transport and target\n",name);
		return 1;
	}

	/* Find the transport in the list of transports */
	mp = 0;
	for(i=0; transports[i]; i++) {
		tp = transports[i];
		dprintf(1,"tp->type: %d\n", tp->type);
		if (tp->type != SOLARD_DRIVER_TRANSPORT) continue;
		dprintf(1,"tp->name: %s\n", tp->name);
		if (strcmp(tp->name,transport)==0) {
			mp = tp;
			break;
		}
	}
	dprintf(1,"mp: %p\n", mp);
	if (!mp) goto add_device_error;

	/* Test the driver */
	mp_handle = mp->new(conf,target,topts);
	if (!mp_handle) goto add_device_error;
	if (mp->open(mp_handle)) goto add_device_error;
	mp->close(mp_handle);

	memset(&dev,0,sizeof(dev));
	strncat(dev.name,name,sizeof(dev.name));
	strncat(dev.type,transport,sizeof(dev.type));
	if (strcmp(mp->name,"can")==0) {
		conf->can = mp;
		conf->can_handle = mp_handle;
		dev.handle = conf;
		dev.open = rdev_can_open;
		dev.read = rdev_can_read;
		dev.write = rdev_can_write;
		dev.close = rdev_can_close;
	} else {
		dev.handle = mp_handle;
		dev.open = mp->open;
		dev.read = mp->read;
		dev.write = mp->write;
		dev.close = mp->close;
	}
	devserver_add_unit(&conf->ds,&dev);
	return 0;
add_device_error:
	return 1;
}

int main(int argc,char **argv) {
	rdev_config_t *conf;
	int port;
	char configfile[256],*p;
	opt_proctab_t opts[] = {
                /* Spec, dest, type len, reqd, default val, have */
		{ "-c:%|config file",&configfile,DATA_TYPE_STRING,sizeof(configfile)-1,1,"" },
		OPTS_END
	};
	int logopts = LOG_INFO|LOG_WARNING|LOG_ERROR|LOG_SYSERR|_ST_DEBUG;
	list devices;
	cfg_proctab_t tab[] = {
		{ "rdev", "port", "Network port", DATA_TYPE_INT,&port, 0, "3900" },
		{ "rdev","devices","Device list",DATA_TYPE_STRING_LIST,&devices,0,0 },
		CFG_PROCTAB_END
	};
	devserver_io_t dummy_dev = { "dummy", "dummy", 0, 0, 0, 0 };
#if TESTING
	char *args[] = { "rdevd", "-d", "4", "-c", "rdev.conf" };
	argc = (sizeof(args)/sizeof(char *));
	argv = args;
#endif

	solard_common_init(argc,argv,opts,logopts);

	if (!strlen(configfile)) {
		log_write(LOG_ERROR,"configfile required.\n");
		return 1;
	}

	conf = calloc(1,sizeof(*conf));
	if (!conf) {
		log_write(LOG_SYSERR,"calloc config");
		goto init_error;
	}
	conf->modules = list_create();

	/* Add a dummy dsfunc for unit 0 */
	devserver_add_unit(&conf->ds,&dummy_dev);

	dprintf(1,"reading...\n");
	conf->cfg = cfg_read(configfile);
	if (!conf->cfg) {
		log_write(LOG_SYSERROR,"error reading configfile '%s'", configfile);
		goto init_error;
	}
	cfg_get_tab(conf->cfg,tab);
	if (debug) cfg_disp_tab(tab,"rdev",0);

	dprintf(1,"devices count: %d\n", list_count(devices));
	list_reset(devices);
	while((p = list_get_next(devices)) != 0) {
		if (add_device(conf,p)) return 1;
	}

	solard_set_state(conf,RDEV_STATE_RUNNING);
	server(conf,port);
	return 0;
init_error:
	free(conf);
	return 1;
}

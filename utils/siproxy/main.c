
#include "siproxy.h"
#include <pthread.h>

#define DEBUG_STARTUP 0

#if DEBUG_STARTUP
#define _ST_DEBUG LOG_DEBUG
#else
#define _ST_DEBUG 0
#endif

static void *siproxy_recv_thread(void *handle) {
	siproxy_config_t *conf = handle;
	struct can_frame frame;
	int r;

	dprintf(3,"thread started!\n");
	while(solard_check_state(conf,SI_STATE_OPEN)) {
		frame.can_id = 0xffff;
		r = conf->can->read(conf->can_handle,&frame,sizeof(frame));
		dprintf(7,"r: %d\n", r);
		if (r < 0) {
			memset(&conf->bitmap,0,sizeof(conf->bitmap));
			sleep(1);
			continue;
		}
 		if (r != sizeof(frame)) continue;
		dprintf(7,"frame.can_id: %03x\n",frame.can_id);
		if (frame.can_id < 0x300 || frame.can_id > 0x30a) continue;
		memcpy(&conf->frames[frame.can_id - 0x300],&frame,sizeof(frame));
		conf->bitmap |= 1 << (frame.can_id - 0x300);
	}
	dprintf(3,"thread returning!\n");
	return 0;
}

static int siproxy_can_open(void *handle) {
	siproxy_config_t *conf = handle;
	pthread_attr_t attr;
	int r;

	r = 1;

	r = conf->can->open(conf->can_handle);
	if (r) return r;
	solard_set_state(conf,SI_STATE_OPEN);

	/* Start the CAN read thread */
	if (pthread_attr_init(&attr)) {
		log_write(LOG_SYSERR,"pthread_attr_init");
		goto siproxy_can_open_error;
	}
	if (pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_DETACHED)) {
		log_write(LOG_SYSERR,"pthread_attr_setdetachstate");
		goto siproxy_can_open_error;
	}
	if (pthread_create(&conf->th,&attr,&siproxy_recv_thread,conf)) {
		log_write(LOG_SYSERR,"pthread_create");
		goto siproxy_can_open_error;
	}
	r = 0;
siproxy_can_open_error:
	return r;
}

static int siproxy_can_read(void *handle, void *buf, int buflen) {
	siproxy_config_t *conf = handle;
	char what[16];
	uint16_t mask;
	struct can_frame *frame;
	int id,idx,retries;

	dprintf(5,"buf: %p, buflen: %d\n", buf, buflen);

	/* buflen must equal can frame size */
	if (buflen != sizeof(struct can_frame)) return -1;

	frame = buf;
	id = frame->can_id;
	dprintf(5,"id: %03x\n", id);
	if (id < 0x300 || id > 0x30F) return 0;

	dprintf(5,"bitmap: %04x\n", conf->bitmap);
	idx = id - 0x300;
	mask = 1 << idx;
	dprintf(5,"mask: %04x\n", mask);
	retries=5;
	while(retries--) {
		if ((conf->bitmap & mask) == 0) {
			dprintf(5,"bit not set, sleeping...\n");
			sleep(1);
			continue;
		}
		sprintf(what,"%03x", id);
		if (debug >= 5) bindump(what,&conf->frames[idx],conf->frames[idx].can_dlc);
		memcpy(buf,&conf->frames[idx],sizeof(struct can_frame));
		return sizeof(struct can_frame);
	}
	return 0;
}

static int siproxy_can_write(void *handle, void *buf, int buflen) {
	siproxy_config_t *conf = handle;
	return conf->can->write(conf->can_handle,buf,buflen);
}

static int siproxy_can_close(void *handle) {
	siproxy_config_t *conf = handle;
	int r;

	dprintf(1,"closing...\n");
	r = conf->can->close(conf->can_handle);
	if (!r) solard_clear_state(conf,SI_STATE_OPEN);
	return r;
}

int main(int argc,char **argv) {
	siproxy_config_t *conf;
	int port;
	char configfile[256],can_interface[32],can_topts[64],tty_interface[32],tty_topts[64];
	opt_proctab_t opts[] = {
                /* Spec, dest, type len, reqd, default val, have */
		{ "-c:%|config file",&configfile,DATA_TYPE_STRING,sizeof(configfile)-1,0,"" },
		{ "-p:#|port",&port,DATA_TYPE_INT,0,0,"3900" },
		{ "-i::|can interface",&can_interface,DATA_TYPE_STRING,sizeof(can_interface)-1,0,"" },
		{ "-t::|tty interface",&tty_interface,DATA_TYPE_STRING,sizeof(tty_interface)-1,0,"" },
		OPTS_END
	};
//	char *args[] = { "t2", "-d", "4", "-c", "siproxy.conf" };
//	#define nargs (sizeof(args)/sizeof(char *))
	int logopts = LOG_INFO|LOG_WARNING|LOG_ERROR|LOG_SYSERR|_ST_DEBUG;
	devserver_io_t dummy_dev = { "dummy", "dummy", 0, 0, 0, 0, 0 };

//	solard_common_init(nargs,args,opts,0xffff);
	solard_common_init(argc,argv,opts,logopts);

	conf = calloc(1,sizeof(*conf));
	if (!conf) {
		log_write(LOG_SYSERR,"calloc config");
		goto init_error;
	}
	conf->modules = list_create();

	/* Add a dummy dsfunc for unit 0 */
	devserver_add_unit(&conf->ds,&dummy_dev);

	dprintf(1,"configfile: %s\n", configfile);
	if (strlen(configfile)) {
		cfg_proctab_t tab[] = {
			{ "siproxy","can_interface","CAN Interface",DATA_TYPE_STRING,&can_interface,sizeof(can_interface), "" },
			{ "siproxy","can_topts","CAN Interface options",DATA_TYPE_STRING,&can_topts,sizeof(can_topts), "" },
			{ "siproxy","tty_interface","Serial Interface",DATA_TYPE_STRING,&tty_interface,sizeof(tty_interface), "" },
			{ "siproxy","tty_topts","Serial Interface options",DATA_TYPE_STRING,&tty_topts,sizeof(tty_topts), "" },
			{ "siproxy", "port", "Network port", DATA_TYPE_INT,&port, 0, 0 },
			CFG_PROCTAB_END
		};
		dprintf(1,"reading...\n");
		conf->cfg = cfg_read(configfile);
                if (!conf->cfg) {
			log_write(LOG_SYSERROR,"error reading configfile '%s'", configfile);
			goto init_error;
		}
                cfg_get_tab(conf->cfg,tab);
                if (debug) cfg_disp_tab(tab,"siproxy",0);
	}

	/* Need to set before starting thread */
	solard_set_state(conf,SI_STATE_RUNNING);

	dprintf(1,"can_interface: %s, topts: %s\n", can_interface, can_topts);
	if (strlen(can_interface)) {
		devserver_io_t can;

		memset(&can,0,sizeof(can));
		configfile[0] = 0;
		strncat(configfile,strele(0,",",can_interface),sizeof(configfile)-1);
		strncat(can.name,strele(0,":",configfile),sizeof(can.name)-1);
		strncat(can.device,strele(1,":",configfile),sizeof(can.device)-1);
		dprintf(1,"can: name: %s, device: %s\n", can.name, can.device);

		/* load the can module and test */
		conf->can = load_module(conf->modules,"can",SOLARD_MODTYPE_TRANSPORT);
		if (!conf->can) goto init_error;
		conf->can_handle = conf->can->new(conf,can.device,can_topts);
		if (!conf->can_handle) goto init_error;
		if (conf->can->open(conf->can_handle)) goto init_error;
		conf->can->close(conf->can_handle);

		can.handle = conf;
		can.open = siproxy_can_open;
		can.read = siproxy_can_read;
		can.write = siproxy_can_write;
		can.close = siproxy_can_close;
		devserver_add_unit(&conf->ds,&can);
	}

	dprintf(1,"tty_interface: %s, topts: %s\n", tty_interface, tty_topts);
	if (strlen(tty_interface)) {
		devserver_io_t tty;

		memset(&tty,0,sizeof(tty));
		configfile[0] = 0;
		strncat(configfile,strele(0,",",tty_interface),sizeof(configfile)-1);
		strncat(tty.name,strele(0,":",configfile),sizeof(tty.name)-1);
		strncat(tty.device,strele(1,":",configfile),sizeof(tty.device)-1);
		dprintf(1,"tty: name: %s, device: %s\n", tty.name, tty.device);

		/* load the serial module and test */
		conf->tty = load_module(conf->modules,"serial",SOLARD_MODTYPE_TRANSPORT);
		if (!conf->tty) goto init_error;
		conf->tty_handle = conf->tty->new(conf,tty.device,tty_topts);
		if (!conf->tty_handle) goto init_error;
		if (conf->tty->open(conf->tty_handle)) goto init_error;
		conf->tty->close(conf->tty_handle);

		tty.handle = conf->tty_handle;
		tty.open = conf->tty->open;
		tty.read = conf->tty->read;
		tty.write = conf->tty->write;
		tty.close = conf->tty->close;
		devserver_add_unit(&conf->ds,&tty);
	}

	server(conf,port);

	return 0;
init_error:
	free(conf);
	return 1;
}

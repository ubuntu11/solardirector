
#ifndef __SIPROX_H
#define __SIPROX_H

#include "common.h"
#include "module.h"
#include "devserver.h"
#include <linux/can.h>

struct siproxy_config {
	cfg_info_t *cfg;
	list modules;
	devserver_config_t ds;
	solard_module_t *can;
	void *can_handle;
	pthread_t th;
	struct can_frame frames[16];
	uint32_t bitmap;
	uint16_t state;
};
typedef struct siproxy_config siproxy_config_t;

/* From SI */
#define SI_STATE_RUNNING 0x01
#define SI_STATE_OPEN 0x10
#define si_getshort(v) (short)( ((v)[1]) << 8 | ((v)[0]) )
#define si_putshort(p,v) { float tmp; *((p+1)) = ((int)(tmp = v) >> 8); *((p)) = (int)(tmp = v); }
#define si_putlong(p,v) { *((p+3)) = ((int)(v) >> 24); *((p)+2) = ((int)(v) >> 16); *((p+1)) = ((int)(v) >> 8); *((p)) = (int)(v); }

int server(siproxy_config_t *,int);

#endif

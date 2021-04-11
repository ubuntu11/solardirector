
#ifndef __SMANET_INTERNAL_H
#define __SMANET_INTERNAL_H

#include "smanet.h"
#include "device.h"
#include "types.h"

struct smanet_session {
	list channels;
	int dev_handle;
	solard_module_t *tp;
	void *tp_handle;
	int (*regfunc)(TDevice * newdev);
};

struct smanet_frame_header {
	uint8_t sync;
	uint8_t addr;
	uint8_t proto;
	uint16_t prodid;
};

struct smanet_data_header {
	uint16_t src;
	uint16_t dest;
	uint8_t ctrl;
	uint8_t count;
	uint8_t cmd;
};

/* Global val */
extern smanet_session_t *smanet_session;

/* internal funcs */
smanet_chaninfo_t *_smanet_find_chan(smanet_session_t *s,char *name);

#endif

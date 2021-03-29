
#ifndef __SMANET_INTERNAL_H
#define __SMANET_INTERNAL_H

#include "smanet.h"
#include "device.h"

struct smanet_session {
	list channels;
	solard_module_t *tp;
	void *tp_handle;
	int (*regfunc)(TDevice * newdev);
};

#endif

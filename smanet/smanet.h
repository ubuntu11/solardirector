
#ifndef __SMANET_H
#define __SMANET_H

#include "module.h"

typedef struct smanet_session smanet_session_t;

smanet_session_t *smanet_init(solard_module_t *, void *);

#endif


#ifndef __SMANET_H
#define __SMANET_H

#include "module.h"

typedef struct smanet_session smanet_session_t;

#define SMANET_CHANNEL_NAME_SIZE 32

struct smanet_chaninfo {
	char name[SMANET_CHANNEL_NAME_SIZE];
	enum DATA_TYPE type;
	void *dest;
	int len;
	union {
		char valtext[32];
		char bval;
		short wval;
		long lval;
		float fval;
	};
};
typedef struct smanet_chaninfo smanet_chaninfo_t;

smanet_chaninfo_t *smanet_find_chan(smanet_session_t *,char *name);

smanet_session_t *smanet_init(solard_module_t *, void *);
list smanet_getchanopt(smanet_session_t *s, char *chanName);
int smanet_getparm(smanet_session_t *s, char *name, smanet_chaninfo_t *);
int smanet_setparm(smanet_session_t *s, char *name, char *sval);

#endif

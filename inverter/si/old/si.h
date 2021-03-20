
#ifndef __SOLARD_SI_H
#define __SOLARD_SI_H

#include "agent.h"

struct si_session {
	sdagent_config_t *conf;
	solard_inverter_t *inv;
	pthread_t th;
	uint32_t bitmap;
	uint8_t messages[16][8];
	solard_module_t *tp;
	void *tp_handle;
	int stop;
	int open;
	int (*get_data)(struct si_session *, int id, uint8_t *data, int len);
};
typedef struct si_session si_session_t;

#endif /* __SOLARD_SI_H */

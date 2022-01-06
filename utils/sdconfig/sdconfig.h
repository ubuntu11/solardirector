
#ifndef __SDCONFIG_H
#define __SDCONFIG_H

#include "client.h"
#include "uuid.h"

struct client_agent_info {
	char id[SOLARD_ID_LEN];
	char target[SOLARD_ROLE_LEN+SOLARD_NAME_LEN];
	int status;
	char errmsg[1024];
	json_value_t *info;
	list funcs;
};
typedef struct client_agent_info client_agent_info_t;

void do_list(client_agent_info_t *ap);

#endif /* __SDCONFIG_H */

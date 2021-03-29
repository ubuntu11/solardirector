
#include "solard.h"

int add_agent(solard_config_t *conf, char *name) {
	return 0;
}

int check_agents(solard_config_t *conf) {
	char *info;

	list_reset(conf->agents);
	while((info = list_get_next(conf->agents)) != 0) {
	}
	return 0;
}

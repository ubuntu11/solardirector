
#ifndef __SDAGENT_CONFIG_H
#define __SDAGENT_CONFIG_H

#include "agent.h"

sdagent_config_t *get_config(char *);
int reconfig(sdagent_config_t *conf);

#endif /* __SDAGENT_CONFIG_H */

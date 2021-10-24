
#ifndef __AGENT_CONFIG_H
#define __AGENT_CONFIG_H

int agent_config_add(solard_agent_t *ap,char *label, char *value);
int agent_config_del(solard_agent_t *ap,char *label, char *value);
int agent_config_get(solard_agent_t *ap,char *label, char *value);
int agent_config_set(solard_agent_t *ap,char *label, char *value);
int agent_config_pub(solard_agent_t *ap);

typedef int (agent_config_process_callback_t)(void *,char *,char *,char *,char *);
int agent_config_process(solard_message_t *msg,agent_config_process_callback_t *func,void *ctx,char *errmsg);

#endif

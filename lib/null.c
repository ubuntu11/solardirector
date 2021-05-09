
#include "module.h"

struct null_session {
	solard_agent_t *ap;
};
typedef struct null_session null_session_t;

#if 0
typedef int (*solard_module_init_t)(void *,char *);
typedef void *(*solard_module_new_t)(void *,void *,void *);
typedef json_value_t *(*solard_module_info_t)(void *);
typedef int (*solard_module_open_t)(void *);
typedef int (*solard_module_read_t)(void *,void *,int);
typedef int (*solard_module_write_t)(void *,void *,int);
typedef int (*solard_module_close_t)(void *);
typedef int (*solard_module_free_t)(void *);
typedef int (*solard_module_shutdown_t)(void *);
typedef int (*solard_module_control_t)(void *,char *,char *,json_value_t *);
typedef int (*solard_module_config_t)(void *,char *,char *,list);
typedef void *(*solard_module_get_handle_t)(void *);
typedef void *(*solard_module_get_info_t)(void *);
#endif

void *null_new(void *conf, void *target, void *topts) {
	null_session_t *s;

	s = calloc(1,sizeof(*s));
	if (!s) return 0;
	s->ap = conf;

	return s;
}

int null_openclose(void *handle) {
	return 0;
}

int null_readwrite(void *handle, void *buf, int buflen) {
	/* Cant read/write to buf - can expects id in buflen */
	return 0;
}

solard_module_t null_transport = {
	SOLARD_MODTYPE_TRANSPORT,
	"null",
	0,			/* Init */
	null_new,		/* New */
	0,			/* info */
	null_openclose,		/* Open */
	null_readwrite,		/* Read */
	null_readwrite,		/* Write */
	null_openclose,		/* Close */
	0,			/* Free */
	0,			/* Shutdown */
	0,			/* Control */
	0,			/* Config */
	0,			/* Get handle */
	0,			/* Get private */
};

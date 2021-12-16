
#include "transports.h"

struct null_session {
	void *conf;
	char target[SOLARD_TARGET_LEN];
	char topts[SOLARD_TOPTS_LEN];
};
typedef struct null_session null_session_t;

static void *null_new(void *conf, void *target, void *topts) {
	null_session_t *s;

	s = calloc(1,sizeof(*s));
	if (!s) return 0;
	if (conf) s->conf = conf;
	if (target) strncpy(s->target,target,sizeof(s->target)-1);
	if (topts) strncpy(s->topts,topts,sizeof(s->topts)-1);

	return s;
}

static int null_openclose(void *handle) {
	return 0;
}

static int null_readwrite(void *handle, void *buf, int buflen) {
	/* Cant read/write to buf - can expects id in buflen */
	return 0;
}

static int null_config(void *h, int func, ...) {
	va_list ap;
	int r;

	r = 1;
	va_start(ap,func);
	switch(func) {
	default:
		dprintf(1,"error: unhandled func: %d\n", func);
		break;
	}
	return r;
}

solard_driver_t null_driver = {
	SOLARD_DRIVER_TRANSPORT,
	"null",
	null_new,		/* New */
	0,			/* Destroy */
	null_openclose,		/* Open */
	null_openclose,		/* Close */
	null_readwrite,		/* Read */
	null_readwrite,		/* Write */
	null_config,		/* Config */
};

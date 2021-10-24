
#include "roles.h"

struct norole_session {
	void *ap;
	void *driver;
	void *handle;
};
typedef struct norole_session norole_session_t;

static void *norole_new(void *conf, void *driver, void *handle) {
	norole_session_t *s;

	s = calloc(1,sizeof(*s));
	if (!s) return 0;
	s->ap = conf;
	s->driver = driver;
	s->handle = handle;

	return s;
}

static int norole_openclose(void *handle) {
	return 0;
}

static int norole_readwrite(void *handle, void *buf, int buflen) {
	/* Cant read/write to buf - can expects id in buflen */
	return 0;
}

static int norole_config(void *h, int func, ...) {
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

solard_driver_t norole_driver = {
	SOLARD_DRIVER_ROLE,
	"norole",
	norole_new,		/* New */
	norole_openclose,		/* Open */
	norole_openclose,		/* Close */
	norole_readwrite,		/* Read */
	norole_readwrite,		/* Write */
	norole_config,			/* Config */
};

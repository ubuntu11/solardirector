
#include "si.h"

int si_config(void *h, int req, ...) {
	si_session_t *s = h;
	va_list va;
	void **vp;
	int r;

	r = 1;
	va_start(va,req);
	dprintf(1,"req: %d\n", req);
	switch(req) {
	case SOLARD_CONFIG_GET_DRIVER:
		vp = va_arg(va,void **);
		if (vp) {
			*vp = s->can;
			r = 0;
		}
		break;
	case SOLARD_CONFIG_GET_HANDLE:
		vp = va_arg(va,void **);
		if (vp) {
			*vp = s->can_handle;
			r = 0;
		}
		break;
	}
	dprintf(1,"returning: %d\n", r);
	va_end(va);
	return r;
}

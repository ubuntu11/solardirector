
#include "sb.h"

int sb_read(void *handle,...) {
	sb_session_t *s = handle;

//	s->tp->read(s->tp_handle);
	dprintf(1,"reading...\n");
	return 1;
}

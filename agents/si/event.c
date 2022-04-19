
#include "si.h"

int si_event(si_session_t *s, char *name, int type, void *value, int len) {
	dprintf(0,"name: %s, type: %s, value: %p, len: %d\n", name, typestr(type), value, len);
	return 0;
}

int si_bool_event(si_session_t *s, char *name, bool value) {
	return si_event(s,name,DATA_TYPE_BOOL,&value,sizeof(value));
}

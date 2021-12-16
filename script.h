
#ifndef __SCRIPT_H
#define __SCRIPT_H

#include "jsapi.h"
#include "list.h"

typedef int (jsinitfunc_t)(JSContext *,JSObject *obj,void *);

struct jsinitfuncinfo {
	char *name;
	jsinitfunc_t *func;
	void *private;
};
typedef struct jsinitfuncinfo jsinitfuncinfo_t;

int script_start(JSRuntime  *,char *,list);
void script_stop(void);

#endif

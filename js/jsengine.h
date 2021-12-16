
#ifndef __JSENGINE_H
#define __JSENGINE_H

#include "jsapi.h"
#include "list.h"
#include <pthread.h>

typedef JSObject *(js_initfunc_t)(JSContext *,void *);

struct js_initfuncinfo {
	char *name;
	js_initfunc_t *func;
	void *priv;
};
typedef struct js_initfuncinfo js_initfuncinfo_t;

typedef int (js_outputfunc_t)(const char *format, ...);
struct JSEngine {
	JSRuntime *rt;
	JSContext *cx;
	pthread_mutex_t lockcx;
	int rtsize;
	list initfuncs;
	int stacksize;
	js_outputfunc_t *output;
	list scripts;
};
typedef struct JSEngine JSEngine;

JSEngine *JS_EngineInit(int rtsize, js_outputfunc_t *);
JSEngine *JS_DupEngine(JSEngine *e);
int JS_EngineDestroy(JSEngine *);
int JS_EngineAddInitFunc(JSEngine *, char *name, js_initfunc_t *func, void *priv);
int JS_EngineExec(JSEngine *, char *, int);
int JS_EngineSetStacksize(JSEngine *,int);

#endif

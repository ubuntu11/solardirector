
/*
Copyright (c) 2021, Stephen P. Shoecraft
All rights reserved.

This source code is licensed under the BSD-style license found in the
LICENSE file in the root directory of this source tree.
*/

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/stat.h>
#include <errno.h>

#include "jsengine.h"
#include "jsprf.h"
#include "jsstddef.h"
#include "jsinterp.h"
#include "jsscript.h"
#include "jsdbgapi.h"
#include "jsdtracef.h"
#include "jsobj.h"

#ifdef DEBUG
#undef DEBUG
#endif
#define DEBUG 1
#define dlevel 1
#include "debug.h"

#if 0
struct JSEngine {
	JSRuntime *rt;
	int rtsize;
	JSInitFuncInfo *initfuncs;
	int initfuncs_size;
	int initfuncs_index;
	int stacksize;
};
#endif

struct _scriptinfo {
	JSEngine *e;
	int newcx;
	char filename[256];
	int modtime;
	JSScript *script;
	JSObject *obj;
};
typedef struct _scriptinfo scriptinfo_t;

static time_t _getmodtime(char *filename) {
	struct stat sb;

	dprintf(1,"filename: %s\n", filename);
	if (stat(filename,&sb) < 0) return 0;
	return sb.st_mtime;
}

static scriptinfo_t *_getsinfo(JSEngine *e, char *filename) {
	scriptinfo_t *sp;

	dprintf(1,"filename: %s\n", filename);
	list_reset(e->scripts);
	while((sp = list_get_next(e->scripts)) != 0) {
		dprintf(1,"sp: %p, sp->filename: %s\n", sp, sp->filename);
		if (strcmp(sp->filename,filename) == 0) {
			dprintf(1,"found\n");
			return sp;
		}
	}
	dprintf(1,"NOT found\n");
	return 0;
}

static int _chkscript(JSContext *cx, scriptinfo_t *sp) {
	time_t mt;

//	dprintf(1,"scriptinfo: e: %p, cx: %p, newcx: %d, filename: %s, modtime: %d, script: %p\n",
//		sp->e, sp->cx, sp->newcx, sp->filename, sp->modtime, sp->script);
	dprintf(1,"scriptinfo: sp: %p, e: %p, newcx: %d, filename: %s, modtime: %d, script: %p\n", sp,
		sp->e, sp->newcx, sp->filename, sp->modtime, sp->script);

	mt = _getmodtime(sp->filename);
	dprintf(1,"mt: %ld, modtime: %ld\n", mt, sp->modtime);
	if (!sp->script || sp->modtime != mt) {
		dprintf(1,"script: %p\n", sp->script);
		if (sp->script) JS_DestroyScript(cx, sp->script);
		sp->script = JS_CompileFile(cx, JS_GetGlobalObject(cx), sp->filename);
		dprintf(1,"script: %p\n", sp->script);
		if (!sp->script) return 1;
		sp->obj = JS_NewScriptObject(cx, sp->script);
		if (!sp->obj) {
			JS_DestroyScript(cx, sp->script);
			return 1;
		}
		if (!JS_AddNamedRoot(cx, &sp->obj, sp->filename)) return 1;
		sp->modtime = mt;
	}
	return 0;
}

static void script_error(JSContext *cx, const char *message, JSErrorReport *report) {
	char line[1024], prefix[128], *p;
	JSEngine *e;
	js_outputfunc_t *output;
	int n,i,j,k;

	dprintf(1,"cx: %p, message: %s, report: %p\n", cx, message, report);
	if (!report) return;

	{
		JSClass *classp;

		classp = OBJ_GET_CLASS(cx, JS_GetGlobalObject(cx));
		dprintf(1,"classp: %p\n", classp);
		if (classp) dprintf(1,"name: %s, flags: %02x\n", classp->name, classp->flags);
	}

	/* Get output func */
	dprintf(1,"cx: %p, global: %p\n", cx, JS_GetGlobalObject(cx));
	e = JS_GetPrivate(cx, JS_GetGlobalObject(cx));
	if (!e) {
		JS_ReportError(cx, "global private is null!");
		return;
	}
	output = e->output;

	if (report->filename) sprintf(prefix, "%s(%d): ", report->filename, report->lineno);
	else *prefix = 0;
	output("%s%s\n", prefix, message);

	if (report->linebuf) {
		output("%s%s",prefix,(char *)report->linebuf);
		p = line;
		n = PTRDIFF(report->tokenptr, report->linebuf, char);
		for (i = j = 0; i < n; i++) {
			if (report->linebuf[i] == '\t') {
				for (k = (j + 8) & ~7; j < k; j++) {
					p += sprintf(p,".");
				}
				continue;
			}
			p += sprintf(p,".");
			j++;
		}
		p += sprintf(p,"^\n");
		output("%s%s",prefix,line);
	}
}

static JSContext *_newcx(JSEngine *e) {
	JSContext *cx;
	js_initfuncinfo_t *f;

	if ((cx = JS_NewContext(e->rt, e->stacksize)) == 0) {
		dprintf(1,"error creating context!\n");
		return 0;
	}
	dprintf(1,"cx: %p\n", cx);

//	JS_SetContextCallback(sinfo->rt, ContextCallback);
	JS_SetErrorReporter(cx, script_error);

	if (!JS_CreateGlobalObject(cx, e)) {
		dprintf(1,"error creating global object\n");
		goto _newcx_error;
	}

	/* Add the initfuncs */
	dprintf(1,"adding init funcs...\n");
	list_reset(e->initfuncs);
	while((f = list_get_next(e->initfuncs)) != 0) {
		dprintf(dlevel,"calling: init for %s\n",f->name);
		if (f->func(cx, f->priv) == 0) {
			char msg[1024];

			sprintf(msg,"JSInitFunc %s failed!\n",f->name);
			log_error(msg);
			JS_ReportError(cx,msg);
			goto _newcx_error;
		}
	}
	return cx;

_newcx_error:
	dprintf(1,"cx: %p\n", cx);
	if (cx) {
		dprintf(1,"destroying CX!!\n");
		JS_DestroyContext(cx);
	}
	return 0;
}

static JSContext *_getcx(JSEngine *e) {
	dprintf(1,"locking global cx\n");
	pthread_mutex_lock(&e->lockcx);
	if (!e->cx) e->cx = _newcx(e);
	dprintf(1,"done, returning cx...\n");
	return e->cx;
}

static void _relcx(JSEngine *e) {
	dprintf(1,"unlocking global cx\n");
	pthread_mutex_unlock(&e->lockcx);
}


JSEngine *JS_EngineInit(int rtsize, js_outputfunc_t *output) {
	JSEngine *e;

	dprintf(1,"Initialize JS Engine...\n");

	e = calloc(sizeof(*e),1);
	if (!e) return 0;

	dprintf(1,"rtsize: %d\n", rtsize);
	e->rt = JS_NewRuntime(rtsize);
	if (!e->rt) {
		dprintf(1,"error creating runtime\n");
		goto JS_EngineInit_error;
	}
	e->rtsize = rtsize;
	e->output = output;
//	e->cx = _newcx(e);
	pthread_mutex_init(&e->lockcx, 0);
	e->scripts = list_create();
	e->initfuncs = list_create();
	e->stacksize = 8192;

	return e;

JS_EngineInit_error:
	if (e->rt) JS_DestroyRuntime(e->rt);
	free(e);
	return 0;
}

JSEngine *JS_DupEngine(JSEngine *old) {
	JSEngine *e;

	dprintf(1,"Initialize JS Engine...\n");

	e = calloc(sizeof(*e),1);
	if (!e) return 0;

	dprintf(1,"rtsize: %d\n", old->rtsize);
	e->rt = JS_NewRuntime(old->rtsize);
	if (!e->rt) {
		dprintf(1,"error creating runtime\n");
		goto JS_EngineInit_error;
	}
	e->rtsize = old->rtsize;
	e->output = old->output;
	pthread_mutex_init(&e->lockcx, 0);
	e->scripts = list_create();
	e->initfuncs = list_dup(old->initfuncs);
	e->stacksize = old->stacksize;

	return e;

JS_EngineInit_error:
	if (e->rt) JS_DestroyRuntime(e->rt);
	free(e);
	return 0;
}

int JS_EngineDestroy(JSEngine *e) {
	JS_DestroyRuntime(e->rt);
	free(e);
	return 1;
}

int JS_EngineAddInitFunc(JSEngine *e, char *name, js_initfunc_t *func, void *priv) {
	js_initfuncinfo_t newfunc;

	dprintf(dlevel,"adding: name: %s, func: %p, priv: %p\n", name, func, priv);
	memset(&newfunc,0,sizeof(newfunc));
//	strncpy(newfunc.name,name,sizeof(newfunc.name)-1);
	newfunc.name = name;
	newfunc.func = func;
	newfunc.priv = priv;
	list_add(e->initfuncs, &newfunc, sizeof(newfunc));
#if 0
	info = &e->initfuncs[e->initfuncs_index];
	info->name = name;
	info->func = func;
	info->priv = priv;
	e->initfuncs_index++;
	if (e->initfuncs_index >= e->initfuncs_size) {
		e->initfuncs_size += 16;
		e->initfuncs = realloc(e->initfuncs, sizeof(JSInitFuncInfo)*e->initfuncs_size);
	}
#endif
	return 0;
}

int JS_EngineExec(JSEngine *e, char *filename, int newcx) {
	JSContext *cx;
	scriptinfo_t *sinfo;
	char local_filename[256];
	int status,ok;
	jsval rval;

	dprintf(dlevel,"engine: %p, filename: %s, newcx: %d\n", e, filename, newcx);
	if (access(filename,0) != 0) {
		dprintf(dlevel,"%s does not exist!\n",filename);
		return 1;
	}

	status = 1;

	strncpy(local_filename,filename,sizeof(local_filename)-1);
	if (newcx) cx = _newcx(e);
	else cx = _getcx(e);
	if (!cx) goto JS_EngineExec_error;
	dprintf(1,"cx: %p\n", cx);

	/* If script previously executed, get it */
	sinfo = _getsinfo(e,local_filename);
	dprintf(1,"sinfo: %p\n", sinfo);
	if (!sinfo) {
		scriptinfo_t newsinfo;

		memset(&newsinfo,0,sizeof(newsinfo));
		newsinfo.e = e;
		newsinfo.newcx = newcx;
		strcpy(newsinfo.filename,local_filename);
		sinfo = list_add(e->scripts,&newsinfo,sizeof(newsinfo));
		if (!sinfo) goto JS_EngineExec_error;
		dprintf(1,"sinfo: %p\n", sinfo);
	}
	if (_chkscript(cx, sinfo)) goto JS_EngineExec_error;

	dprintf(dlevel,"executing script...\n");
	dprintf(1,"cx: %p, global: %p, script: %p\n", cx, JS_GetGlobalObject(cx), sinfo->script);
	ok = JS_ExecuteScript(cx, JS_GetGlobalObject(cx), sinfo->script, &rval);
	dprintf(dlevel,"ok: %d, rval: %d\n", ok, (JSVAL_IS_INT(rval) ? JSVAL_TO_INT(rval) : 0));

	status = (ok != 1);

JS_EngineExec_error:
	dprintf(1,"cx: %p, newcx: %d\n", cx, newcx);
	if (cx) {
		if (newcx) {
			dprintf(1,"destroying CX!!\n");
			JS_DestroyContext(cx);
		} else _relcx(e);
	}
	dprintf(1,"status: %d\n", status);
	return status;
}

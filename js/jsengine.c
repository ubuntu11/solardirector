
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

#define DEBUG_JSENGINE 1
#define dlevel 1

#ifdef DEBUG
#undef DEBUG
#endif
#define DEBUG DEBUG_JSENGINE
#include "debug.h"

struct _scriptinfo {
	JSEngine *e;
	char filename[256];
	int modtime;
	JSScript *script;
	JSObject *obj;
	int exitcode;
};
typedef struct _scriptinfo scriptinfo_t;

static time_t _getmodtime(char *filename) {
	struct stat sb;

	dprintf(dlevel,"filename: %s\n", filename);
	if (stat(filename,&sb) < 0) return 0;
	return sb.st_mtime;
}

static scriptinfo_t *_getsinfo(JSEngine *e, char *filename) {
	scriptinfo_t *sp;

	dprintf(dlevel,"filename: %s\n", filename);
	list_reset(e->scripts);
	while((sp = list_get_next(e->scripts)) != 0) {
		dprintf(dlevel,"sp: %p, sp->filename: %s\n", sp, sp->filename);
		if (strcmp(sp->filename,filename) == 0) {
			dprintf(dlevel,"found\n");
			return sp;
		}
	}
	dprintf(dlevel,"NOT found\n");
	return 0;
}

static int _chkscript(JSContext *cx, scriptinfo_t *sp) {
	time_t mt;

//	dprintf(dlevel,"scriptinfo: e: %p, cx: %p, newcx: %d, filename: %s, modtime: %d, script: %p\n",
//		sp->e, sp->cx, sp->newcx, sp->filename, sp->modtime, sp->script);
//	dprintf(dlevel,"scriptinfo: sp: %p, e: %p, newcx: %d, filename: %s, modtime: %d, script: %p\n", sp,
//		sp->e, sp->newcx, sp->filename, sp->modtime, sp->script);
	dprintf(dlevel,"scriptinfo: sp: %p, e: %p, filename: %s, modtime: %d, script: %p\n", sp,
		sp->e, sp->filename, sp->modtime, sp->script);

	mt = _getmodtime(sp->filename);
	dprintf(dlevel,"mt: %ld, modtime: %ld\n", mt, sp->modtime);
	if (!sp->script || sp->modtime != mt) {
		dprintf(dlevel,"script: %p\n", sp->script);
		if (sp->script) JS_DestroyScript(cx, sp->script);
		sp->script = JS_CompileFile(cx, JS_GetGlobalObject(cx), sp->filename);
		dprintf(dlevel,"script: %p\n", sp->script);
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

	dprintf(dlevel,"cx: %p, message: %s, report: %p\n", cx, message, report);
	if (!report) return;

	{
		JSClass *classp;

		classp = OBJ_GET_CLASS(cx, JS_GetGlobalObject(cx));
		dprintf(dlevel,"classp: %p\n", classp);
		if (classp) dprintf(dlevel,"name: %s, flags: %02x\n", classp->name, classp->flags);
	}

	/* Get output func */
	dprintf(dlevel,"cx: %p, global: %p\n", cx, JS_GetGlobalObject(cx));
	e = JS_GetPrivate(cx, JS_GetGlobalObject(cx));
	if (!e) {
		JS_ReportError(cx, "global private is null!");
		return;
	}
	output = e->output;
	if (!output) output = printf;

	if (report->filename) sprintf(prefix, "%s(%d): ", report->filename, report->lineno);
	else *prefix = 0;
	output("%s%s\n", prefix, message);
	sprintf(e->errmsg,"%s%s", prefix, message);

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

JSContext *JS_EngineNewContext(JSEngine *e) {
	JSContext *cx;
	js_initfuncinfo_t *f;
	JSObject *gobj,*newobj;

	if ((cx = JS_NewContext(e->rt, e->stacksize)) == 0) {
		dprintf(dlevel,"error creating context!\n");
		return 0;
	}
	dprintf(dlevel,"cx: %p\n", cx);

//	JS_SetContextCallback(sinfo->rt, ContextCallback);
	JS_SetErrorReporter(cx, script_error);

	gobj = JS_CreateGlobalObject(cx, e);
	if (!gobj) {
		dprintf(dlevel,"error creating global object\n");
		goto JS_EngineNewContext_error;
	}

	/* Add the initfuncs */
	dprintf(dlevel,"adding init funcs...\n");
	list_reset(e->initfuncs);
	while((f = list_get_next(e->initfuncs)) != 0) {
		dprintf(dlevel,"calling: init for %s\n",f->name);
		if (f->type == 0) newobj = f->func(cx, f->priv);
		else if (f->type == 1) newobj = f->class(cx, gobj);
		if (!newobj) {
			char msg[128];

			sprintf(msg,"JSInitFunc %s failed!\n",f->name);
			log_error(msg);
			JS_ReportError(cx,msg);
			goto JS_EngineNewContext_error;
		}
	}
	return cx;

JS_EngineNewContext_error:
	dprintf(dlevel,"cx: %p\n", cx);
	if (cx) {
		dprintf(dlevel,"destroying CX!!\n");
		JS_DestroyContext(cx);
	}
	return 0;
}

static JSContext *_getcx(JSEngine *e) {
	dprintf(dlevel,"locking global cx\n");
	pthread_mutex_lock(&e->lockcx);
	if (!e->cx) e->cx = JS_EngineNewContext(e);
	dprintf(dlevel,"done, returning cx...\n");
	return e->cx;
}

static void _relcx(JSEngine *e) {
	dprintf(dlevel,"unlocking global cx\n");
	pthread_mutex_unlock(&e->lockcx);
}


JSEngine *JS_EngineInit(int rtsize, js_outputfunc_t *output) {
	JSEngine *e;

	dprintf(dlevel,"Initialize JS Engine...\n");

	e = calloc(sizeof(*e),1);
	if (!e) return 0;

	dprintf(dlevel,"rtsize: %d\n", rtsize);
	e->rt = JS_NewRuntime(rtsize);
	if (!e->rt) {
		dprintf(dlevel,"error creating runtime\n");
		goto JS_EngineInit_error;
	}
	e->rtsize = rtsize;
	e->output = output;
	e->scripts = list_create();
	e->initfuncs = list_create();
	pthread_mutex_init(&e->lockcx, 0);
	e->stacksize = 4096;
//	e->cx = JS_EngineNewContext(e);

	return e;

JS_EngineInit_error:
	if (e->rt) JS_DestroyRuntime(e->rt);
	free(e);
	return 0;
}

JSEngine *JS_DupEngine(JSEngine *old) {
	JSEngine *e;

	dprintf(dlevel,"Initialize JS Engine...\n");

	e = calloc(sizeof(*e),1);
	if (!e) return 0;

	dprintf(dlevel,"rtsize: %d\n", old->rtsize);
	e->rt = JS_NewRuntime(old->rtsize);
	if (!e->rt) {
		dprintf(dlevel,"error creating runtime\n");
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

#if 0
int JS_EngineAddObject(JSEngine *e, jsobjinit_t *func, void *priv) {
	jsval rval;
	JSContext *cx;
	int ok;

	cx = _getcx(e);
	if (!cx) return 1;
	func(cx, priv);
	_relcx(e);
	return (ok ? 0 : 1);
}
#endif

int JS_EngineAddInitFunc(JSEngine *e, char *name, js_initfunc_t *func, void *priv) {
	js_initfuncinfo_t newfunc;

	dprintf(dlevel,"adding: name: %s, func: %p, priv: %p\n", name, func, priv);
	memset(&newfunc,0,sizeof(newfunc));
	newfunc.name = malloc(strlen(name)+1);
	if (newfunc.name) strncpy(newfunc.name,name,sizeof(newfunc.name)-1);
	else newfunc.name = name;
	newfunc.type = 0;
	newfunc.func = func;
	newfunc.priv = priv;
	list_add(e->initfuncs, &newfunc, sizeof(newfunc));
	return 0;
}

int JS_EngineAddInitClass(JSEngine *e, char *name, js_initclass_t *class) {
	js_initfuncinfo_t newfunc;

	dprintf(dlevel,"adding: name: %s, class: %p\n", name, class);
	memset(&newfunc,0,sizeof(newfunc));
	newfunc.name = malloc(strlen(name)+1);
	if (newfunc.name) strncpy(newfunc.name,name,sizeof(newfunc.name)-1);
	else newfunc.name = name;
	newfunc.type = 1;
	newfunc.class = class;
	list_add(e->initfuncs, &newfunc, sizeof(newfunc));
	return 0;
}

int _JS_EngineExec(JSEngine *e, char *filename, JSContext *cx) {
	scriptinfo_t *sinfo;
	int status,ok;
	jsval rval;

	/* If script previously executed, get it */
	sinfo = _getsinfo(e,filename);
	dprintf(dlevel,"sinfo: %p\n", sinfo);
	if (!sinfo) {
		scriptinfo_t newsinfo;

		memset(&newsinfo,0,sizeof(newsinfo));
		newsinfo.e = e;
		strcpy(newsinfo.filename,filename);
		sinfo = list_add(e->scripts,&newsinfo,sizeof(newsinfo));
		if (!sinfo) return 1;
		dprintf(dlevel,"sinfo: %p\n", sinfo);
	}
	if (_chkscript(cx, sinfo)) return 1;

	dprintf(dlevel,"executing script: %s\n", filename);
	dprintf(dlevel,"cx: %p, global: %p, script: %p\n", cx, JS_GetGlobalObject(cx), sinfo->script);
	ok = JS_ExecuteScript(cx, JS_GetGlobalObject(cx), sinfo->script, &rval);
	dprintf(dlevel,"%s: ok: %d\n", filename, ok);

	dprintf(dlevel,"getting exit code...\n");
	JS_GetProperty(cx, JS_GetGlobalObject(cx), "exit_code", &rval);
	if (JSVAL_IS_VOID(rval)) {
		dprintf(dlevel,"using ok...\n");
		status = (ok != 1);
	} else {
		JS_ValueToInt32(cx,rval,&status);
	}

	dprintf(dlevel,"status: %d\n", status);
	return status;
}

int JS_EngineExec(JSEngine *e, char *filename, int newcx) {
	JSContext *cx;
	char local_filename[256];
	int r;

	dprintf(dlevel,"engine: %p, filename: %s, newcx: %d\n", e, filename, newcx);
	if (access(filename,0) != 0) {
		dprintf(dlevel,"%s does not exist!\n",filename);
		return 1;
	}
	r = 1;
	strncpy(local_filename,filename,sizeof(local_filename)-1);
	fixpath(local_filename,sizeof(local_filename)-1);
	if (newcx) cx = JS_EngineNewContext(e);
	else cx = _getcx(e);
	dprintf(dlevel,"cx: %p\n", cx);
	if (!cx) goto JS_EngineExec_error;
	r = _JS_EngineExec(e, filename, cx);

JS_EngineExec_error:
	dprintf(dlevel,"cx: %p, newcx: %d\n", cx, newcx);
	if (newcx && cx) {
		dprintf(dlevel,"destroying CX!!\n");
		JS_DestroyContext(cx);
	} else _relcx(e);
	return r;
}


int JS_EngineExecString(JSEngine *e, char *string) {
	jsval rval;
	JSContext *cx;
	int ok;

	cx = _getcx(e);
	if (!cx) return 1;
	ok = JS_EvaluateScript(cx, JS_GetGlobalObject(cx), string, strlen(string),"-e", 1, &rval);
	_relcx(e);
	return (ok ? 0 : 1);
}

char *JS_EngineGetErrmsg(JSEngine *e) {
	if (!e) return "";
	return e->errmsg;
}

void JS_EngineCleanup(JSEngine *e) {
	JSContext *cx;

	cx = _getcx(e);
	if (!cx) return;
	JS_GC(cx);
	_relcx(e);
	return;
}

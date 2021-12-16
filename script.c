
/*
Copyright (c) 2021, Stephen P. Shoecraft
All rights reserved.

This source code is licensed under the BSD-style license found in the
LICENSE file in the root directory of this source tree.
*/

#include "common.h"
#include "script.h"
#include <sys/stat.h>
#include <sys/types.h>
#include <utime.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include "debug.h"
#include "jsstddef.h"
#include "jscntxt.h"
#include "jsinterp.h"
#include "jsscript.h"
#include "jsdbgapi.h"
#include "jsprf.h"
#ifdef WINDOWS
#include <windows.h>
#define os_utime _utime
typedef strcut _utimbuf os_utimbuf_t;
#else
#define os_utime utime
typedef struct utimbuf os_utimbuf_t;
#endif
#include <pthread.h>

#define dlevel 1

#define STACK_SIZE 8192

#if 0
static uint32 gBranchCount;
static uint32 gBranchLimit;
#endif

int stop = 0;

static int jsprint(JSContext *cx, JSObject *obj, uintN argc, jsval *argv, jsval *rval) {
	char msg[1024];
	register u_int i;

	msg[0] = 0;
	for(i = 0; i < argc; i++)	{
//		if (JSVAL_IS_INT(argv[i]) || JSVAL_IS_DOUBLE(argv[i]) || JSVAL_IS_STRING(argv[i]) || JSVAL_IS_BOOLEAN(argv[i]))
			strcat(msg,JS_GetStringBytes(JSVAL_TO_STRING(argv[i])));
	}
	printf("\n*****\n%s\n\n",msg);
	return JS_TRUE;
}

static JSBool Print(JSContext *cx, JSObject *obj, uintN argc, jsval *argv, jsval *rval) {
    uintN i;
    JSString *str;
    char *bytes;

    for (i = 0; i < argc; i++) {
        str = JS_ValueToString(cx, argv[i]);
        if (!str)
            return JS_FALSE;
        bytes = JS_EncodeString(cx, str);
        if (!bytes)
            return JS_FALSE;
        printf("%s%s", i ? " " : "", bytes);
        JS_free(cx, bytes);
    }

	printf("\n");

    return JS_TRUE;
}

static int jserror(JSContext *cx, JSObject *obj, uintN argc, jsval *argv, jsval *rval) {
	char msg[1024];
	register u_int i;

	if (stop) return JS_FALSE;
//	dprintf(1,"jserror");

	msg[0] = 0;
	for(i = 0; i < argc; i++)	{
		if (JSVAL_IS_STRING(argv[i]))
			strcat(msg,JS_GetStringBytes(JSVAL_TO_STRING(argv[i])));
	}
	JS_ReportError(cx,msg);
	return JS_TRUE;
}

static int jsexit(JSContext *cx, JSObject *obj, uintN argc, jsval *argv, jsval *rval) {
	int status;

	if (stop) return JS_FALSE;
	dprintf(1,"jsExit");

	status = (argc ? JSVAL_TO_INT(argv[0]) : 0);

	dprintf(1,"status: %d\n", status);
	exit(status);
}

#if 0
static int jsinclude(JSContext *cx, JSObject *obj, uintN argc, jsval *argv, jsval *rval) {
	char *name;
	jsval status;

	if (stop) return JS_FALSE;
	dprintf(1,"argc: %d, argv: %p\n",argc,argv);

	dprintf(1,"argc: %d", argc);
	if (!argc) return JS_TRUE;

	name = JS_GetStringBytes(JS_ValueToString(cx, argv[0]));
	dprintf(1,"name: %s", name);

	status = script_include(cx, obj, name);
	return JS_TRUE;
}
#endif

// exec a new script in the same cx
int script_run(JSContext *cx, JSObject *obj, char *name, int argc, jsval *argv) {
	JSObject *newobj;
	JSScript *script;
	int status;
	JSBool ok;
	jsval rval;

	dprintf(dlevel,"name: %s", name);

	script = JS_CompileFile(cx, obj, name);
	if (!script) {
		log_error("unable to load script");
		return 1;
	}
	newobj = JS_NewScriptObject(cx, script);
	ok = JS_ExecuteScript(cx, newobj, script, &rval);
	dprintf(1,"ok: %d\n", ok);
	ok = JS_CallFunctionName(cx, newobj, "main", 0, 0, &rval);
	dprintf(1,"ok: %d\n", ok);
	dprintf(1,"rval: %d\n", rval);
	status = (ok ? 0 : -1);

	dprintf(1,"status: %d\n", status);
	return status;
}

static int jsRun(JSContext *cx, JSObject *obj, uintN argc, jsval *argv, jsval *rval) {
	char *name;

	if (stop) return JS_FALSE;

	name = JS_GetStringBytes(JS_ValueToString(cx, argv[0]));
	*rval = INT_TO_JSVAL(script_run(cx,obj,name,argc-1,&argv[1]));

	return JS_TRUE;
}

int script_include(JSContext *cx, JSObject *obj, uintN argc, jsval *argv, jsval *rval) {
    uintN i;
    JSString *str;
    const char *filename;
    JSScript *script;
    JSBool ok;
    jsval result;
    uint32 oldopts;

    for (i = 0; i < argc; i++) {
        str = JS_ValueToString(cx, argv[i]);
        if (!str) return JS_FALSE;
        argv[i] = STRING_TO_JSVAL(str);
        filename = JS_GetStringBytes(str);
	dprintf(1,"filename: %s\n", filename);
        errno = 0;
        oldopts = JS_GetOptions(cx);
        JS_SetOptions(cx, oldopts | JSOPTION_COMPILE_N_GO);
        script = JS_CompileFile(cx, obj, filename);
	dprintf(1,"script: %p\n", script);
        if (!script) {
		ok = JS_FALSE;
        } else {
		ok = JS_ExecuteScript(cx, obj, script, &result);
		JS_DestroyScript(cx, script);
        }
        JS_SetOptions(cx, oldopts);
	dprintf(1,"ok: %d\n", ok);
        if (!ok) return JS_FALSE;
    }

    return JS_TRUE;
}
JSClass global_class;

#if 0
JSClass global_class = {
    "global", JSCLASS_NEW_RESOLVE | JSCLASS_GLOBAL_FLAGS,
    JS_PropertyStub,  JS_PropertyStub,
    JS_PropertyStub,  JS_PropertyStub,
    global_enumerate, (JSResolveOp) global_resolve,
    JS_ConvertStub,   JS_FinalizeStub,
    JSCLASS_NO_OPTIONAL_MEMBERS
};
#endif

#if 0
/* core script functions */
static JSClass global_class = {
	"global", 0,
	JS_PropertyStub, JS_PropertyStub, JS_PropertyStub, JS_PropertyStub,
	JS_EnumerateStub, JS_ResolveStub, JS_ConvertStub, JS_FinalizeStub,
	JSCLASS_NO_OPTIONAL_MEMBERS
};
#endif

	JSFunctionSpec core_functions[] = {
		{ "main",				0,				0 },
		JS_FS("load",script_include,1,0,0),
		JS_FS("include",script_include,1,0,0),
		JS_FS("print",          Print,          0,0,0),
		JS_FS("run",		jsRun,		0,0,0),
		JS_FS("error",		jserror,	0,0,0),
		JS_FS("exit",		jsexit,	0,0,0),
		{0}
	};

enum {
	SOLARD_BINDIR_ID,
	SOLARD_ETCDIR_ID,
	SOLARD_LIBDIR_ID,
	SOLARD_LOGDIR_ID,
	GLOBAL_SCRIPT_NAME_PROPID,
};

static const char *get_script_name(JSContext *cx) {
	JSStackFrame *fp;

	/* Get the currently executing script's name. */
	fp = JS_GetScriptedCaller(cx, NULL);
	if (!fp || !fp->script || !fp->script->filename) {
		dprintf(1,"unable to get current script!\n");
		return "unknown";
	}
	return(fp->script->filename);
}

static JSBool global_getprop(JSContext *cx, JSObject *obj, jsval id, jsval *rval) {
	int prop_id;

	dprintf(1,"is_int: %d\n", JSVAL_IS_INT(id));
	if(JSVAL_IS_INT(id)) {
		prop_id = JSVAL_TO_INT(id);
		dprintf(1,"prop_id: %d\n", prop_id);
		switch(prop_id) {
		case SOLARD_BINDIR_ID:
			*rval = STRING_TO_JSVAL(JS_NewStringCopyZ(cx,SOLARD_BINDIR));
			break;
		case SOLARD_ETCDIR_ID:
			*rval = STRING_TO_JSVAL(JS_NewStringCopyZ(cx,SOLARD_ETCDIR));
			break;
		case SOLARD_LIBDIR_ID:
			*rval = STRING_TO_JSVAL(JS_NewStringCopyZ(cx,SOLARD_LIBDIR));
			break;
		case SOLARD_LOGDIR_ID:
			*rval = STRING_TO_JSVAL(JS_NewStringCopyZ(cx,SOLARD_LOGDIR));
			break;
		case GLOBAL_SCRIPT_NAME_PROPID:
			*rval = STRING_TO_JSVAL(JS_NewStringCopyZ(cx,get_script_name(cx)));
			break;
		default:
			*rval = JSVAL_NULL;
		}
	}
	return JS_TRUE;
}

#if 0
void script_error(JSContext *cx, const char *message, JSErrorReport *report) {
	if (message && report) {
		log_error("%s(%d): %s", report->filename, report->lineno, message);
	} else {
		log_error("%s\n",message);
	}
	return;
}
#endif

#define JSREPORT_IS_WARNING(flags)      (((flags) & JSREPORT_WARNING) != 0)

static void script_error(JSContext *cx, const char *message, JSErrorReport *report)
{
    int i, j, k, n;
    char *prefix, *tmp;
    const char *ctmp;
	FILE *gErrFile = stdout;
	int reportWarnings = 1;

    if (!report) {
        dprintf(1, "%s\n", message);
        return;
    }

    /* Conditionally ignore reported warnings. */
    if (JSREPORT_IS_WARNING(report->flags) && !reportWarnings)
        return;

    prefix = NULL;
    if (report->filename)
        prefix = JS_smprintf("%s:", report->filename);
    if (report->lineno) {
        tmp = prefix;
        prefix = JS_smprintf("%s%u: ", tmp ? tmp : "", report->lineno);
        JS_free(cx, tmp);
    }
    if (JSREPORT_IS_WARNING(report->flags)) {
        tmp = prefix;
        prefix = JS_smprintf("%s%swarning: ", tmp ? tmp : "", JSREPORT_IS_STRICT(report->flags) ? "strict " : "");
        JS_free(cx, tmp);
    }

    /* embedded newlines -- argh! */
    while ((ctmp = strchr(message, '\n')) != 0) {
        ctmp++;
        if (prefix) fputs(prefix, gErrFile);
        fwrite(message, 1, ctmp - message, gErrFile);
        message = ctmp;
    }

    /* If there were no filename or lineno, the prefix might be empty */
    if (prefix)
        fputs(prefix, gErrFile);
    fputs(message, gErrFile);

    if (!report->linebuf) {
        fputc('\n', gErrFile);
        goto out;
    }

    /* report->linebuf usually ends with a newline. */
    n = strlen(report->linebuf);
    fprintf(gErrFile, ":\n%s%s%s%s",
            prefix,
            report->linebuf,
            (n > 0 && report->linebuf[n-1] == '\n') ? "" : "\n",
            prefix);
    n = PTRDIFF(report->tokenptr, report->linebuf, char);
    for (i = j = 0; i < n; i++) {
        if (report->linebuf[i] == '\t') {
            for (k = (j + 8) & ~7; j < k; j++) {
                fputc('.', gErrFile);
            }
            continue;
        }
        fputc('.', gErrFile);
        j++;
    }
    fputs("^\n", gErrFile);
 out:
    if (!JSREPORT_IS_WARNING(report->flags)) {
        if (report->errorNumber == JSMSG_OUT_OF_MEMORY) {
		dprintf(1,"OUT OF MEMORY!\n");
        } else {
		dprintf(1,"runtime error!\n");
        }
    }
    JS_free(cx, prefix);
}

/* Create runtime, initial context, global object */
int script_init(JSRuntime *rt, list initfuncs) {
	JSPropertySpec solard_props[] = {
		{ "SOLARD_BINDIR",	SOLARD_BINDIR_ID,	JSPROP_ENUMERATE | JSPROP_READONLY },
		{ "SOLARD_ETCDIR",	SOLARD_ETCDIR_ID,	JSPROP_ENUMERATE | JSPROP_READONLY },
		{ "SOLARD_LIBDIR",	SOLARD_LIBDIR_ID,	JSPROP_ENUMERATE | JSPROP_READONLY },
		{ "SOLARD_LOGDIR",	SOLARD_LOGDIR_ID,	JSPROP_ENUMERATE | JSPROP_READONLY },
		{ "script_name",	GLOBAL_SCRIPT_NAME_PROPID,	JSPROP_ENUMERATE | JSPROP_READONLY },
                {0}
        };
	JSContext *cx;
	JSObject *obj;
	jsinitfuncinfo_t *finfo;
	int status;

	dprintf(1,"Adding core objects...\n");

	status = 1;

	if ((cx = JS_NewContext(rt, 8L*1024L*1024L)) == 0) goto script_init_error;

//	JS_SetContextCallback(sinfo->rt, ContextCallback);
	JS_SetErrorReporter(cx, script_error);
//        JS_ToggleOptions(cx, JSOPTION_NATIVE_BRANCH_CALLBACK);

	/* Create the global object */
	if ((obj = JS_NewObject(cx, &global_class, 0, 0)) == 0) goto script_init_error;

	/* Add standard classes */
	if(!JS_InitStandardClasses(cx, obj)) goto script_init_error;

	/* Define global props/funcs */
	if(!JS_DefineProperties(cx, obj, solard_props)) goto script_init_error;
	if(!JS_DefineFunctions(cx, obj, core_functions)) goto script_init_error;

	/* Init driver/modules */
	list_reset(initfuncs);
	while((finfo = list_get_next(initfuncs)) != 0) {
		dprintf(1,"calling: %s_jsinit\n",finfo->name);
		if (finfo->func(cx, obj, finfo->private)) {
			log_error("jsinit function for %s failed!\n",finfo->name);
			goto script_init_error;
		}
	}
	status = 0;

script_init_error:
	if (cx) JS_DestroyContext(cx);
	return status;
}

static int _script_exec(JSContext *cx, JSObject *obj, char *name, char *func, int argc, jsval *argv) {
	JSScript *script;
	int ok,status;
	jsval rval;

	dprintf(dlevel,"cx: %p, obj: %p, name: %s, func: %s, argc: %d, argv: %p", cx, obj, name, func, argc, argv);
	if (!name) return 1;

	script = JS_CompileFile(cx, obj, name);
	dprintf(1,"script: %p\n", script);
	status = -1;
	if (script) {
		ok = JS_ExecuteScript(cx, obj, script, &rval);
		dprintf(1,"ok: %d\n", ok);
		ok = 1;
		if (ok) {
			if (func) {
				ok = JS_GetProperty(cx, obj, func, &rval);
				dprintf(1,"ok: %d\n", ok);
				if (ok) {
					dprintf(1,"isvoid: %d\n", JSVAL_IS_VOID(rval));
					if (JSVAL_IS_VOID(rval)) {
						dprintf(1,"function not found!\n");
					} else {
					dprintf(dlevel,"calling %s %s\n", name, func);
					if (JS_CallFunctionName(cx, obj, func, argc, argv, &rval) == JS_FALSE) {
						dprintf(dlevel,"error calling function!");
						status = -1;
					} else {
						status = JSVAL_TO_INT(rval);
					}
					}
				} else {
					dprintf(1,"unable to get property!\n");
				}
			}
		} else {
			dprintf(1,"error executing script!\n");
		}
		JS_DestroyScript(cx, script);
	} else {
		log_error("_script_exec: %s: failed to compile!\n", name);
	}
	dprintf(1,"status: %d\n", status);

	dprintf(dlevel,"_script_exec: %s: status: %d", name, status);
	return status;
}

#if 0
        jsval argv[5];
        argv[0] = OBJECT_TO_JSVAL(screenArrayBuffer);
        argv[1] = INT_TO_JSVAL(WIDTH);
        argv[2] = INT_TO_JSVAL(HEIGHT);
        argv[3] = INT_TO_JSVAL(screen->pitch);
        argv[4] = INT_TO_JSVAL(BPP);
        JSBool ok = JS_CallFunctionName(cx, global, "paint", 5, argv, &rval);
#endif


struct start_info {
	JSRuntime *rt;
	list initfuncs;
	char name[256];
	char func[64];
	int argc;
	void *args;
	int nargs;
	jsval *argv;
};

/* Start a new instance of a script */
static void *_script_start(void *ctx) {
	struct start_info *sinfo = ctx;
	JSContext *cx;
	int status;

	dprintf(dlevel,"info: %p",sinfo);
	if (!sinfo) return 0;
	dprintf(dlevel,"sinfo: rt: %p, initfuncs: %p, name: %s, func: %s", sinfo->rt, sinfo->initfuncs, sinfo->name, sinfo->func);

	status = 1;
	if (script_init(sinfo->rt,sinfo->initfuncs)) goto _script_start_error;

	// Exec the script
	status = _script_exec(cx, JS_GetGlobalObject(cx), sinfo->name,
		(strlen(sinfo->func) ? sinfo->func : 0), sinfo->argc, sinfo->argv);
//	status = _script_exec(cx, obj, info->name, (strlen(info->func) ? info->func : 0), info->argc, info->argv);
	dprintf(1,"status: %d\n",status);
	JS_GC(cx);

_script_start_error:
	free(sinfo);
	dprintf(dlevel,"_script_start: status: %d",status);

//	thread_end(status == JS_TRUE ? 0 : 1);
#if 0
	{
		pthread_t tid;
		void *sptr;
		tid = pthread_self();
		if (tid) {
			pthread_cancel(tid);
			pthread_join(tid,&sptr);
		}
	}
#endif
	return 0;
}

// exec a script with a new cx and new thread
int script_start(JSRuntime *rt, char *name, list initfuncs) {
	struct start_info *sinfo;
//	pthread_t tid;

	dprintf(1,"name: %s\n", name);
	if (!name) return 1;
	if (access(name,0) != 0) {
		log_syserror("unable to start script %s:",name);
		return 1;
	}

	sinfo = calloc(sizeof(*sinfo),1);
	if (!sinfo) return 1;
	sinfo->rt = rt;
	sinfo->initfuncs = initfuncs;
	strcpy(sinfo->name,name);
	strcpy(sinfo->func,"main");

//	thread_start("script",_script_start,ptr,THREAD_PRIORITY_NORMAL);
//	pthread_create(&tid,NULL,_script_start,info);
	_script_start(sinfo);
	dprintf(dlevel,"done!");
	return 0;
}

void script_stop(void) {

	dprintf(dlevel,"script_stop");

	// Set stop flag
//	stop = 1;

	// Let functions abort
//	Delay(100);

	// Kill thread(s)
//	thread_stop("script");
}

static JSBool Load(JSContext *cx, JSObject *obj, uintN argc, jsval *argv, jsval *rval)
{
    uintN i;
    JSString *str;
    const char *filename;
    JSScript *script;
    JSBool ok;
    jsval result;
    uint32 oldopts;

    for (i = 0; i < argc; i++) {
        str = JS_ValueToString(cx, argv[i]);
        if (!str)
            return JS_FALSE;
        argv[i] = STRING_TO_JSVAL(str);
        filename = JS_GetStringBytes(str);
        errno = 0;
        oldopts = JS_GetOptions(cx);
        JS_SetOptions(cx, oldopts | JSOPTION_COMPILE_N_GO);
        script = JS_CompileFile(cx, obj, filename);
        if (!script) {
            ok = JS_FALSE;
        } else {
		ok = JS_ExecuteScript(cx, obj, script, &result);
            JS_DestroyScript(cx, script);
        }
        JS_SetOptions(cx, oldopts);
        if (!ok) return JS_FALSE;
    }

    return JS_TRUE;
}

static JSFunctionSpec shell_functions[] = {
#if 0
    JS_FS("version",        Version,        0,0,0),
    JS_FS("options",        Options,        0,0,0),
#endif
    JS_FS("load",           Load,           1,0,0),
    JS_FS("run",           jsRun,           1,0,0),
#if 0
    JS_FN("readline",       ReadLine,       0,0,0),
#endif
    JS_FS("print",          Print,          0,0,0),
#if 0
    JS_FS("help",           Help,           0,0,0),
    JS_FS("exit",           Quit,           0,0,0),
    JS_FS("quit",           Quit,           0,0,0),
    JS_FN("gc",             GC,             0,0,0),
    JS_FN("gcparam",        GCParameter,    2,2,0),
    JS_FN("countHeap",      CountHeap,      0,0,0),
#ifdef JS_GC_ZEAL
    JS_FN("gczeal",         GCZeal,         1,1,0),
#endif
    JS_FS("trap",           Trap,           3,0,0),
    JS_FS("untrap",         Untrap,         2,0,0),
    JS_FS("line2pc",        LineToPC,       0,0,0),
    JS_FS("pc2line",        PCToLine,       0,0,0),
    JS_FN("stackQuota",     StackQuota,     0,0,0),
    JS_FS("stringsAreUTF8", StringsAreUTF8, 0,0,0),
    JS_FS("testUTF8",       TestUTF8,       1,0,0),
    JS_FS("throwError",     ThrowError,     0,0,0),
#ifdef DEBUG
    JS_FS("dis",            Disassemble,    1,0,0),
    JS_FS("dissrc",         DisassWithSrc,  1,0,0),
    JS_FN("dumpHeap",       DumpHeap,       0,0,0),
    JS_FS("notes",          Notes,          1,0,0),
    JS_FS("tracing",        Tracing,        0,0,0),
    JS_FS("stats",          DumpStats,      1,0,0),
#endif
#ifdef TEST_EXPORT
    JS_FS("export",          DoExport,       2,0,0),
#endif
#ifdef TEST_CVTARGS
    JS_FS("cvtargs",        ConvertArgs,    0,0,12),
#endif
    JS_FN("build",          BuildDate,      0,0,0),
    JS_FS("clear",          Clear,          0,0,0),
    JS_FN("intern",         Intern,         1,1,0),
    JS_FS("clone",          Clone,          1,0,0),
    JS_FS("seal",           Seal,           1,0,1),
    JS_FN("getpda",         GetPDA,         1,1,0),
    JS_FN("getslx",         GetSLX,         1,1,0),
    JS_FN("toint32",        ToInt32,        1,1,0),
    JS_FS("evalcx",         EvalInContext,  1,0,0),
#ifdef MOZ_SHARK
    JS_FS("startShark",      js_StartShark,      0,0,0),
    JS_FS("stopShark",       js_StopShark,       0,0,0),
    JS_FS("connectShark",    js_ConnectShark,    0,0,0),
    JS_FS("disconnectShark", js_DisconnectShark, 0,0,0),
#endif
#ifdef DEBUG_ARRAYS
    JS_FS("arrayInfo",       js_ArrayInfo,       1,0,0),
#endif
#ifdef JS_THREADSAFE
    JS_FN("sleep",          Sleep,          1,1,0),
    JS_FN("scatter",        Scatter,        1,1,0),
#endif
#endif
    JS_FS_END
};

static JSBool
global_enumerate(JSContext *cx, JSObject *obj)
{
    return JS_EnumerateStandardClasses(cx, obj);
}

static JSBool
global_resolve(JSContext *cx, JSObject *obj, jsval id, uintN flags,
               JSObject **objp)
{
    if ((flags & JSRESOLVE_ASSIGNING) == 0) {
        JSBool resolved;

        if (!JS_ResolveStandardClass(cx, obj, id, &resolved))
            return JS_FALSE;
        if (resolved) {
            *objp = obj;
            return JS_TRUE;
        }
    }

    return JS_TRUE;
}

JSClass global_class = {
	"global",		/* Name */
	JSCLASS_NEW_RESOLVE | JSCLASS_GLOBAL_FLAGS, /* Flags */
	JS_PropertyStub,	/* addProperty */
	JS_PropertyStub,	/* delProperty */
	global_getprop	,	/* getProperty */
	JS_PropertyStub,	/* setProperty */
	global_enumerate,	/* enumerate */
	(JSResolveOp) global_resolve, /* resolve */
	JS_ConvertStub,		/* convert */
	JS_FinalizeStub,	/* finalize */
	JSCLASS_NO_OPTIONAL_MEMBERS
};

int script_startO(JSRuntime *rt, char *name) {
	JSContext *cx;
	JSObject *gobj;
	JSScript *script;
	jsval rval;

	cx = JS_NewContext(rt, 8192);
	if (!cx) return 1;

	if ((gobj = JS_NewObject(cx, &global_class, NULL, NULL)) == 0) return 1;
	JS_SetGlobalObject(cx, gobj);
	JS_SetErrorReporter(cx, script_error);
	if (!JS_DefineFunctions(cx, gobj, shell_functions)) return 1;

	script = JS_CompileFile(cx, gobj, name);
	dprintf(1,"script: %p\n", script);
	if (script) {
		(void)JS_ExecuteScript(cx, gobj, script, &rval);
		JS_DestroyScript(cx, script);
	}

	JS_DestroyContext(cx);
	return 0;
}

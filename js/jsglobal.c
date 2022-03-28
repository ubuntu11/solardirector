
#include <string.h>
#include <stdlib.h>
#ifdef WINDOWS
#include <winsock2.h>
#include <windows.h>
#else
#include <unistd.h>
#endif
#include <libgen.h>

#include "jsapi.h"
#include "jscntxt.h"
#include "jsengine.h"
#include "jsprintf.h"
#include "jsprf.h"
#include "jsstddef.h"
#include "jsinterp.h"
#include "jsscript.h"
#include "jsdbgapi.h"
#include "jsdtracef.h"
#include "jsengine.h"
#include "jsscan.h"
#include "jsarena.h"
#include "jsgc.h"

#define DEBUG_GLOBAL 1
#define dlevel 6

#ifdef DEBUG
#undef DEBUG
#endif
#if DEBUG_GLOBAL
#define DEBUG 1
#endif
#include "debug.h"

static int version = 182;
#define VERSION_MAJOR 1
#define VERSION_MINOR 8
#define VERSION_RELEASE 2
#define XSTR(n) #n
#define STR(n) XSTR(n)
#define VERSION_STRING STR(VERSION_MAJOR)"."STR(VERSION_MINOR)"-"STR(VERSION_RELEASE)

enum GLOBAL_PROPERTY_ID {
	GLOBAL_PROPERTY_ID_SCRIPT_NAME,
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

//	dprintf(dlevel,"is_int: %d\n", JSVAL_IS_INT(id));
	if(JSVAL_IS_INT(id)) {
		prop_id = JSVAL_TO_INT(id);
		dprintf(dlevel,"prop_id: %d\n", prop_id);
		switch(prop_id) {
		case GLOBAL_PROPERTY_ID_SCRIPT_NAME:
			*rval = STRING_TO_JSVAL(JS_NewStringCopyZ(cx,get_script_name(cx)));
			break;
		default:
			*rval = JSVAL_NULL;
		}
	}
	return JS_TRUE;
}

static JSBool js_print(JSContext *cx, JSObject *obj, uintN argc, jsval *argv, jsval *rval) {
	JSEngine *e;
	uintN i;
	JSString *str;
	char *bytes;

	e = JS_GetPrivate(cx, obj);

	for (i = 0; i < argc; i++) {
//		dprintf(0,"argv[%d]: %s\n", i, jstypestr(cx,argv[i]));
		str = JS_ValueToString(cx, argv[i]);
		if (!str) return JS_FALSE;
		bytes = JS_EncodeString(cx, str);
		if (bytes) e->output("%s",bytes);
		JS_free(cx, bytes);
	}

	return JS_TRUE;
}

static int js_error(JSContext *cx, JSObject *obj, uintN argc, jsval *argv, jsval *rval) {
	char msg[1024];
	register u_int i;

	msg[0] = 0;
	for(i = 0; i < argc; i++)	{
		if (JSVAL_IS_STRING(argv[i]))
			strcat(msg,JS_GetStringBytes(JSVAL_TO_STRING(argv[i])));
	}
	JS_ReportError(cx,msg);
	return JS_TRUE;
}

static JSBool js_load(JSContext *cx, uintN argc, jsval *vp) {
	JSObject *obj;
	JSEngine *e;
	jsval *argv = vp + 2;
	JSString *str;
	int i;
	char *filename;
	char fixedname[256];

	obj = JS_THIS_OBJECT(cx, vp);
	if (!obj) return JS_FALSE;
	e = JS_GetPrivate(cx, obj);
	for (i = 0; i < argc; i++) {
		str = JS_ValueToString(cx, argv[i]);
		if (!str) return JS_FALSE;
		argv[i] = STRING_TO_JSVAL(str);
		filename = JS_GetStringBytes(str);
		strncpy(fixedname,filename,sizeof(fixedname)-1);
		JS_free(cx,filename);
		fixpath(fixedname,sizeof(fixedname)-1);
		if (_JS_EngineExec(e, fixedname, cx, 0)) {
			JS_ReportError(cx, "load(%s) failed\n",fixedname);
			return JS_FALSE;
		}
	}
	return JS_TRUE;
}

static JSBool js_run(JSContext *cx, uintN argc, jsval *vp) {
	char *name, *func;
	JSObject *obj;
	JSEngine *e;
	char fixedname[256],fname[256];

	if (argc < 1) {
		JS_ReportError(cx, "run takes 2 arguments (script:string, optional-function:string)\n");
		return 1;
	}
	name = func = 0;
	if (!JS_ConvertArguments(cx, argc, JS_ARGV(cx,vp), "s / s", &name, &func)) return JS_FALSE;
	dprintf(dlevel,"name: %s, func: %s\n", name, func);

	obj = JS_THIS_OBJECT(cx, vp);
	if (!obj) return JS_FALSE;
	e = JS_GetPrivate(cx, obj);
	strncpy(fixedname,name,sizeof(fixedname)-1);
	fixpath(fixedname,sizeof(fixedname)-1);
	if (!func) {
		register char *p;

		strncpy(fname,basename(name),sizeof(fname)-1);
		while(1) {
			p = strrchr(fname,'.');
			if (!p) break;
			*p = 0;
		}
		strcat(fname,"_main");
		func = fname;
	}
	*vp = INT_TO_JSVAL(_JS_EngineExec(e, fixedname, cx, func));
        return JS_TRUE;
}

static JSBool js_exit(JSContext *cx, JSObject *obj, uintN argc, jsval *argv, jsval *rval) {

	dprintf(dlevel,"argc: %d\n", argc);

	if (argc != 0) {
		dprintf(dlevel,"defining prop...\n");
		JS_DefineProperty(cx, JS_GetGlobalObject(cx), "exit_code", argv[0] ,NULL,NULL,JSPROP_ENUMERATE|JSPROP_READONLY);
	} else {
		*rval = JSVAL_VOID;
	}

        return JS_FALSE;
}

static JSBool js_abort(JSContext *cx, uintN argc, jsval *vp) {
	jsval *argv = JS_ARGV(cx,vp);

	dprintf(dlevel,"argc: %d\n", argc);
	if (argc && JSVAL_IS_INT(argv[0])) exit(JSVAL_TO_INT(argv[0]));
	else exit(0);

}

static JSBool js_sleep(JSContext *cx, JSObject *obj, uintN argc, jsval *argv, jsval *rval) {
	int r,n;

	dprintf(dlevel,"argc: %d\n", argc);
	if (!argc) return JS_TRUE;
	n = JSVAL_TO_INT(argv[0]);
	dprintf(dlevel,"n: %d\n", n);

	r = (sleep(n) != 0);
	*rval = BOOLEAN_TO_JSVAL(r);
	return JS_TRUE;
}

static JSBool jstime(JSContext *cx, uintN argc, jsval *vp){
	*vp = INT_TO_JSVAL(time(0));
	return JS_TRUE;
}

/*
 * function readline()
 * Provides a hook for scripts to read a line from stdin.
 */
static JSBool
ReadLine(JSContext *cx, uintN argc, jsval *vp)
{
#define BUFSIZE 256
    FILE *from;
    char *buf, *tmp;
    size_t bufsize, buflength, gotlength;
    JSBool sawNewline;
    JSString *str;

    from = stdin;
    buflength = 0;
    bufsize = BUFSIZE;
    buf = (char *) JS_malloc(cx, bufsize);
    if (!buf)
        return JS_FALSE;

    sawNewline = JS_FALSE;
    while ((gotlength = js_fgets(buf + buflength, bufsize - buflength, from)) > 0) {
        buflength += gotlength;

        /* Are we done? */
        if (buf[buflength - 1] == '\n') {
            buf[buflength - 1] = '\0';
            sawNewline = JS_TRUE;
            break;
        } else if (buflength < bufsize - 1) {
            break;
        }

        /* Else, grow our buffer for another pass. */
        bufsize *= 2;
        if (bufsize > buflength) {
            tmp = (char *) JS_realloc(cx, buf, bufsize);
        } else {
            JS_ReportOutOfMemory(cx);
            tmp = NULL;
        }

        if (!tmp) {
            JS_free(cx, buf);
            return JS_FALSE;
        }

        buf = tmp;
    }

    /* Treat the empty string specially. */
    if (buflength == 0) {
        *vp = feof(from) ? JSVAL_NULL : JS_GetEmptyStringValue(cx);
        JS_free(cx, buf);
        return JS_TRUE;
    }

    /* Shrink the buffer to the real size. */
    tmp = (char *) JS_realloc(cx, buf, buflength);
    if (!tmp) {
        JS_free(cx, buf);
        return JS_FALSE;
    }

    buf = tmp;

    /*
     * Turn buf into a JSString. Note that buflength includes the trailing null
     * character.
     */
    str = JS_NewStringCopyN(cx, buf, sawNewline ? buflength - 1 : buflength);
    JS_free(cx, buf);
    if (!str)
        return JS_FALSE;

    *vp = STRING_TO_JSVAL(str);
    return JS_TRUE;
}

static JSBool js_timestamp(JSContext *cx, uintN argc, jsval *vp) {
	char ts[32];
	int local;

	if (argc) {
		if (!JS_ConvertArguments(cx, argc, JS_ARGV(cx,vp), "d", &local)) return JS_FALSE;
	} else {
		local = 1;
	}
	if (get_timestamp(ts,sizeof(ts),local)) return JS_FALSE;
	*vp = STRING_TO_JSVAL(JS_NewStringCopyZ(cx,ts));
	return JS_TRUE;
}

static JSBool js_dirname(JSContext *cx, JSObject *obj, uintN argc, jsval *argv, jsval *rval) {
	char *path;

	if (argc != 1) {
		JS_ReportError(cx,"dirname requires 1 argument (path:string)\n");
		return JS_FALSE;
	}
	if (!JS_ConvertArguments(cx, argc, argv, "s", &path)) return JS_FALSE;
	if (!path) {
		JS_ReportError(cx,"path is null!");
		return JS_FALSE;
	}
	*rval = STRING_TO_JSVAL(JS_NewStringCopyZ(cx,dirname(path)));
	return JS_TRUE;
}

static JSBool js_basename(JSContext *cx, JSObject *obj, uintN argc, jsval *argv, jsval *rval) {
	char *path;

	if (argc != 1) {
		JS_ReportError(cx,"basename requires 1 argument (path:string)\n");
		return JS_FALSE;
	}
	if (!JS_ConvertArguments(cx, argc, argv, "s", &path)) return JS_FALSE;
	if (!path) {
		JS_ReportError(cx,"path is null!");
		return JS_FALSE;
	}
	*rval = STRING_TO_JSVAL(JS_NewStringCopyZ(cx,basename(path)));
	return JS_TRUE;
}

static JSBool js_log_info(JSContext *cx, uintN argc, jsval *vp) {
	return js_log_write(LOG_INFO,cx,argc,vp);
}

static JSBool js_log_warning(JSContext *cx, uintN argc, jsval *vp) {
	return js_log_write(LOG_WARNING,cx,argc,vp);
}

static JSBool js_log_error(JSContext *cx, uintN argc, jsval *vp) {
	return js_log_write(LOG_ERROR,cx,argc,vp);
}

static JSBool js_log_syserror(JSContext *cx, uintN argc, jsval *vp) {
	return js_log_write(LOG_SYSERR,cx,argc,vp);
}

static JSBool js_gc(JSContext *cx, uintN argc, jsval *vp) {
	JS_GC(cx);
	return JS_TRUE;
}

static JSBool js_memused(JSContext *cx, uintN argc, jsval *vp) {
	*vp = INT_TO_JSVAL(JS_ArenaTotalBytes());
	return JS_TRUE;
}

static JSBool js_version(JSContext *cx, uintN argc, jsval *vp) {
	*vp = INT_TO_JSVAL(version);
	return JS_TRUE;
}


static JSClass global_class = {
	"global",		/* Name */
	JSCLASS_GLOBAL_FLAGS | JSCLASS_HAS_PRIVATE,	/* Flags */
	JS_PropertyStub,	/* addProperty */
	JS_PropertyStub,	/* delProperty */
	global_getprop,		/* getProperty */
	JS_PropertyStub,	/* setProperty */
	JS_EnumerateStub,	/* enumerate */
	JS_ResolveStub,		/* resolve */
	JS_ConvertStub,		/* convert */
	JS_FinalizeStub,	/* finalize */
	JSCLASS_NO_OPTIONAL_MEMBERS
};

JSObject *JS_CreateGlobalObject(JSContext *cx, void *priv) {
	JSPropertySpec global_props[] = {
		{ "script_name",GLOBAL_PROPERTY_ID_SCRIPT_NAME, JSPROP_ENUMERATE | JSPROP_READONLY },
		{0}
	};
	JSFunctionSpec global_funcs[] = {
		JS_FS("printf",JS_Printf,0,0,0),
		JS_FS("sprintf",JS_SPrintf,0,0,0),
		JS_FS("dprintf",JS_DPrintf,1,1,0),
		JS_FS("putstr",js_print,0,0,0),
		JS_FS("print",js_print,0,0,0),
		JS_FS("error",js_error,0,0,0),
		JS_FN("load",js_load,1,1,0),
		JS_FN("include",js_load,1,1,0),
		JS_FN("run",js_run,1,1,0),
		JS_FS("sleep",js_sleep,1,0,0),
		JS_FS("exit",js_exit,1,1,0),
		JS_FN("abort",js_abort,1,1,0),
		JS_FN("quit",js_abort,0,0,0),
		JS_FN("readline",ReadLine,0,0,0),
		JS_FN("timestamp",js_timestamp,0,1,0),
		JS_FN("time",jstime,0,0,0),
		JS_FS("dirname",js_dirname,1,0,0),
		JS_FS("basename",js_basename,1,0,0),
		JS_FN("log_info",js_log_info,0,0,0),
		JS_FN("log_warning",js_log_warning,0,0,0),
		JS_FN("log_error",js_log_error,0,0,0),
		JS_FN("log_syserror",js_log_syserror,0,0,0),
		JS_FN("gc",js_gc,0,0,0),
		JS_FN("version",js_version,1,1,0),
		JS_FN("memused",js_memused,0,0,0),
		JS_FS_END
	};
	JSConstantSpec global_const[] = {
		JS_NUMCONST(VERSION_MAJOR),
		JS_NUMCONST(VERSION_MINOR),
		JS_NUMCONST(VERSION_RELEASE),
		JS_STRCONST(VERSION_STRING),
		{0}
	};
	JSAliasSpec global_aliases[] = {
//		{ "this", "global" },
		{ 0 },
	};
	JSObject *obj;

	/* Create the global object */
	dprintf(1,"Creating global object...\n");
	if ((obj = JS_NewObject(cx, &global_class, 0, 0)) == 0) {
		dprintf(1,"error creating global object\n");
		goto _create_global_error;
	}
	dprintf(1,"Global object: %p\n", obj);

	dprintf(1,"Defining global props...\n");
	if(!JS_DefineProperties(cx, obj, global_props)) {
		dprintf(1,"error defining global properties\n");
		goto _create_global_error;
	}

	dprintf(1,"Defining global funcs...\n");
	if(!JS_DefineFunctions(cx, obj, global_funcs)) {
		dprintf(1,"error defining global functions\n");
		goto _create_global_error;
	}

	dprintf(1,"Defining global constants...\n");
	if(!JS_DefineConstants(cx, obj, global_const)) {
		dprintf(1,"error defining global constants\n");
		goto _create_global_error;
	}

	dprintf(1,"Defining global aliases...\n");
	if(!JS_DefineAliases(cx, obj, global_aliases)) {
		dprintf(1,"error defining global aliases\n");
		goto _create_global_error;
	}

	dprintf(1,"Setting global private...\n");
	JS_SetPrivate(cx,obj,priv);

	/* Add standard classes */
	dprintf(1,"Adding standard classes...\n");
	if(!JS_InitStandardClasses(cx, obj)) {
		dprintf(1,"error initializing standard classes\n");
		goto _create_global_error;
	}

	dprintf(1,"done!\n");
	return obj;

_create_global_error:
	return 0;
}

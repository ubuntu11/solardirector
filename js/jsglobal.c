
#include <string.h>
#include <stdlib.h>
#include <unistd.h>

#include "jsapi.h"
#include "jsprintf.h"
#include "jsprf.h"
#include "jsstddef.h"
#include "jsinterp.h"
#include "jsscript.h"
#include "jsdbgapi.h"
#include "jsdtracef.h"
#include "jsengine.h"
#include "jsscan.h"

#define DEBUG_GLOBAL 1
#define dlevel 1

#ifdef DEBUG
#undef DEBUG
#endif
#if DEBUG_GLOBAL
#define DEBUG 1
#endif
#include "debug.h"

#define VERSION_MAJOR 1
#define VERSION_MINOR 8
#define VERSION_RELEASE 2
#define XSTR(n) #n
#define STR(n) XSTR(n)
#define VERSION_STRING() STR(VERSION_MAJOR)"."STR(VERSION_MINOR)"-"STR(VERSION_RELEASE)

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

	dprintf(dlevel,"is_int: %d\n", JSVAL_IS_INT(id));
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

static JSBool Print(JSContext *cx, JSObject *obj, uintN argc, jsval *argv, jsval *rval) {
	char msg[4096];
	uintN i;
	JSString *str;
	char *bytes,*p;
	JSEngine *e;

	e = JS_GetPrivate(cx, obj);

	p = msg;
	for (i = 0; i < argc; i++) {
//		dprintf(dlevel,"argv[%d]: %x\n", i, argv[1] & JSVAL_TAGMASK);
		str = JS_ValueToString(cx, argv[i]);
		if (!str) return JS_FALSE;
		bytes = JS_EncodeString(cx, str);
		if (!bytes) return JS_FALSE;
		p += sprintf(p,"%s%s",i ? " " : "", bytes);
		JS_free(cx, bytes);
	}
	e->output("%s\n",msg);

	return JS_TRUE;
}

//static JSBool PutStr(JSContext *cx, uintN argc, jsval *vp)
static JSBool PutStr(JSContext *cx, JSObject *obj, uintN argc, jsval *argv, jsval *rval) {
//	jsval *argv;
	JSString *str;
	char *bytes;
	JSEngine *e;

	e = JS_GetPrivate(cx, obj);
	if (argc != 0) {
//		argv = JS_ARGV(cx, vp);
		str = JS_ValueToString(cx, argv[0]);
		if (!str) return JS_FALSE;
		bytes = JS_EncodeString(cx, str);
		if (!bytes) return JS_FALSE;
		e->output(bytes);
		JS_free(cx, bytes);
	}

//	JS_SET_RVAL(cx, vp, JSVAL_VOID);
	JS_SET_RVAL(cx, rval, JSVAL_VOID);
	return JS_TRUE;
}

static int Error(JSContext *cx, JSObject *obj, uintN argc, jsval *argv, jsval *rval) {
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

static JSBool Exit(JSContext *cx, JSObject *obj, uintN argc, jsval *argv, jsval *rval) {
	return JS_FALSE;
}

static JSBool Sleep(JSContext *cx, JSObject *obj, uintN argc, jsval *argv, jsval *rval) {
	int s;

	dprintf(dlevel,"argc: %d\n", argc);
	if (!argc) return JS_TRUE;
	s = JSVAL_TO_INT(argv[0]);
	dprintf(dlevel,"s: %d\n", s);
	sleep(s);
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

struct jsalias {
	char *name;
	char *alias;
};
typedef struct jsalias jsalias_t;

JSObject *JS_CreateGlobalObject(JSContext *cx, void *priv) {
	JSObject *obj;
	JSPropertySpec global_props[] = {
		{ "script_name",GLOBAL_PROPERTY_ID_SCRIPT_NAME, JSPROP_ENUMERATE | JSPROP_READONLY },
		{0}
	};
	JSFunctionSpec global_funcs[] = {
		JS_FS("printf",JS_Printf,0,0,0),
		JS_FS("sprintf",JS_SPrintf,0,0,0),
		JS_FS("dprintf",JS_DPrintf,0,0,0),
		JS_FS("print",Print,0,0,0),
		JS_FS("error",Error,0,0,0),
		JS_FS("load",Load,1,0,0),
		JS_FS("include",Load,1,0,0),
		JS_FS("sleep",Sleep,1,0,0),
		JS_FS("exit",Exit,1,0,0),
		JS_FN("readline",ReadLine,0,0,0),
		JS_FS("putstr",PutStr,0,0,0),
		JS_FS_END
	};
	JSConstantSpec global_const[] = {
		{ "VERSION_MAJOR", DOUBLE_TO_JSVAL(VERSION_MAJOR) },
		{ "VERSION_MINOR", DOUBLE_TO_JSVAL(VERSION_MINOR) },
		{ "VERSION_RELEASE", DOUBLE_TO_JSVAL(VERSION_RELEASE) },
		{ "VERSION", STRING_TO_JSVAL(JS_NewStringCopyZ(cx,VERSION_STRING())) },
		{0}
	};
	jsalias_t *ap, global_aliases[] = {
		{ 0 },
	};

	/* Create the global object */
	dprintf(1,"Creating global object...\n");
	if ((obj = JS_NewObject(cx, &global_class, 0, 0)) == 0) {
		dprintf(1,"error creating global object\n");
		goto _create_global_error;
	}
	dprintf(1,"Global object: %p\n", obj);

	/* Define global props/funcs */
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

	dprintf(1,"Setting global private...\n");
	JS_SetPrivate(cx,obj,priv);

	/* Add standard classes */
	dprintf(1,"Adding standard classes...\n");
	if(!JS_InitStandardClasses(cx, obj)) {
		dprintf(1,"error initializing standard classes\n");
		goto _create_global_error;
	}

	dprintf(1,"Defining global aliases...\n");
	for(ap = global_aliases; ap->name; ap++) JS_AliasProperty(cx, obj, ap->name, ap->alias);

	dprintf(1,"done!\n");
	return obj;

_create_global_error:
	return 0;
}

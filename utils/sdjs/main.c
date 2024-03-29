
#ifdef JS
#undef DEBUG_MEM

#include "common.h"
#include "agent.h"
#include "jsengine.h"
#include "smanet.h"
#include "transports.h"
#include "client.h"

#ifdef WINDOWS
#include "wineditline/editline.c"
#include "wineditline/history.c"
#include "wineditline/fn_complete.c"
#else
#include <readline/readline.h>
#include <readline/history.h>
#endif

#define TESTING 0
int gQuitting = 0;

static JSBool
GetLine(JSContext *cx, char *bufp, FILE *file, const char *prompt) {
    /*
     * Use readline only if file is stdin, because there's no way to specify
     * another handle.  Are other filehandles interactive?
     */
#ifndef WINDOWS
    if (file == stdin) {
        char *linep = readline(prompt);
        if (!linep) return JS_FALSE;
        if (linep[0] != '\0') add_history(linep);
        strcpy(bufp, linep);
        JS_free(cx, linep);
        bufp += strlen(bufp);
        *bufp++ = '\n';
        *bufp = '\0';
   } else
#endif
   {
        char line[256];
        printf(prompt);
        fflush(stdout);
        if (!fgets(line, sizeof line, file)) return JS_FALSE;
        strcpy(bufp, line);
   }
   return JS_TRUE;
}

int shell(JSContext *cx) {
    JSBool ok, hitEOF;
    JSScript *script;
    jsval result;
    JSString *str;
    char buffer[4096];
    char *bufp;
    int lineno;
    int startline;

    /* It's an interactive filehandle; drop into read-eval-print loop. */
    lineno = 1;
    hitEOF = JS_FALSE;
    do {
        bufp = buffer;
        *bufp = '\0';

        /*
         * Accumulate lines until we get a 'compilable unit' - one that either
         * generates an error (before running out of source) or that compiles
         * cleanly.  This should be whenever we get a complete statement that
         * coincides with the end of a line.
         */
        startline = lineno;
        do {
            if (!GetLine(cx, bufp, stdin, startline == lineno ? "js> " : "")) {
                hitEOF = JS_TRUE;
                break;
            }
            bufp += strlen(bufp);
            lineno++;
        } while (!JS_BufferIsCompilableUnit(cx, JS_GetGlobalObject(cx), buffer, strlen(buffer)));
	dprintf(1,"buffer: %s\n", buffer);
	if (strncmp(buffer,"quit",4)==0) break;

        /* Clear any pending exception from previous failed compiles.  */
        JS_ClearPendingException(cx);
        script = JS_CompileScript(cx, JS_GetGlobalObject(cx), buffer, strlen(buffer), "typein", startline);
        if (script) {
                ok = JS_ExecuteScript(cx, JS_GetGlobalObject(cx), script, &result);
                if (ok && result != JSVAL_VOID) {
                    str = JS_ValueToString(cx, result);
                    if (str)
                        printf("%s\n", JS_GetStringBytes(str));
                    else
                        ok = JS_FALSE;
                }
            JS_DestroyScript(cx, script);
        }
    } while (!hitEOF && !gQuitting);
	return 0;
}

typedef JSObject *(js_newobj_t)(JSContext *cx, JSObject *parent, void *priv);
static void newgobj(JSContext *cx, char *name, js_newobj_t func, void *priv) {
	JSObject *newobj, *global = JS_GetGlobalObject(cx);
	jsval newval;

	newobj = func(cx, global, priv);
	newval = OBJECT_TO_JSVAL(newobj);
	JS_DefineProperty(cx, global, name, newval, 0, 0, 0);
}

static int js_init(JSContext *cx, JSObject *parent, void *priv) {
	solard_agent_t *ap = priv;
//	jsval agent_val;

//	client_jsinit(ap->js,0);
	newgobj(cx,"agent",(js_newobj_t *)jsagent_new,ap);
//	agent_val = OBJECT_TO_JSVAL(jsagent_new(cx,parent,ap));
//	JS_DefineProperty(cx, parent, "agent", agent_val, 0, 0, 0);
	JS_DefineProperty(cx, parent, "config", ap->config_val, 0, 0, 0);
	JS_DefineProperty(cx, parent, "mqtt", ap->mqtt_val, 0, 0, 0);
	newgobj(cx,"influx",js_influx_new,0);
	return 0;
}

int sdjs_config(void *h, int req, ...) {
	solard_agent_t *ap;
	va_list va;
	int r;

	r = 1;
	va_start(va,req);
	dprintf(1,"req: %d\n", req);
	switch(req) {
	case SOLARD_CONFIG_INIT:
		dprintf(1,"**** CONFIG INIT *******\n");
		/* 1st arg is AP */
		ap = va_arg(va,solard_agent_t *);
		JS_EngineAddInitFunc(ap->js, "js", js_init, ap);
		smanet_jsinit(ap->js);
		r = 0;
		break;
	case SOLARD_CONFIG_GET_INFO:
	    {
		void **vp;

		vp = va_arg(va,void *);
		*vp = json_object_get_value(json_create_object());
		r = 0;
	    }
	    break;
	}
	return r;
}

int main(int argc, char **argv) {
	char script[256],func[128];
	opt_proctab_t opts[] = {
		{ "%|script",&script,DATA_TYPE_STRING,sizeof(script)-1,0,"" },
		{ ":|func",&func,DATA_TYPE_STRING,sizeof(func)-1,0,"" },
		{ 0 }
	};
#if TESTING
	char *args[] = { "js", "-d", "7", "json.js" };
	argc = (sizeof(args)/sizeof(char *));
	argv = args;
#endif
	solard_agent_t *ap;
	solard_driver_t driver = null_driver;

	driver.name = "sdjs";
	driver.config = sdjs_config;

	*script = *func = 0;
	ap = agent_init(argc,argv,"1.0",opts,&driver,0,0,0);
	if (!ap) return 1;

        smanet_jsinit(ap->js);

//	printf("==> SCRIPT: %s, func: %s\n", script, func);
	dprintf(1,"script: %s\n", script);
	if (*script) {
		if (access(script,0) < 0) {
			printf("%s: %s\n",script,strerror(errno));
		} else {
			if (JS_EngineExec(ap->js,script,func,1)) {
				char *msg = JS_EngineGetErrmsg(ap->js);
				printf("%s: %s\n",script,strlen(msg) ? msg : "error executing script");
			}
		}
	} else {
		shell(JS_EngineNewContext(ap->js));
	}
	return 0;
}
#else
int main(void) { return 1; }
#endif

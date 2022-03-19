
#include "solard.h"

#if 0
#ifdef JS
#include "solard_agentinfo_propid.h"

enum SOLARD_AGENTINFO_PROPERTY_ID {
	SOLARD_AGENTINFO_PROPIDS
};

#include "solard_agentinfo_getprop.h"

static JSBool agentinfo_getprop(JSContext *cx, JSObject *obj, jsval id, jsval *rval) {
	solard_agentinfo_t *info;
	int prop_id;

	info = JS_GetPrivate(cx,obj);
	dprintf(1,"info: %p\n", info);
	if (!info) {
		JS_ReportError(cx, "private is null!");
		return JS_FALSE;
	}

	dprintf(1,"is_int: %d\n", JSVAL_IS_INT(id));
	if(JSVAL_IS_INT(id)) {
		prop_id = JSVAL_TO_INT(id);
		dprintf(1,"prop_id: %d\n", prop_id);
		switch(prop_id) {
		SOLARD_AGENTINFO_GETPROP
		default:
			*rval = JSVAL_NULL;
			break;
		}
	}
	return JS_TRUE;
}

#include "solard_agentinfo_setprop.h"

static JSBool agentinfo_setprop(JSContext *cx, JSObject *obj, jsval id, jsval *vp) {
	solard_agentinfo_t *info;
	int prop_id;

	info = JS_GetPrivate(cx,obj);
	dprintf(1,"info: %p\n", info);
	if (!info) {
		JS_ReportError(cx, "private is null!");
		return JS_FALSE;
	}

	dprintf(1,"is_int: %d\n", JSVAL_IS_INT(id));
	if(JSVAL_IS_INT(id)) {
		prop_id = JSVAL_TO_INT(id);
		dprintf(1,"prop_id: %d\n", prop_id);
		switch(prop_id) {
		SOLARD_AGENTINFO_SETPROP
		default:
			*vp = JSVAL_NULL;
			break;
		}
	}
	return JS_TRUE;
}

static JSClass agentinfo_class = {
	"agentinfo",
	JSCLASS_HAS_PRIVATE,
	JS_PropertyStub,
	JS_PropertyStub,
	agentinfo_getprop,
	agentinfo_setprop,
	JS_EnumerateStub,
	JS_ResolveStub,
	JS_ConvertStub,
	JS_FinalizeStub,
	JSCLASS_NO_OPTIONAL_MEMBERS
};

#include "solard_agentinfo_propspec.h"

JSObject *JSAgentInfo(JSContext *cx, solard_agentinfo_t *info) {
	JSPropertySpec agentinfo_props[] = { 
		SOLARD_AGENTINFO_PROPSPEC
		{0}
	};
	JSFunctionSpec agentinfo_funcs[] = {
		{ 0 }
	};
	JSObject *obj;

	dprintf(1,"defining %s object\n",agentinfo_class.name);
	obj = JS_InitClass(cx, JS_GetGlobalObject(cx), 0, &agentinfo_class, 0, 0, agentinfo_props, agentinfo_funcs, 0, 0);
	if (!obj) {
		JS_ReportError(cx,"unable to initialize si class");
		return 0;
	}
	JS_SetPrivate(cx,obj,info);
	dprintf(1,"done!\n");
	return obj;
}

#if 0
        char name[SOLARD_NAME_LEN];             /* Site name */
        solard_agent_t *ap;
        list inverters;
        list batteries;
        list producers;
        list consumers;
        list agents;
        int state;
        int interval;                           /* Agent check interval */
        int agent_warning;                      /* Warning, in seconds, when agent dosnt respond */
        int agent_error;                        /* In seconds, when agent considered lost */
        int agent_notify;                       /* In seconds, when monitoring should notify */
        int status;
        char errmsg[128];
        time_t last_check;                      /* Last time agents were checked */
        long start;
        char notify_path[256];
#endif

enum SOLARD_PROPERTY_ID {
	SOLARD_PROPERTY_ID_NONE,
	SOLARD_PROPERTY_ID_INTERVAL,
	SOLARD_PROPERTY_ID_SITE_NAME,
	SOLARD_PROPERTY_ID_INVERTERS,
	SOLARD_PROPERTY_ID_BATTERIES,
	SOLARD_PROPERTY_ID_AGENTS,
	SOLARD_PROPERTY_ID_STATE,
	SOLARD_PROPERTY_ID_INVTERVAL,
	SOLARD_PROPERTY_ID_AGENT_WARNING,
	SOLARD_PROPERTY_ID_AGENT_ERROR,
	SOLARD_PROPERTY_ID_AGENT_NOTIFY,
	SOLARD_PROPERTY_ID_STATUS,
	SOLARD_PROPERTY_ID_ERRMSG,
	SOLARD_PROPERTY_ID_LAST_CHECK,
	SOLARD_PROPERTY_ID_START,
};

#if 0
struct solard_jsconfig {
        char *name;
        solard_agent_t *ap;
        list jsinv;
        list jsbat;
        list jsagents;
        int state;
        int interval;                           /* Agent check interval */
        int agent_warning;                      /* Warning, in seconds, when agent dosnt respond */
        int agent_error;                        /* In seconds, when agent considered lost */
        int agent_notify;                       /* In seconds, when monitoring should notify */
        int status;
        char errmsg[128];
        time_t last_check;                      /* Last time agents were checked */
        long start;
        char notify_path[256];
};
#endif

static JSBool solard_getprop(JSContext *cx, JSObject *obj, jsval id, jsval *rval) {
	int prop_id;
	solard_config_t *conf;
	register int i;

	conf = JS_GetPrivate(cx,obj);
	dprintf(1,"conf: %p\n", conf);
	if (!conf) {
		JS_ReportError(cx, "private is null!");
		return JS_FALSE;
	}

	dprintf(1,"is_int: %d\n", JSVAL_IS_INT(id));
	if(JSVAL_IS_INT(id)) {
		prop_id = JSVAL_TO_INT(id);
		dprintf(1,"prop_id: %d\n", prop_id);
		switch(prop_id) {
#if 0
		case SOLARD_PROPERTY_ID_SITE_NAME:
			*rval = STRING_TO_JSVAL(JS_NewStringCopyZ(cx,conf->name));
			break;
#endif
		case SOLARD_PROPERTY_ID_AGENTS:
			{
				solard_agentinfo_t *info;
				JSObject *rows;
				jsval node;

				rows = JS_NewArrayObject(cx, list_count(conf->agents), NULL);
				i = 0;
				list_reset(conf->agents);
				while( (info = list_next(conf->agents)) != 0) {
					node = OBJECT_TO_JSVAL(JSAgentInfo(cx,info));
					JS_SetElement(cx, rows, i++, &node);
				}
				*rval = OBJECT_TO_JSVAL(rows);
			}
			break;
		case SOLARD_PROPERTY_ID_BATTERIES:
			{
				solard_battery_t *bp;
				JSObject *rows;
				jsval node;

				rows = JS_NewArrayObject(cx, list_count(conf->batteries), NULL);
				i = 0;
				list_reset(conf->batteries);
				while( (bp = list_next(conf->batteries)) != 0) {
					node = OBJECT_TO_JSVAL(JSBattery(cx,bp));
					JS_SetElement(cx, rows, i++, &node);
				}
				*rval = OBJECT_TO_JSVAL(rows);
			}
			break;
		case SOLARD_PROPERTY_ID_INTERVAL:
			dprintf(1,"getting interval: %d\n", conf->ap->read_interval);
			*rval = INT_TO_JSVAL(conf->ap->read_interval);
			break;
		default:
			*rval = JSVAL_NULL;
			break;
		}
	}
	return JS_TRUE;
}

static JSBool solard_setprop(JSContext *cx, JSObject *obj, jsval id, jsval *rval) {
	int prop_id;
	solard_config_t *conf;

	conf = JS_GetPrivate(cx,obj);
	dprintf(1,"conf: %p\n", conf);
	if (!conf) {
		JS_ReportError(cx, "private is null!");
		return JS_FALSE;
	}

	dprintf(1,"is_int: %d\n", JSVAL_IS_INT(id));
	if(JSVAL_IS_INT(id)) {
		prop_id = JSVAL_TO_INT(id);
		dprintf(1,"prop_id: %d\n", prop_id);
		switch(prop_id) {
		case SOLARD_PROPERTY_ID_INTERVAL:
			conf->ap->read_interval = JSVAL_TO_INT(*rval);
			break;
		default:
			*rval = JSVAL_NULL;
			break;
		}
	}
	return JS_TRUE;
}

static JSClass solard_class = {
	"sd",
	JSCLASS_HAS_PRIVATE,
	JS_PropertyStub,
	JS_PropertyStub,
	solard_getprop,
	solard_setprop,
	JS_EnumerateStub,
	JS_ResolveStub,
	JS_ConvertStub,
	JS_FinalizeStub,
	JSCLASS_NO_OPTIONAL_MEMBERS
};

JSObject *JSSolard(JSContext *cx, void *priv) {
	JSPropertySpec solard_props[] = { 
		{ "name",		SOLARD_PROPERTY_ID_SITE_NAME,	JSPROP_ENUMERATE | JSPROP_READONLY },
		{ "agents",		SOLARD_PROPERTY_ID_AGENTS,	JSPROP_ENUMERATE },
		{ "batteries",		SOLARD_PROPERTY_ID_BATTERIES,	JSPROP_ENUMERATE | JSPROP_READONLY },
		{ "inverters",		SOLARD_PROPERTY_ID_INVERTERS,	JSPROP_ENUMERATE | JSPROP_READONLY },
		{ "interval",		SOLARD_PROPERTY_ID_INTERVAL,	JSPROP_ENUMERATE },
		{0}
	};
	JSFunctionSpec solard_funcs[] = {
		{ 0 }
	};
	JSObject *obj;
#if 0
	solard_config_t *conf = priv;

	if (!conf->props) {
		conf->props = config_to_props(conf->ap->cp, "solard", 0);
		if (!conf->props) {
			log_error("unable to create props: %s\n", config_get_errmsg(conf->ap->cp));
			return 0;
		}
	}
#endif

	dprintf(1,"defining %s object\n",solard_class.name);
	obj = JS_InitClass(cx, JS_GetGlobalObject(cx), 0, &solard_class, 0, 0, solard_props, solard_funcs, 0, 0);
	if (!obj) {
		JS_ReportError(cx,"unable to initialize si class");
		return 0;
	}
//	JS_DefineProperties(cx, obj, solard_props);
	JS_SetPrivate(cx,obj,priv);
	dprintf(1,"done!\n");
	return obj;
}

int solard_jsinit(solard_config_t *conf) {
	int r;
	r = JS_EngineAddInitFunc(conf->ap->js, "solard", JSSolard, conf);
	return r;
}
#endif /* JS */
#endif

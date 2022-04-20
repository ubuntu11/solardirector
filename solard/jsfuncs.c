
#ifdef JS
#include "solard.h"
#include "jsstr.h"
#include "jsprintf.h"

#define dlevel 1

static JSBool agentinfo_getprop(JSContext *cx, JSObject *obj, jsval id, jsval *rval) {
	solard_agent_t *info;
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
		default:
			*rval = JSVAL_NULL;
			break;
		}
	}
	return JS_TRUE;
}

static JSBool agentinfo_setprop(JSContext *cx, JSObject *obj, jsval id, jsval *vp) {
	solard_agent_t *info;
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

JSObject *JSAgentInfo(JSContext *cx, solard_agent_t *info) {
	JSPropertySpec agentinfo_props[] = { 
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

static JSBool solard_getprop(JSContext *cx, JSObject *obj, jsval id, jsval *rval) {
	int prop_id;
	solard_instance_t *sd;
	register int i;

	sd = JS_GetPrivate(cx,obj);
	dprintf(1,"sd: %p\n", sd);
	if (!sd) {
		JS_ReportError(cx, "private is null!");
		return JS_FALSE;
	}

	dprintf(1,"is_int: %d\n", JSVAL_IS_INT(id));
	if(JSVAL_IS_INT(id)) {
		prop_id = JSVAL_TO_INT(id);
		dprintf(1,"prop_id: %d\n", prop_id);
		switch(prop_id) {
		case SOLARD_PROPERTY_ID_AGENTS:
			{
				solard_agent_t *info;
				JSObject *rows;
				jsval node;

				rows = JS_NewArrayObject(cx, list_count(sd->agents), NULL);
				i = 0;
				list_reset(sd->agents);
				while( (info = list_next(sd->agents)) != 0) {
					node = OBJECT_TO_JSVAL(JSAgentInfo(cx,info));
					JS_SetElement(cx, rows, i++, &node);
				}
				*rval = OBJECT_TO_JSVAL(rows);
			}
			break;
#if 0
		case SOLARD_PROPERTY_ID_BATTERIES:
			{
				solard_battery_t *bp;
				JSObject *rows;
				jsval node;

				rows = JS_NewArrayObject(cx, list_count(sd->batteries), NULL);
				i = 0;
				list_reset(sd->batteries);
				while( (bp = list_next(sd->batteries)) != 0) {
					node = OBJECT_TO_JSVAL(JSBattery(cx,bp));
					JS_SetElement(cx, rows, i++, &node);
				}
				*rval = OBJECT_TO_JSVAL(rows);
			}
			break;
#endif
		case SOLARD_PROPERTY_ID_INTERVAL:
			dprintf(1,"getting interval: %d\n", sd->ap->interval);
			*rval = INT_TO_JSVAL(sd->ap->interval);
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
	solard_instance_t *sd;

	sd = JS_GetPrivate(cx,obj);
	dprintf(1,"sd: %p\n", sd);
	if (!sd) {
		JS_ReportError(cx, "private is null!");
		return JS_FALSE;
	}

	dprintf(1,"is_int: %d\n", JSVAL_IS_INT(id));
	if(JSVAL_IS_INT(id)) {
		prop_id = JSVAL_TO_INT(id);
		dprintf(1,"prop_id: %d\n", prop_id);
		switch(prop_id) {
		case SOLARD_PROPERTY_ID_INTERVAL:
			sd->ap->interval = JSVAL_TO_INT(*rval);
			break;
		default:
			*rval = JSVAL_NULL;
			break;
		}
	}
	return JS_TRUE;
}

static JSClass sd_class = {
	"SolarDirector",
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

static JSBool jssd_notify(JSContext *cx, uintN argc, jsval *vp) {
	jsval val;
	char *str;
//	solard_instance_t *sd;
	JSObject *obj;

	obj = JS_THIS_OBJECT(cx, vp);
	if (!obj) return JS_FALSE;
//	sd = JS_GetPrivate(cx, obj);

	JS_SPrintf(cx, JS_GetGlobalObject(cx), argc, JS_ARGV(cx, vp), &val);
	str = (char *)js_GetStringBytes(cx, JSVAL_TO_STRING(val));
	dprintf(dlevel,"str: %s\n", str);
//	sd_notify(sd,"%s",str);
	return JS_TRUE;
}

static int jssd_init(JSContext *cx, JSObject *parent, void *priv) {
	solard_instance_t *sd = priv;
	JSPropertySpec sd_props[] = {
		{ "name",		SOLARD_PROPERTY_ID_SITE_NAME,	JSPROP_ENUMERATE | JSPROP_READONLY },
		{ "agents",		SOLARD_PROPERTY_ID_AGENTS,	JSPROP_ENUMERATE },
		{ "batteries",		SOLARD_PROPERTY_ID_BATTERIES,	JSPROP_ENUMERATE | JSPROP_READONLY },
		{ "inverters",		SOLARD_PROPERTY_ID_INVERTERS,	JSPROP_ENUMERATE | JSPROP_READONLY },
		{ "interval",		SOLARD_PROPERTY_ID_INTERVAL,	JSPROP_ENUMERATE },
		{ 0 }
	};
	JSFunctionSpec sd_funcs[] = {
		JS_FN("notify",jssd_notify,0,0,0),
		{ 0 }
	};
	JSAliasSpec sd_aliases[] = {
		{ 0 }
	};
	JSConstantSpec sd_constants[] = {
		{ 0 }
	};
	JSObject *obj,*global = JS_GetGlobalObject(cx);

	dprintf(dlevel,"sd->props: %p, cp: %p\n",sd->props,sd->ap->cp);
	if (!sd->props) {
		/* XXX name must match section in agent_init */
		sd->props = config_to_props(sd->ap->cp, "solard", sd_props);
		dprintf(dlevel,"sd->props: %p\n",sd->props);
		if (!sd->props) {
			log_error("unable to create props: %s\n", config_get_errmsg(sd->ap->cp));
			return 0;
		}
	}

	dprintf(dlevel,"Defining %s object\n",sd_class.name);
	obj = JS_InitClass(cx, parent, 0, &sd_class, 0, 0, sd->props, sd_funcs, 0, 0);
	if (!obj) {
		JS_ReportError(cx,"unable to initialize si class");
		return 1;
	}
	dprintf(dlevel,"Defining %s aliases\n",sd_class.name);
	if (!JS_DefineAliases(cx, obj, sd_aliases)) {
		JS_ReportError(cx,"unable to define aliases");
		return 1;
	}
	dprintf(dlevel,"Defining %s constants\n",sd_class.name);
	if (!JS_DefineConstants(cx, global, sd_constants)) {
		JS_ReportError(cx,"unable to define constants");
		return 1;
	}
	dprintf(dlevel,"done!\n");
	JS_SetPrivate(cx,obj,sd);

//	sd->agents_val = OBJECT_TO_JSVAL(jsagent_new(cx,obj,sd->ap));

	/* Create the global convenience objects */
	JS_DefineProperty(cx, global, "sd", OBJECT_TO_JSVAL(obj), 0, 0, 0);
//	JS_DefineProperty(cx, global, "data", sd->data_val, 0, 0, 0);
	return 0;
}

int solard_jsinit(solard_instance_t *sd) {
	return JS_EngineAddInitFunc(sd->ap->js, "solard", jssd_init, sd);
}
#endif

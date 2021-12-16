
#include "agent.h"

enum AGENT_PROPERTY_ID {
	AGENT_PROPERTY_ID_NAME,
	AGENT_PROPERTY_ID_INSTANCE_NAME,
	AGENT_PROPERTY_ID_CFG,
	AGENT_PROPERTY_ID_SECTION_NAME,
	AGENT_PROPERTY_ID_MQTT,
	AGENT_PROPERTY_ID_M,
	AGENT_PROPERTY_ID_READ_INTERVAL,
	AGENT_PROPERTY_ID_WRITE_INTERVAL,
	AGENT_PROPERTY_ID_MQ,
	AGENT_PROPERTY_ID_STATE,
	AGENT_PROPERTY_ID_ROLE,
	AGENT_PROPERTY_ID_INFO,
	AGENT_PROPERTY_ID_SCRIPT_DIR,
	AGENT_PROPERTY_ID_START_SCRIPT,
	AGENT_PROPERTY_ID_STOP_SCRIPT,
	AGENT_PROPERTY_ID_MONITOR_SCRIPT,
	AGENT_PROPERTY_ID_READ_SCRIPT,
	AGENT_PROPERTY_ID_WRITE_SCRIPT,
	AGENT_PROPERTY_ID_RUN_SCRIPT,
	AGENT_PROPERTY_ID_MAX
};

static JSBool global_enumerate(JSContext *cx, JSObject *obj) {
    return JS_EnumerateStandardClasses(cx, obj);
}

static JSBool global_resolve(JSContext *cx, JSObject *obj, jsval id, uintN flags, JSObject **objp)
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

//	JSCLASS_NEW_RESOLVE | JSCLASS_GLOBAL_FLAGS,
JSClass agent_class = {
	"agent",		/* Name */
	0,			/* Flags */
	JS_PropertyStub,	/* addProperty */
	JS_PropertyStub,	/* delProperty */
	agent_getprop	,	/* getProperty */
	JS_PropertyStub,	/* setProperty */
	JS_EnumerateStub,	/* enumerate */
	JS_ResolveStub,		/* resolve */
	JS_ConvertStub,		/* convert */
	JS_FinalizeStub,	/* finalize */
	JSCLASS_NO_OPTIONAL_MEMBERS
};


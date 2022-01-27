
#ifdef JS
#include "smanet.h"
#include "jsengine.h"
#include "debug.h"
#include "transports.h"

#define dlevel 6

enum SMANET_PROPERTY_ID {
	SMANET_PROPERTY_ID_CONNECTED,
	SMANET_PROPERTY_ID_AUTOCONNECT,
	SMANET_PROPERTY_ID_CHANNELS,
	SMANET_PROPERTY_ID_TRANSPORT,
	SMANET_PROPERTY_ID_TARGET,
	SMANET_PROPERTY_ID_TOPTS,
	SMANET_PROPERTY_ID_SERIAL,
	SMANET_PROPERTY_ID_TYPE,
	SMANET_PROPERTY_ID_SRC,
	SMANET_PROPERTY_ID_DEST,
	SMANET_PROPERTY_ID_TIMEOUTS,
	SMANET_PROPERTY_ID_COMMANDS,
};

static JSBool smanet_getprop(JSContext *cx, JSObject *obj, jsval id, jsval *rval) {
	smanet_session_t *s;
	int prop_id;

	s = JS_GetPrivate(cx, obj);
//	dprintf(dlevel,"s: %p\n", s);
	if (!s) {
		*rval = JSVAL_VOID;
		return JS_TRUE;
	}
	if (JSVAL_IS_INT(id)) {
		prop_id = JSVAL_TO_INT(id);
		dprintf(dlevel,"prop_id: %d", prop_id);
		switch(prop_id) {
		case SMANET_PROPERTY_ID_CONNECTED:
			*rval = BOOLEAN_TO_JSVAL(s->connected);
			break;
		case SMANET_PROPERTY_ID_AUTOCONNECT:
//			*rval = BOOLEAN_TO_JSVAL(s->autoconnect);
			break;
		case SMANET_PROPERTY_ID_CHANNELS:
			*rval = JSVAL_VOID;
			break;
		case SMANET_PROPERTY_ID_SERIAL:
			*rval = INT_TO_JSVAL(s->serial);
			break;
		case SMANET_PROPERTY_ID_TRANSPORT:
			type_to_jsval(cx,DATA_TYPE_STRING,rval,0);
			break;
		case SMANET_PROPERTY_ID_TYPE:
			*rval = STRING_TO_JSVAL(JS_NewStringCopyZ(cx,s->type));
			break;
		case SMANET_PROPERTY_ID_SRC:
			*rval = INT_TO_JSVAL(s->src);
			break;
		case SMANET_PROPERTY_ID_DEST:
			*rval = INT_TO_JSVAL(s->dest);
			break;
		case SMANET_PROPERTY_ID_TIMEOUTS:
			*rval = INT_TO_JSVAL(s->timeouts);
			break;
		case SMANET_PROPERTY_ID_COMMANDS:
			*rval = INT_TO_JSVAL(s->commands);
			break;
		default:
			break;
		}
	}
	return JS_TRUE;
}

static JSBool smanet_setprop(JSContext *cx, JSObject *obj, jsval id, jsval *vp) {
	smanet_session_t *s;
	int r,prop_id;

	r = 1;
	s = JS_GetPrivate(cx, obj);
	if (!s) goto smanet_setprop_done;
	if (JSVAL_IS_INT(id)) {
		prop_id = JSVAL_TO_INT(id);
		dprintf(dlevel,"smanet_setprop: prop_id: %d", prop_id);
		switch(prop_id) {
		case SMANET_PROPERTY_ID_AUTOCONNECT:
			jsval_to_type(DATA_TYPE_BOOLEAN,&s->connected,0,cx,*vp);
			r = 0;
			break;
		default:
			break;
		}
	}
smanet_setprop_done:
	*vp = BOOLEAN_TO_JSVAL(r);
	return JS_TRUE;
}

JSClass smanet_class = {
	"SMANET",		/* Name */
	JSCLASS_HAS_PRIVATE,	/* Flags */
	JS_PropertyStub,	/* addProperty */
	JS_PropertyStub,	/* delProperty */
	smanet_getprop,		/* getProperty */
	smanet_setprop,		/* setProperty */
	JS_EnumerateStub,	/* enumerate */
	JS_ResolveStub,		/* resolve */
	JS_ConvertStub,		/* convert */
	JS_FinalizeStub,	/* finalize */
	JSCLASS_NO_OPTIONAL_MEMBERS
};

static JSBool jsget_channels(JSContext *cx, JSObject *obj, uintN argc, jsval *argv, jsval *rval) {
	return JS_FALSE;
}

static JSBool jsload_channels(JSContext *cx, uintN argc, jsval *vp) {
	smanet_session_t *s;
	char *filename;
	int r;

	r = 1;
	s = JS_GetPrivate(cx, JS_THIS_OBJECT(cx,vp));
	if (!s) goto jsload_channels_done;
	if (argc < 1) return JS_FALSE;
	if (!JS_ConvertArguments(cx, argc, JS_ARGV(cx,vp), "s", &filename)) return JS_FALSE;
	dprintf(dlevel,"filename: %s\n", filename);
	r = smanet_load_channels(s,filename);
jsload_channels_done:
	*vp = INT_TO_JSVAL(r);
	return JS_TRUE;
}

static JSBool jssave_channels(JSContext *cx, JSObject *obj, uintN argc, jsval *argv, jsval *rval) {
	return JS_FALSE;
}

static JSBool jsget(JSContext *cx, JSObject *obj, uintN argc, jsval *argv, jsval *rval) {
	JSString *str;
	char *name;
	smanet_session_t *s;
	double d;
	char *p;

	s = JS_GetPrivate(cx, obj);
	if (!s) {
		JS_ReportError(cx,"SMANET private is null!");
		goto jsget_error;
	}
	if (!s->connected) {
		sprintf(s->errmsg,"SMANET.get: not connected");
		goto jsget_error;
	}
	dprintf(dlevel,"locking...\n");
	smanet_lock(s);
	dprintf(dlevel,"argc: %d\n", argc);
	if (argc != 1) {
		sprintf(s->errmsg,"SMANET.get: 1 argument is required");
		goto jsget_error;
	}
	str = JS_ValueToString(cx, argv[0]);
	if (!str) {
		sprintf(s->errmsg,"SMANET.get: unable to get string");
		goto jsget_error;
	}
	name = JS_EncodeString(cx, str);
	if (!name) {
		sprintf(s->errmsg,"SMANET.get: unable to get name");
		goto jsget_error;
	}
	dprintf(dlevel,"name: %s\n", name);
	if (smanet_get_value(s, name, &d, &p)) {
		JS_ReportError(cx, "error getting value: %s",s->errmsg);
		goto jsget_error;
	}
	dprintf(dlevel,"d: %f, p: %s\n", d, p);
	if (p) {
		*rval = STRING_TO_JSVAL(JS_NewStringCopyZ(cx,p));
	} else {
		JS_NewDoubleValue(cx, d, rval);
	}
	dprintf(dlevel,"unlocking...\n");
	smanet_unlock(s);
	return JS_TRUE;

jsget_error:
	dprintf(dlevel,"unlocking...\n");
	smanet_unlock(s);
	*rval = JSVAL_VOID;
	return JS_TRUE;
}

static JSBool jsset(JSContext *cx, JSObject *obj, uintN argc, jsval *argv, jsval *rval) {
	JSString *str;
	char *name;
	smanet_session_t *s;
	double d;
	char *p;

	*rval = BOOLEAN_TO_JSVAL(true);
	s = JS_GetPrivate(cx, obj);
	if (!s) {
		JS_ReportError(cx, "SMANET private is null!");
		goto jsset_error;
	}
	if (!s->connected) {
		sprintf(s->errmsg,"SMANET.set: not connected");
		goto jsset_error;
	}
	dprintf(dlevel,"locking...\n");
	smanet_lock(s);
	dprintf(dlevel,"argc: %d\n", argc);
	if (argc != 2) {
		sprintf(s->errmsg,"SMANET.set: 2 arguments are required");
		goto jsset_error;
	}
	str = JS_ValueToString(cx, argv[0]);
	if (!str) {
		sprintf(s->errmsg,"SMANET.set: unable to get string");
		goto jsset_error;
	}
	name = JS_EncodeString(cx, str);
	if (!name) {
		sprintf(s->errmsg,"SMANET.set: unable to get name");
		goto jsset_error;
	}
	dprintf(dlevel,"name: %s\n", name);
	p = 0;
	if (JSVAL_IS_STRING(argv[1])) {
		p = JS_GetStringBytes(JSVAL_TO_STRING(argv[1]));
	} else {
		d = *JSVAL_TO_DOUBLE(argv[1]);
	}
	dprintf(dlevel,"d: %f, p: %s\n", d, p);
	if (smanet_set_value(s, name, d, p)) {
		JS_ReportError(cx, "error setting value: %s",s->errmsg);
		goto jsset_error;
	}
	*rval = BOOLEAN_TO_JSVAL(false);
	dprintf(dlevel,"unlocking...\n");
	smanet_unlock(s);
	return JS_TRUE;

jsset_error:
	smanet_unlock(s);
	return JS_TRUE;
}

static JSBool jsconnect(JSContext *cx, uintN argc, jsval *vp) {
	smanet_session_t *s;
	char *transport, *target, *topts;
//	jsval *argv = JS_ARGV(cx,vp);
	JSObject *obj;
	int r;

	obj = JS_THIS_OBJECT(cx, vp);
	if (!obj) return JS_FALSE;
	s = JS_GetPrivate(cx, obj);
	if (!s) return JS_FALSE;

	if (argc > 2) {
		if (!JS_ConvertArguments(cx, argc, JS_ARGV(cx,vp), "s s s", &transport, &target, &topts))
			return JS_FALSE;
	}
	dprintf(dlevel,"transport: %s, target: %s, topts:%s\n", transport, target, topts);

	if (!transport || !target) {
		JS_ReportError(cx, "smanet_connect: transport not set");
		return JS_FALSE;
	}

	r = smanet_connect(s, transport, target, topts);
	*vp = BOOLEAN_TO_JSVAL(r);
	return JS_TRUE;
}

static JSBool jsdisconnect(JSContext *cx, JSObject *obj, uintN argc, jsval *argv, jsval *rval) {
	return JS_FALSE;
}

static JSBool smanet_ctor(JSContext *cx, JSObject *obj, uintN argc, jsval *argv, jsval *rval) {
	smanet_session_t *s;
	char *transport, *target, *topts;
	JSObject *newobj;

	dprintf(dlevel,"argc: %d\n", argc);
	transport = target = topts = 0;
	if (argc == 3) {
		if (!JS_ConvertArguments(cx, argc, argv, "s s s", &transport, &target, &topts)) return JS_FALSE;
	} else if (argc > 1) {
		JS_ReportError(cx, "SMANET requires 3 arguments (transport, target, topts)");
		return JS_FALSE;
	}
	dprintf(dlevel,"transport: %s, target: %s, topts:%s\n", transport, target, topts);

	s = smanet_init(transport,target,topts);
	dprintf(dlevel,"s: %p\n", s);
	if (!s) return JS_FALSE;

	newobj = js_InitSMANETClass(cx,JS_GetGlobalObject(cx));
	JS_SetPrivate(cx,newobj,s);
	*rval = OBJECT_TO_JSVAL(newobj);

	return JS_TRUE;
}

JSObject *jssmanet_new(JSContext *cx, smanet_session_t *s, char *transport, char *target, char *topts) {
	JSObject *newobj;

#if 0
	s->transport = transport;
	s->target = target;
	s->topts = topts;
#endif
	dprintf(dlevel,"s: %p\n", s);
	newobj = js_InitSMANETClass(cx,JS_GetGlobalObject(cx));
	JS_SetPrivate(cx,newobj,s);
	return newobj;
}

JSObject *js_InitSMANETClass(JSContext *cx, JSObject *gobj) {
	JSPropertySpec smanet_props[] = { 
		{ "connected", SMANET_PROPERTY_ID_CONNECTED, JSPROP_ENUMERATE | JSPROP_READONLY },
		{ "autoconnect", SMANET_PROPERTY_ID_AUTOCONNECT, JSPROP_ENUMERATE },
		{ "channels", SMANET_PROPERTY_ID_CHANNELS, JSPROP_ENUMERATE | JSPROP_READONLY },
		{ "serial",SMANET_PROPERTY_ID_SERIAL,JSPROP_ENUMERATE | JSPROP_READONLY  },
		{ "type",SMANET_PROPERTY_ID_TYPE,JSPROP_ENUMERATE | JSPROP_READONLY  },
		{ "src",SMANET_PROPERTY_ID_SRC,JSPROP_ENUMERATE | JSPROP_READONLY  },
		{ "dest",SMANET_PROPERTY_ID_DEST,JSPROP_ENUMERATE | JSPROP_READONLY  },
		{ "timeouts",SMANET_PROPERTY_ID_TIMEOUTS,JSPROP_ENUMERATE | JSPROP_READONLY },
		{ "commands",SMANET_PROPERTY_ID_COMMANDS,JSPROP_ENUMERATE | JSPROP_READONLY  },
		{0}
	};
	JSFunctionSpec smanet_funcs[] = {
		JS_FN("connect",jsconnect,0,0,0),
		JS_FN("disconnect",jsdisconnect,0,0,0),
		JS_FN("get_channels",jsget_channels,0,0,0),
		JS_FN("load_channels",jsload_channels,1,1,0),
		JS_FN("save_channels",jssave_channels,1,1,0),
		JS_FS("get",jsget,1,1,0),
		JS_FS("set",jsset,2,2,0),
		{ 0 }
	};
	JSObject *obj;

	dprintf(dlevel,"creating %s class...\n",smanet_class.name);
	obj = JS_InitClass(cx, gobj, gobj, &smanet_class, smanet_ctor, 3, smanet_props, smanet_funcs, 0, 0);
	if (!obj) {
		JS_ReportError(cx,"unable to initialize smanet class");
		return 0;
	}
	JS_SetPrivate(cx,obj,0);
	dprintf(dlevel,"done!\n");
	return obj;
}

int smanet_jsinit(JSEngine *js) {
        return JS_EngineAddInitClass(js, "SMANET", js_InitSMANETClass);
}
#endif

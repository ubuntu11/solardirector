
#ifdef JS
#include "smanet.h"
#include "jsengine.h"
#include "debug.h"
#include "transports.h"

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
	if (!s) return JS_FALSE;
	if (JSVAL_IS_INT(id)) {
		prop_id = JSVAL_TO_INT(id);
		dprintf(1,"prop_id: %d", prop_id);
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
	int prop_id;

	s = JS_GetPrivate(cx, obj);
	if (!s) return JS_FALSE;
	if (JSVAL_IS_INT(id)) {
		prop_id = JSVAL_TO_INT(id);
		dprintf(1,"smanet_setprop: prop_id: %d", prop_id);
		switch(prop_id) {
		case SMANET_PROPERTY_ID_AUTOCONNECT:
			jsval_to_type(DATA_TYPE_BOOLEAN,&s->connected,0,cx,*vp);
			break;
		default:
			break;
		}
	}
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

	s = JS_GetPrivate(cx, JS_THIS_OBJECT(cx,vp));
	if (!s) return JS_FALSE;
	if (argc < 1) return JS_FALSE;
	if (!JS_ConvertArguments(cx, argc, JS_ARGV(cx,vp), "s", &filename)) return JS_FALSE;
	dprintf(1,"filename: %s\n", filename);
	r = smanet_load_channels(s,filename);
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
	int r;

	s = JS_GetPrivate(cx, obj);
	if (!s) return JS_FALSE;
	if (!s->connected) {
		*rval = JSVAL_VOID;
		return JS_TRUE;
	}

	r = JS_FALSE;
	dprintf(1,"locking...\n");
	smanet_lock(s);
	dprintf(1,"argc: %d\n", argc);
	str = JS_ValueToString(cx, argv[0]);
	if (!str) goto jsget_done;
	name = JS_EncodeString(cx, str);
	if (!name) goto jsget_done;
	dprintf(1,"name: %s\n", name);
	if (smanet_get_value(s, name, &d, &p)) {
		JS_ReportError(cx, "unable to get value: %s",s->errmsg);
		r = JS_TRUE;
		goto jsget_done;
	}
	dprintf(1,"d: %f, p: %s\n", d, p);
	if (p) {
		*rval = STRING_TO_JSVAL(JS_NewStringCopyZ(cx,p));
	} else {
		JS_NewDoubleValue(cx, d, rval);
	}
	r = JS_TRUE;
jsget_done:
	dprintf(1,"unlocking...\n");
	smanet_unlock(s);
	dprintf(1,"returning: %d\n", r);
	return r;
}

static JSBool jsset(JSContext *cx, JSObject *obj, uintN argc, jsval *argv, jsval *rval) {
	JSString *str;
	char *name;
	smanet_session_t *s;
	double d;
	char *p;
	int r;

	s = JS_GetPrivate(cx, obj);
	if (!s) return JS_FALSE;
	if (!s->connected) return JS_TRUE;

	r = JS_FALSE;
	dprintf(1,"locking...\n");
	smanet_lock(s);
	dprintf(1,"argc: %d\n", argc);
	str = JS_ValueToString(cx, argv[0]);
	if (!str) goto jsget_done;
	name = JS_EncodeString(cx, str);
	if (!name) goto jsget_done;
	dprintf(1,"name: %s\n", name);
	p = 0;
	if (JSVAL_IS_STRING(argv[1])) {
		p = JS_GetStringBytes(JSVAL_TO_STRING(argv[1]));
	} else {
		d = *JSVAL_TO_DOUBLE(argv[1]);
	}
	dprintf(1,"d: %f, p: %s\n", d, p);
	if (smanet_set_value(s, name, d, p)) {
		JS_ReportError(cx, "unable to set value: %s",s->errmsg);
		r = JS_TRUE;
		goto jsget_done;
	}
	dprintf(1,"d: %f, p: %s\n", d, p);
	if (p) {
		*rval = STRING_TO_JSVAL(JS_NewStringCopyZ(cx,p));
	} else {
		JS_NewDoubleValue(cx, d, rval);
	}
	r = JS_TRUE;
jsget_done:
	dprintf(1,"unlocking...\n");
	smanet_unlock(s);
	dprintf(1,"returning: %d\n", r);
	return r;
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
	dprintf(0,"transport: %s, target: %s, topts:%s\n", transport, target, topts);

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

	dprintf(0,"argc: %d\n", argc);
	transport = target = topts = 0;
	if (argc == 3) {
		if (!JS_ConvertArguments(cx, argc, argv, "s s s", &transport, &target, &topts)) return JS_FALSE;
	} else if (argc > 1) {
		JS_ReportError(cx, "SMANET requires 3 arguments (transport, target, topts)");
		return JS_FALSE;
	}
	dprintf(0,"transport: %s, target: %s, topts:%s\n", transport, target, topts);

	s = smanet_init(transport,target,topts);
	dprintf(1,"s: %p\n", s);
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

	dprintf(1,"creating %s class...\n",smanet_class.name);
	obj = JS_InitClass(cx, gobj, gobj, &smanet_class, smanet_ctor, 3, smanet_props, smanet_funcs, 0, 0);
	if (!obj) {
		JS_ReportError(cx,"unable to initialize smanet class");
		return 0;
	}
	JS_SetPrivate(cx,obj,0);
	dprintf(1,"done!\n");
	return obj;
}

int smanet_jsinit(JSEngine *js) {
        return JS_EngineAddInitClass(js, "SMANET", js_InitSMANETClass);
}
#endif

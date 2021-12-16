
#include "smanet.h"
#include "jsengine.h"
#include "debug.h"

static JSBool smanet_getprop(JSContext *cx, JSObject *obj, jsval id, jsval *rval) {
	smanet_session_t *s;
	int prop_id;

	s = JS_GetPrivate(cx, obj);
	dprintf(1,"s: %p\n", s);
	if (!s) {
		JS_ReportError(cx, "private is null!");
		return JS_FALSE;
	}

	dprintf(1,"is_int: %d\n", JSVAL_IS_INT(id));
	if (JSVAL_IS_INT(id)) {
		prop_id = JSVAL_TO_INT(id);
		dprintf(1,"prop_id: %d", prop_id);
		switch(prop_id) {
		default:
			break;
		}
	}
	return JS_TRUE;
}

static JSBool smanet_setprop(JSContext *cx, JSObject *obj, jsval id, jsval *rval) {
	int prop_id;

	if (JSVAL_IS_INT(id)) {
		prop_id = JSVAL_TO_INT(id);
		dprintf(1,"smanet_setprop: prop_id: %d", prop_id);
		switch(prop_id) {
		default:
			break;
		}
	}
	return JS_TRUE;
}

JSClass smanet_class = {
	"smanet",		/* Name */
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

static JSBool jsget(JSContext *cx, JSObject *obj, uintN argc, jsval *argv, jsval *rval) {
	JSString *str;
	char *name;
	smanet_session_t *s;
	double d;
	char *p;
	int r;

	s = JS_GetPrivate(cx, obj);
	dprintf(1,"s: %p\n", s);
	if (!s) {
		JS_ReportError(cx, "private is null!");
		return JS_FALSE;
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
	dprintf(1,"s: %p\n", s);
	if (!s) {
		JS_ReportError(cx, "private is null!");
		return JS_FALSE;
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

#if 0
static JSBool smanet_new(JSContext * cx, JSObject * obj, uintN argc, jsval * argv, jsval * rval) {
	smanet_session_t *s;

	s = JS_GetPrivate(cx, obj);
	if (!s) {
		JS_ReportError(cx, "private is null!");
		return JS_FALSE;
	}
	*rval = BOOLEAN_TO_JSVAL(JS_TRUE);
	return JS_TRUE;
}
#endif

JSObject *JSSMANET(JSContext *cx, void *priv) {
	JSPropertySpec smanet_props[] = { 
		{0}
	};
	JSFunctionSpec smanet_funcs[] = {
		{ "get",jsget,1,1 },
		{ "set",jsset,2,2 },
		{ 0 }
	};
	JSObject *obj;

	obj = JS_InitClass(cx, JS_GetGlobalObject(cx), 0, &smanet_class, 0, 0, smanet_props, smanet_funcs, 0, 0);
	if (!obj) {
		JS_ReportError(cx,"unable to initialize smanet class");
		return 0;
	}
	JS_SetPrivate(cx,obj,priv);
	return obj;
}

int smanet_jsinit(JSEngine *js, smanet_session_t *s) {
        return JS_EngineAddInitFunc(js, "smanet", JSSMANET, s);
}

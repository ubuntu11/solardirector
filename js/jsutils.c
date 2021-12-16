
#include "jsapi.h"
#include "jsutils.h"

JSBool JS_DefineConstants(JSContext *cx, JSObject *obj, JSConstSpec *cs) {
	JSConstSpec *csp;
	JSBool ok;

	for(csp = cs; csp->name; csp++) {
		ok = JS_DefineProperty(cx, obj, csp->name, csp->value, 0, 0,
			JSPROP_ENUMERATE|JSPROP_READONLY|JSPROP_PERMANENT|JSPROP_EXPORTED);
		if (!ok) return JS_FALSE;
	}
	return JS_TRUE;
}

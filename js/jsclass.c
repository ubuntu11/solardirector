
#include "common.h"
#include "config.h"
#include "jsapi.h"
#include "jsobj.h"
#include "jsstr.h"

#define dlevel 6

struct class_private {
	char name[64];
	config_t *cp;
	config_section_t *sec;
	JSPropertySpec *props;
};
typedef struct class_private class_private_t;

static JSBool class_getprop(JSContext *cx, JSObject *obj, jsval id, jsval *rval) {
	class_private_t *p;

	dprintf(dlevel,"getprop called!\n");
	p = JS_GetPrivate(cx,obj);
	if (!p) {
		JS_ReportError(cx,"private is null!");
		return JS_FALSE;
	}
	return js_config_common_getprop(cx, obj, id, rval, p->cp, 0);
}

static JSBool class_setprop(JSContext *cx, JSObject *obj, jsval id, jsval *rval) {
	class_private_t *p;

	dprintf(dlevel,"setprop called!\n");
	p = JS_GetPrivate(cx,obj);
	if (!p) {
		JS_ReportError(cx,"private is null!");
		return JS_FALSE;
	}
	return js_config_common_setprop(cx, obj, id, rval, p->cp, 0);
}

static JSClass class_class = {
	"Class",		/* Name */
	JSCLASS_GLOBAL_FLAGS | JSCLASS_HAS_PRIVATE,	/* Flags */
	JS_PropertyStub,	/* addProperty */
	JS_PropertyStub,	/* delProperty */
	class_getprop,		/* getProperty */
	class_setprop,		/* setProperty */
	JS_EnumerateStub,	/* enumerate */
	JS_ResolveStub,		/* resolve */
	JS_ConvertStub,		/* convert */
	JS_FinalizeStub,	/* finalize */
	JSCLASS_NO_OPTIONAL_MEMBERS
};

static JSBool class_ctor(JSContext *cx, JSObject *obj, uintN argc, jsval *argv, jsval *rval) {
	class_private_t *p;
	JSObject *newobj,*objp,*gobj;
	JSClass *newclassp,*ccp = &class_class;
	char *namep;
	int flags;

	if (argc < 2) {
		JS_ReportError(cx, "Class requires 2 arguments (name:string, config:object, [config flags:number])\n");
		return JS_FALSE;
	}
	namep =0;
	objp = 0;
	flags = 0;
	if (!JS_ConvertArguments(cx, argc, argv, "s o / u", &namep, &objp, &flags)) return JS_FALSE;
	dprintf(dlevel,"name: %s, objp: %p, flags: %x\n", namep, objp, flags);

	p = malloc(sizeof(*p));
	if (!p) {
		JS_ReportError(cx,"error allocating memory");
		return JS_FALSE;
	}
	strncpy(p->name,namep,sizeof(p->name)-1);
	if (objp) {
		p->cp = JS_GetPrivate(cx,objp);
		if (!p->cp) {
			JS_ReportError(cx,"config pointer is null");
			return JS_FALSE;
		}

		/* Create a new config section */
		p->sec = config_get_section(p->cp, namep);
		if (!p->sec) p->sec = config_create_section(p->cp, namep, flags);
		if (!p->sec) {
			JS_ReportError(cx,"error creating new config section");
			return JS_FALSE;
		}
	}

	/* Create a new instance of Class */
	newclassp = malloc(sizeof(JSClass));
	*newclassp = *ccp;
	newclassp->name = p->name;

	gobj = JS_GetGlobalObject(cx);
	newobj = JS_InitClass(cx, gobj, gobj, newclassp, 0, 0, 0, 0, 0, 0);
	JS_SetPrivate(cx,newobj,p);
	*rval = OBJECT_TO_JSVAL(newobj);
	return JS_TRUE;
}

JSObject *js_InitClassClass(JSContext *cx, JSObject *gobj) {
	JSObject *obj;

	dprintf(dlevel,"Defining %s object\n",class_class.name);
	obj = JS_InitClass(cx, gobj, gobj, &class_class, class_ctor, 2, 0, 0, 0, 0);
	if (!obj) {
		JS_ReportError(cx,"unable to initialize Class class");
		return 0;
	}
	dprintf(dlevel,"done!\n");
	return obj;
}

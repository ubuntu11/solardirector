
#ifdef JS
#include "jbd.h"

#define dlevel 5

enum JBD_HW_PROPERTY_ID {
	JBD_HW_PROPERTY_ID_MF,
	JBD_HW_PROPERTY_ID_MODEL,
	JBD_HW_PROPERTY_ID_DATE,
	JBD_HW_PROPERTY_ID_VER
};

static JSBool jbd_hw_getprop(JSContext *cx, JSObject *obj, jsval id, jsval *rval) {
	int prop_id;
	jbd_hwinfo_t *hwinfo;

	hwinfo = JS_GetPrivate(cx,obj);
	if(JSVAL_IS_INT(id)) {
		prop_id = JSVAL_TO_INT(id);
		dprintf(dlevel,"prop_id: %d\n", prop_id);
		switch(prop_id) {
		case JBD_HW_PROPERTY_ID_MF:
			*rval = STRING_TO_JSVAL(JS_NewStringCopyZ(cx,hwinfo->manufacturer));
			break;
		case JBD_HW_PROPERTY_ID_MODEL:
			*rval = STRING_TO_JSVAL(JS_NewStringCopyZ(cx,hwinfo->model));
			break;
		case JBD_HW_PROPERTY_ID_DATE:
			*rval = STRING_TO_JSVAL(JS_NewStringCopyZ(cx,hwinfo->mfgdate));
			break;
		case JBD_HW_PROPERTY_ID_VER:
			*rval = STRING_TO_JSVAL(JS_NewStringCopyZ(cx,hwinfo->version));
			break;
		default:
			JS_ReportError(cx, "property not found");
			*rval = JSVAL_VOID;
			return JS_FALSE;
			break;
		}
	}
	return JS_TRUE;
}

static JSClass jbd_hw_class = {
	"jbd_hw",		/* Name */
	JSCLASS_HAS_PRIVATE,	/* Flags */
	JS_PropertyStub,	/* addProperty */
	JS_PropertyStub,	/* delProperty */
	jbd_hw_getprop,		/* getProperty */
	JS_PropertyStub,	/* setProperty */
	JS_EnumerateStub,	/* enumerate */
	JS_ResolveStub,		/* resolve */
	JS_ConvertStub,		/* convert */
	JS_FinalizeStub,	/* finalize */
	JSCLASS_NO_OPTIONAL_MEMBERS
};

JSObject *JSJBDHWInfo(JSContext *cx, void *priv) {
	JSPropertySpec jbd_hw_props[] = {
		{ "manufacturer", JBD_HW_PROPERTY_ID_MF, JSPROP_ENUMERATE | JSPROP_READONLY },
		{ "model", JBD_HW_PROPERTY_ID_MODEL, JSPROP_ENUMERATE | JSPROP_READONLY },
		{ "mfgdate", JBD_HW_PROPERTY_ID_DATE, JSPROP_ENUMERATE | JSPROP_READONLY },
		{ "version", JBD_HW_PROPERTY_ID_VER, JSPROP_ENUMERATE | JSPROP_READONLY },
		{ 0 }
	};
	JSObject *obj;

	dprintf(dlevel,"defining %s object\n",jbd_hw_class.name);
	obj = JS_InitClass(cx, JS_GetGlobalObject(cx), 0, &jbd_hw_class, 0, 0, jbd_hw_props, 0, 0, 0);
	if (!obj) {
		JS_ReportError(cx,"unable to initialize %s", jbd_hw_class.name);
		return 0;
	}
	JS_SetPrivate(cx,obj,priv);
	dprintf(dlevel,"done!\n");
	return obj;
}

/*************************************************************************/

#define JBD_PROPERTY_ID_HW 127

static JSBool jbd_getprop(JSContext *cx, JSObject *obj, jsval id, jsval *rval) {
	int prop_id;
	jbd_session_t *s;
	config_property_t *p;

	s = JS_GetPrivate(cx,obj);
	if(JSVAL_IS_INT(id)) {
		prop_id = JSVAL_TO_INT(id);
		dprintf(dlevel,"prop_id: %d\n", prop_id);
		switch(prop_id) {
		case JBD_PROPERTY_ID_HW:
			*rval = OBJECT_TO_JSVAL(JSJBDHWInfo(cx,&s->hwinfo));
			break;
		default:
			p = CONFIG_GETMAP(s->ap->cp,prop_id);
			if (!p) p = config_get_propbyid(s->ap->cp,prop_id);
			if (!p) {
				JS_ReportError(cx, "property not found");
				return JS_FALSE;
			}
//			dprintf(1,"p: type: %d(%s), name: %s\n", p->type, typestr(p->type), p->name);
			if (strcmp(p->name,"temps") == 0) p->len = s->data.ntemps;
			else if (strcmp(p->name,"cellvolt") == 0) p->len = s->data.ncells;
			*rval = type_to_jsval(cx,p->type,p->dest,p->len);
			break;
		}
	}
	return JS_TRUE;
}

static JSBool jbd_setprop(JSContext *cx, JSObject *obj, jsval id, jsval *vp) {
	int prop_id;
	jbd_session_t *s;
	config_property_t *p;

	s = JS_GetPrivate(cx,obj);
	if(JSVAL_IS_INT(id)) {
		prop_id = JSVAL_TO_INT(id);
		dprintf(dlevel,"prop_id: %d\n", prop_id);
		switch(prop_id) {
		default:
			p = CONFIG_GETMAP(s->ap->cp,prop_id);
			if (!p) p = config_get_propbyid(s->ap->cp,prop_id);
			if (!p) {
				JS_ReportError(cx, "property not found");
				return JS_FALSE;
			}
			dprintf(1,"p: type: %d(%s), name: %s\n", p->type, typestr(p->type), p->name);
			jsval_to_type(p->type,p->dest,p->dsize,cx,*vp);
			break;
		}
	}
	return JS_TRUE;
}

static JSClass jbd_class = {
	"jbd",			/* Name */
	JSCLASS_HAS_PRIVATE,	/* Flags */
	JS_PropertyStub,	/* addProperty */
	JS_PropertyStub,	/* delProperty */
	jbd_getprop,		/* getProperty */
	jbd_setprop,		/* setProperty */
	JS_EnumerateStub,	/* enumerate */
	JS_ResolveStub,		/* resolve */
	JS_ConvertStub,		/* convert */
	JS_FinalizeStub,	/* finalize */
	JSCLASS_NO_OPTIONAL_MEMBERS
};

JSObject *JSJBD(JSContext *cx, void *priv) {
	JSPropertySpec jbd_props[] = {
		{ "hw", JBD_PROPERTY_ID_HW, JSPROP_ENUMERATE | JSPROP_READONLY },
		{ 0 }
	};
	JSConstantSpec jbd_consts[] = {
		JS_NUMCONST(JBD_MAX_TEMPS),
		JS_NUMCONST(JBD_MAX_CELLS),
		{ 0 }
	};
	JSObject *obj;
	jbd_session_t *s = priv;

	if (!s->props) {
		s->props = config_to_props(s->ap->cp, "jbd", jbd_props);
		if (!s->props) {
			log_error("unable to create props: %s\n", config_get_errmsg(s->ap->cp));
			return 0;
		}
	}

	dprintf(dlevel,"defining %s object\n",jbd_class.name);
	obj = JS_InitClass(cx, JS_GetGlobalObject(cx), 0, &jbd_class, 0, 0, s->props, 0, 0, 0);
	if (!obj) {
		JS_ReportError(cx,"unable to initialize si class");
		return 0;
	}
	JS_SetPrivate(cx,obj,priv);
	if (!JS_DefineConstants(cx, JS_GetGlobalObject(cx), jbd_consts)) {
		JS_ReportError(cx,"unable to add jbd constants");
		return 0;
	}
	dprintf(dlevel,"done!\n");
	return obj;
}

int jbd_jsinit(jbd_session_t *s) {
	return JS_EngineAddInitFunc(s->ap->js, "jbd", JSJBD, s);
}

#endif
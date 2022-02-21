
#ifdef JS
#include "jk.h"

#define dlevel 5

static JSBool jk_getprop(JSContext *cx, JSObject *obj, jsval id, jsval *rval);
static JSBool jk_setprop(JSContext *cx, JSObject *obj, jsval id, jsval *rval);

enum JK_HW_PROPERTY_ID {
	JK_HW_PROPERTY_ID_MF,
	JK_HW_PROPERTY_ID_MODEL,
	JK_HW_PROPERTY_ID_DATE,
	JK_HW_PROPERTY_ID_VER
};

static JSBool jk_hw_getprop(JSContext *cx, JSObject *obj, jsval id, jsval *rval) {
	int prop_id;
	jk_hwinfo_t *hwinfo;

	hwinfo = JS_GetPrivate(cx,obj);
	if(JSVAL_IS_INT(id)) {
		prop_id = JSVAL_TO_INT(id);
		dprintf(dlevel,"prop_id: %d\n", prop_id);
		switch(prop_id) {
		case JK_HW_PROPERTY_ID_MF:
			*rval = STRING_TO_JSVAL(JS_NewStringCopyZ(cx,hwinfo->manufacturer));
			break;
		case JK_HW_PROPERTY_ID_MODEL:
			*rval = STRING_TO_JSVAL(JS_NewStringCopyZ(cx,hwinfo->model));
			break;
		case JK_HW_PROPERTY_ID_DATE:
			*rval = STRING_TO_JSVAL(JS_NewStringCopyZ(cx,hwinfo->mfgdate));
			break;
		case JK_HW_PROPERTY_ID_VER:
			*rval = STRING_TO_JSVAL(JS_NewStringCopyZ(cx,hwinfo->swvers));
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

static JSClass jk_hw_class = {
	"jk_hw",		/* Name */
	JSCLASS_HAS_PRIVATE,	/* Flags */
	JS_PropertyStub,	/* addProperty */
	JS_PropertyStub,	/* delProperty */
	jk_hw_getprop,		/* getProperty */
	JS_PropertyStub,	/* setProperty */
	JS_EnumerateStub,	/* enumerate */
	JS_ResolveStub,		/* resolve */
	JS_ConvertStub,		/* convert */
	JS_FinalizeStub,	/* finalize */
	JSCLASS_NO_OPTIONAL_MEMBERS
};

JSObject *JSJKHWInfo(JSContext *cx, void *priv) {
	JSPropertySpec jk_hw_props[] = {
		{ "manufacturer", JK_HW_PROPERTY_ID_MF, JSPROP_ENUMERATE | JSPROP_READONLY },
		{ "model", JK_HW_PROPERTY_ID_MODEL, JSPROP_ENUMERATE | JSPROP_READONLY },
		{ "mfgdate", JK_HW_PROPERTY_ID_DATE, JSPROP_ENUMERATE | JSPROP_READONLY },
		{ "version", JK_HW_PROPERTY_ID_VER, JSPROP_ENUMERATE | JSPROP_READONLY },
		{ 0 }
	};
	JSObject *obj;

	dprintf(dlevel,"defining %s object\n",jk_hw_class.name);
	obj = JS_InitClass(cx, JS_GetGlobalObject(cx), 0, &jk_hw_class, 0, 0, jk_hw_props, 0, 0, 0);
	if (!obj) {
		JS_ReportError(cx,"unable to initialize %s", jk_hw_class.name);
		return 0;
	}
	JS_SetPrivate(cx,obj,priv);
	dprintf(dlevel,"done!\n");
	return obj;
}

/*************************************************************************/

static JSClass jk_data_class = {
	"jk_data",		/* Name */
	JSCLASS_HAS_PRIVATE,	/* Flags */
	JS_PropertyStub,	/* addProperty */
	JS_PropertyStub,	/* delProperty */
	jk_getprop,		/* getProperty */
	JS_PropertyStub,	/* setProperty */
	JS_EnumerateStub,	/* enumerate */
	JS_ResolveStub,		/* resolve */
	JS_ConvertStub,		/* convert */
	JS_FinalizeStub,	/* finalize */
	JSCLASS_NO_OPTIONAL_MEMBERS
};

JSObject *JSJKData(JSContext *cx, jk_session_t *s) {
	JSAliasSpec jk_data_aliases[] = {
		{ 0 }
	};
	JSObject *obj;

	if (!s->data_props) {
		s->data_props = config_to_props(s->ap->cp, "jk_data", 0);
		dprintf(dlevel,"info->props: %p\n",s->data_props);
		if (!s->data_props) {
			log_error("unable to create jk_data props: %s\n", config_get_errmsg(s->ap->cp));
			return 0;
		}
	}

	dprintf(dlevel,"defining %s object\n",jk_data_class.name);
	obj = JS_InitClass(cx, JS_GetGlobalObject(cx), 0, &jk_data_class, 0, 0, s->data_props, 0, 0, 0);
	if (!obj) {
		JS_ReportError(cx,"unable to initialize %s", jk_data_class.name);
		return 0;
	}
	dprintf(dlevel,"defining %s aliases\n",jk_data_class.name);
	if (!JS_DefineAliases(cx, obj, jk_data_aliases)) {
		JS_ReportError(cx,"unable to define aliases");
		return 0;
	}
	JS_SetPrivate(cx,obj,s);
	dprintf(dlevel,"done!\n");
	return obj;
}

/*************************************************************************/

#define	JK_PROPERTY_ID_DATA 1024
#define JK_PROPERTY_ID_HW 1025

static JSBool jk_getprop(JSContext *cx, JSObject *obj, jsval id, jsval *rval) {
	int prop_id;
	jk_session_t *s;
	config_property_t *p;

	s = JS_GetPrivate(cx,obj);
        if (!s) {
                JS_ReportError(cx,"private is null!");
                return JS_FALSE;
        }
	if(JSVAL_IS_INT(id)) {
		prop_id = JSVAL_TO_INT(id);
		dprintf(dlevel,"prop_id: %d\n", prop_id);
		switch(prop_id) {
		case JK_PROPERTY_ID_DATA:
			*rval = OBJECT_TO_JSVAL(JSJKData(cx,s));
			break;
		case JK_PROPERTY_ID_HW:
			*rval = OBJECT_TO_JSVAL(JSJKHWInfo(cx,&s->hwinfo));
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
			else if (strcmp(p->name,"cellres") == 0) p->len = s->data.ncells;
			*rval = type_to_jsval(cx,p->type,p->dest,p->len);
			break;
		}
	}
	return JS_TRUE;
}

static JSBool jk_setprop(JSContext *cx, JSObject *obj, jsval id, jsval *rval) {
	jk_session_t *s;

	s = JS_GetPrivate(cx,obj);
	if (!s) {
		JS_ReportError(cx,"private is null!");
		return JS_FALSE;
	}
	return config_jssetprop(cx, obj, id, rval, s->ap->cp, s->props);
}

static JSClass jk_class = {
	"jk",			/* Name */
	JSCLASS_HAS_PRIVATE,	/* Flags */
	JS_PropertyStub,	/* addProperty */
	JS_PropertyStub,	/* delProperty */
	jk_getprop,		/* getProperty */
	jk_setprop,		/* setProperty */
	JS_EnumerateStub,	/* enumerate */
	JS_ResolveStub,		/* resolve */
	JS_ConvertStub,		/* convert */
	JS_FinalizeStub,	/* finalize */
	JSCLASS_NO_OPTIONAL_MEMBERS
};

JSObject *JSJK(JSContext *cx, void *priv) {
	JSPropertySpec jk_props[] = {
		{ "data", JK_PROPERTY_ID_DATA, JSPROP_ENUMERATE | JSPROP_READONLY },
		{ "hw", JK_PROPERTY_ID_HW, JSPROP_ENUMERATE | JSPROP_READONLY },
		{ 0 }
	};
	JSConstantSpec jk_consts[] = {
		JS_NUMCONST(JK_STATE_CHARGING),
		JS_NUMCONST(JK_STATE_DISCHARGING),
		JS_NUMCONST(JK_STATE_BALANCING),
		JS_NUMCONST(JK_MAX_TEMPS),
		JS_NUMCONST(JK_MAX_CELLS),
		{ 0 }
	};
	JSObject *obj;
	jk_session_t *s = priv;

	if (!s->props) {
		s->props = config_to_props(s->ap->cp, s->ap->section_name, jk_props);
		if (!s->props) {
			log_error("unable to create props: %s\n", config_get_errmsg(s->ap->cp));
			return 0;
		}
	}

	dprintf(dlevel,"defining %s object\n",jk_class.name);
	obj = JS_InitClass(cx, JS_GetGlobalObject(cx), 0, &jk_class, 0, 0, s->props, 0, 0, 0);
	if (!obj) {
		JS_ReportError(cx,"unable to initialize si class");
		return 0;
	}
	JS_SetPrivate(cx,obj,priv);
	if (!JS_DefineConstants(cx, JS_GetGlobalObject(cx), jk_consts)) {
		JS_ReportError(cx,"unable to add jk constants");
		return 0;
	}
	dprintf(dlevel,"done!\n");
	return obj;
}

int jk_jsinit(jk_session_t *s) {
	return JS_EngineAddInitFunc(s->ap->js, s->ap->section_name, JSJK, s);
}

#endif

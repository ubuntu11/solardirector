
#ifdef JS
#include "si.h"

#define dlevel 5
enum SI_PROPERTY_ID {
	SI_PROPERTY_ID_NONE,
};

static JSBool si_getprop(JSContext *cx, JSObject *obj, jsval id, jsval *rval) {
	int prop_id;
	si_session_t *s;
	config_property_t *p;

	s = JS_GetPrivate(cx,obj);
	dprintf(dlevel,"s: %p\n", s);
	if (!s) {
		JS_ReportError(cx, "private is null!");
		return JS_FALSE;
	}

//	dprintf(dlevel,"is_int: %d\n", JSVAL_IS_INT(id));
	if(JSVAL_IS_INT(id)) {
		prop_id = JSVAL_TO_INT(id);
		dprintf(dlevel,"prop_id: %d\n", prop_id);
		switch(prop_id) {
#if 0
		case SI_SESSION_PROPERTY_ID_BA:
			{
				JSObject *arr;
				jsval element;
				int i,num;

				num = (s->bafull ? SI_MAX_BA : s->baidx);
				arr = JS_NewArrayObject(cx, num, NULL);
				for(i=0; i < num; i++) {
					JS_NewDoubleValue(cx, s->ba[i], &element);
					JS_SetElement(cx, arr, i, &element);
				}
				*rval = OBJECT_TO_JSVAL(arr);
			}
			break;
#endif
		default:
			p = CONFIG_GETMAP(s->ap->cp,prop_id);
			if (!p) p = config_get_propbyid(s->ap->cp,prop_id);
			if (!p) {
				JS_ReportError(cx, "property not found");
				return JS_FALSE;
			}
//			dprintf(1,"p: type: %d(%s), name: %s\n", p->type, typestr(p->type), p->name);
			*rval = typetojsval(cx,p->type,p->dest,p->len);
			break;
		}
	}
	return JS_TRUE;
}

static JSBool si_setprop(JSContext *cx, JSObject *obj, jsval id, jsval *vp) {
	int prop_id;
	si_session_t *s;
	config_property_t *p;

	s = JS_GetPrivate(cx,obj);
//	dprintf(1,"s: %p\n", s);
	if (!s) {
		JS_ReportError(cx, "private is null!");
		return JS_FALSE;
	}

//	dprintf(dlevel,"is_int: %d\n", JSVAL_IS_INT(id));
	if(JSVAL_IS_INT(id)) {
		prop_id = JSVAL_TO_INT(id);
		dprintf(dlevel,"prop_id: %d\n", prop_id);
		switch(prop_id) {
		default:
			p = CONFIG_GETMAP(s->ap->cp,prop_id);
//			dprintf(1,"p: %p\n", p);
			if (!p) p = config_get_propbyid(s->ap->cp,prop_id);
//			dprintf(1,"p: %p\n", p);
			if (!p) {
				JS_ReportError(cx, "property not found");
				return JS_FALSE;
			}
//			dprintf(1,"p: type: %d(%s), name: %s\n", p->type, typestr(p->type), p->name);
			jsvaltotype(p->type,p->dest,p->len,cx,*vp);
			break;
		}
	}
	return JS_TRUE;
}

static JSClass si_class = {
	"si",			/* Name */
	JSCLASS_HAS_PRIVATE,	/* Flags */
	JS_PropertyStub,	/* addProperty */
	JS_PropertyStub,	/* delProperty */
	si_getprop,		/* getProperty */
	si_setprop,		/* setProperty */
	JS_EnumerateStub,	/* enumerate */
	JS_ResolveStub,		/* resolve */
	JS_ConvertStub,		/* convert */
	JS_FinalizeStub,	/* finalize */
	JSCLASS_NO_OPTIONAL_MEMBERS
};

static JSBool jsstart_grid(JSContext *cx, JSObject *obj, uintN argc, jsval *argv, jsval *rval) {
	si_session_t *s;
	int wait;

	s = JS_GetPrivate(cx, obj);
	dprintf(1,"s: %p\n", s);
	if (!s) {
		JS_ReportError(cx, "private is null!");
		return JS_FALSE;
	}
	dprintf(1,"argc: %d\n", argc);
	jsvaltotype(DATA_TYPE_INT,&wait,0,cx,argv[0]);
	dprintf(1,"wait: %d\n", wait);
	si_start_grid(s,wait);
	return JS_TRUE;
}

JSObject *JSSunnyIsland(JSContext *cx, void *priv) {
	JSFunctionSpec si_funcs[] = {
		{ "start_grid",jsstart_grid,1,1 },
		{ 0 }
	};
	JSObject *obj;
	si_session_t *s = priv;

	if (!s->props) {
		s->props = configtoprops(s->ap->cp, "si", 0);
		if (!s->props) {
			log_error("unable to create props: %s\n", config_get_errmsg(s->ap->cp));
			return 0;
		}
	}

	dprintf(dlevel,"defining %s object\n",si_class.name);
	obj = JS_InitClass(cx, JS_GetGlobalObject(cx), 0, &si_class, 0, 0, s->props, si_funcs, 0, 0);
	if (!obj) {
		JS_ReportError(cx,"unable to initialize si class");
		return 0;
	}
	JS_SetPrivate(cx,obj,priv);
	dprintf(dlevel,"done!\n");
	return obj;
}

int si_jsinit(si_session_t *s) {
	dprintf(dlevel,"s->smanet: %p\n", s->smanet);
	if (s->smanet) smanet_jsinit(s->ap->js, s->smanet);
	return JS_EngineAddInitFunc(s->ap->js, "si", JSSunnyIsland, s);
}

#endif

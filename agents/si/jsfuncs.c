
#ifdef JS

#define DEBUG_JSFUNCS 1
#define dlevel 6

#ifdef DEBUG
#undef DEBUG
#endif
#define DEBUG DEBUG_JSFUNCS
#include "debug.h"
#include "si.h"
#include "jsobj.h"

static JSBool si_getprop(JSContext *cx, JSObject *obj, jsval id, jsval *rval);
static JSBool si_setprop(JSContext *cx, JSObject *obj, jsval id, jsval *rval);

static JSClass si_data_class = {
	"si_data",		/* Name */
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

JSObject *JSSIData(JSContext *cx, si_session_t *s) {
	JSAliasSpec si_data_aliases[] = {
		{ "battery_soc", "battery_level" },
		{ "ac1_voltage", "output_voltage" },
		{ "ac1_frequency", "output_frequency" },
		{ "ac1_current", "output_current" },
		{ "ac1_power", "output_power" },
		{ "ac2_voltage", "input_voltage" },
		{ "ac2_frequency", "input_frequency" },
		{ "ac2_current", "input_current" },
		{ "ac2_power", "input_power" },
		{ "TotLodPwr", "load" },
		{ 0 }
	};
	JSObject *obj;

	if (!s->data_props) {
		s->data_props = config_to_props(s->ap->cp, "si_data", 0);
		dprintf(dlevel,"info->props: %p\n",s->data_props);
		if (!s->data_props) {
			log_error("unable to create si_data props: %s\n", config_get_errmsg(s->ap->cp));
			return 0;
		}
	}

	dprintf(dlevel,"defining %s object\n",si_data_class.name);
	obj = JS_InitClass(cx, JS_GetGlobalObject(cx), 0, &si_data_class, 0, 0, s->data_props, 0, 0, 0);
	if (!obj) {
		JS_ReportError(cx,"unable to initialize %s", si_data_class.name);
		return 0;
	}
	dprintf(dlevel,"defining %s aliases\n",si_data_class.name);
	if (!JS_DefineAliases(cx, obj, si_data_aliases)) {
		JS_ReportError(cx,"unable to define aliases");
		return 0;
	}
	JS_SetPrivate(cx,obj,s);
	dprintf(dlevel,"done!\n");
	return obj;
}

/*************************************************************************/

#define	SI_PROPERTY_ID_DATA 1024
#define	SI_PROPERTY_ID_BA 1025

static JSBool si_getprop(JSContext *cx, JSObject *obj, jsval id, jsval *rval) {
	int prop_id;
	si_session_t *s;
	config_property_t *p;

	s = JS_GetPrivate(cx,obj);
	if(JSVAL_IS_INT(id)) {
		prop_id = JSVAL_TO_INT(id);
		dprintf(dlevel,"prop_id: %d\n", prop_id);
		switch(prop_id) {
		case SI_PROPERTY_ID_DATA:
			*rval = OBJECT_TO_JSVAL(JSSIData(cx,s));
			break;
		case SI_PROPERTY_ID_BA:
			*rval = type_to_jsval(cx,DATA_TYPE_FLOAT_ARRAY,s->ba,SI_MAX_BA);
			break;
		default:
			p = CONFIG_GETMAP(s->ap->cp,prop_id);
			if (!p) p = config_get_propbyid(s->ap->cp,prop_id);
			if (!p) {
				JS_ReportError(cx, "property %d not found", prop_id);
				return JS_FALSE;
			}
//			dprintf(dlevel,"p: type: %d(%s), name: %s\n", p->type, typestr(p->type), p->name);
			*rval = type_to_jsval(cx,p->type,p->dest,p->len);
			break;
		}
	}
	return JS_TRUE;
}

static JSBool si_setprop(JSContext *cx, JSObject *obj, jsval id, jsval *rval) {
	si_session_t *s;

	s = JS_GetPrivate(cx,obj);
	if (!s) {
		JS_ReportError(cx,"private is null!");
		return JS_FALSE;
	}
	return config_jssetprop(cx, obj, id, rval, s->ap->cp, s->props);
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

//static JSBool jsget_relays(JSContext *cx, JSObject *obj, uintN argc, jsval *argv, jsval *rval) {
static JSBool jsget_relays(JSContext *cx, uintN argc, jsval *vp) {
	si_session_t *s;
	JSObject *obj;

	obj = JS_THIS_OBJECT(cx, vp);
	if (!obj) return JS_FALSE;
	s = JS_GetPrivate(cx, obj);
	if (s->can_connected && si_can_read_relays(s)) return JS_FALSE;
	return JS_TRUE;
}

static JSBool jscan_read(JSContext *cx, uintN argc, jsval *vp) {
	si_session_t *s;
	uint8_t data[8];
	int id,len,r;
	jsval *argv = vp + 2;
	JSObject *obj;

	obj = JS_THIS_OBJECT(cx, vp);
	if (!obj) return JS_FALSE;
	s = JS_GetPrivate(cx, obj);
	dprintf(1,"s: %p\n", s);

	if (argc != 2) {
		JS_ReportError(cx,"can_read requires 2 arguments (id: number, length: number)\n");
		return JS_FALSE;
	}


	if (!jsval_to_type(DATA_TYPE_INT,&id,0,cx,argv[0])) return JS_FALSE;
	dprintf(dlevel,"id: 0x%03x\n", id);
	if (!jsval_to_type(DATA_TYPE_INT,&len,0,cx,argv[1])) return JS_FALSE;
	dprintf(dlevel,"len: %d\n", len);
	if (len > 8) len = 8;
	dprintf(1,"****** id: %03x, len: %d\n", id, len);
	if (!s->can) return JS_FALSE;
	r = si_can_read(s,id,data,len);
	dprintf(4,"r: %d\n", r);
	if (r) return JS_FALSE;
	*vp = type_to_jsval(cx,DATA_TYPE_U8_ARRAY,data,len);
	return JS_TRUE;
}

static JSBool jscan_write(JSContext *cx, uintN argc, jsval *vp) {
	si_session_t *s;
	uint8_t data[8];
	int id,len,r;
	jsval *argv = vp + 2;
	JSObject *obj;
	JSClass *classp;

	obj = JS_THIS_OBJECT(cx, vp);
	if (!obj) return JS_FALSE;
	s = JS_GetPrivate(cx, obj);

	if (argc != 2) {
		JS_ReportError(cx,"can_write requires 2 arguments (id: number, data: array)\n");
		return JS_FALSE;
	}

	dprintf(dlevel,"isobj: %d\n", JSVAL_IS_OBJECT(argv[1]));
	if (!JSVAL_IS_OBJECT(argv[1])) {
		JS_ReportError(cx, "can_write: 2nd argument must be array\n");
		return JS_FALSE;
	}
	obj = JSVAL_TO_OBJECT(argv[1]);
	classp = OBJ_GET_CLASS(cx,obj);
	if (!classp) {
		JS_ReportError(cx, "can_write: 2nd argument must be array\n");
		return JS_FALSE;
	}
	dprintf(dlevel,"class: %s\n", classp->name);
	if (classp && strcmp(classp->name,"Array")) {
		JS_ReportError(cx, "can_write: 2nd argument must be array\n");
		return JS_FALSE;
	}

	dprintf(dlevel,"id: 0x%03x\n", id);

	if (!jsval_to_type(DATA_TYPE_INT,&id,0,cx,argv[0])) return JS_FALSE;
	len = jsval_to_type(DATA_TYPE_U8_ARRAY,data,8,cx,argv[1]);
	dprintf(dlevel,"len: %d\n", len);
//	if (debug >= dlevel+1) bindump("data",data,len);
	r = si_can_write(s,id,data,len);
	dprintf(dlevel,"r: %d\n", r);
	*vp = INT_TO_JSVAL(r);
	return JS_TRUE;
}

static JSBool jsinit_can(JSContext *cx, uintN argc, jsval *vp) {
	si_session_t *s;
	JSObject *obj;

	obj = JS_THIS_OBJECT(cx, vp);
	if (!obj) return JS_FALSE;
	s = JS_GetPrivate(cx, obj);
	*vp = BOOLEAN_TO_JSVAL(si_can_init(s));
	return JS_TRUE;
}

static JSBool jsinit_smanet(JSContext *cx, uintN argc, jsval *vp) {
	si_session_t *s;
	JSObject *obj;

	obj = JS_THIS_OBJECT(cx, vp);
	if (!obj) return JS_FALSE;
	s = JS_GetPrivate(cx, obj);
	*vp = BOOLEAN_TO_JSVAL(si_smanet_init(s));
	return JS_TRUE;
}

JSObject *js_init_candev(JSContext *cx, void *priv) {
	si_session_t *s = priv;
	JSObject *newobj;
	jsval can;

	newobj = jscan_new(cx,s->can,s->can_handle,s->can_transport,s->can_target,s->can_topts,&s->can_connected);
	can = OBJECT_TO_JSVAL(newobj);
	JS_DefineProperty(cx, JS_GetGlobalObject(cx), "can", can, 0, 0, 0);
	return newobj;
}

JSObject *js_init_smanetdev(JSContext *cx, void *priv) {
	si_session_t *s = priv;
	JSObject *newobj;
	jsval val;

	dprintf(dlevel,"s: %p, smanet: %p\n", s, (s ? s->smanet : 0));
	newobj = jssmanet_new(cx,s->smanet,s->smanet_transport,s->smanet_target,s->smanet_topts);
	val = OBJECT_TO_JSVAL(newobj);
	JS_DefineProperty(cx, JS_GetGlobalObject(cx), "smanet", val, 0, 0, 0);
	return newobj;
}

JSObject *JSSunnyIsland(JSContext *cx, void *priv) {
	JSPropertySpec si_props[] = {
		{ "data", SI_PROPERTY_ID_DATA, JSPROP_ENUMERATE | JSPROP_READONLY },
		{ "ba", SI_PROPERTY_ID_BA, JSPROP_ENUMERATE | JSPROP_READONLY },
		{ 0 }
	};
	JSFunctionSpec si_funcs[] = {
		JS_FN("get_relays",jsget_relays,0,0,0),
		JS_FN("can_read",jscan_read,1,1,0),
		JS_FN("can_write",jscan_write,2,2,0),
		JS_FN("smanet_get",jscan_read,1,1,0),
		JS_FN("smanet_set",jscan_read,1,1,0),
		JS_FN("can_init",jsinit_can,0,0,0),
		JS_FN("smanet_init",jsinit_smanet,0,0,0),
		{ 0 }
	};
	JSAliasSpec si_aliases[] = {
		{ "charge_amps", "charge_current" },
		{ 0 }
	};
	JSConstantSpec si_constants[] = {
		JS_NUMCONST(SI_STATE_RUNNING),
		JS_NUMCONST(SI_VOLTAGE_MIN),
		JS_NUMCONST(SI_VOLTAGE_MAX),
		JS_NUMCONST(SI_CONFIG_FLAG_SMANET),
		JS_NUMCONST(CV_METHOD_TIME),
		JS_NUMCONST(CV_METHOD_AMPS),
		JS_NUMCONST(CURRENT_SOURCE_NONE),
		JS_NUMCONST(CURRENT_SOURCE_CALCULATED),
		JS_NUMCONST(CURRENT_SOURCE_CAN),
		JS_NUMCONST(CURRENT_SOURCE_SMANET),
		JS_NUMCONST(CURRENT_TYPE_AMPS),
		JS_NUMCONST(CURRENT_TYPE_WATTS),
		JS_NUMCONST(SI_MAX_BA),
		{ 0 }
	};
	JSObject *obj;
	si_session_t *s = priv;

	dprintf(dlevel,"s->props: %p, cp: %p\n",s->props,s->ap->cp);
	if (!s->props) {

		s->props = config_to_props(s->ap->cp, "si", si_props);
		dprintf(dlevel,"s->props: %p\n",s->props);
		if (!s->props) {
			log_error("unable to create props: %s\n", config_get_errmsg(s->ap->cp));
			return 0;
		}
	}

	dprintf(dlevel,"Defining %s object\n",si_class.name);
	obj = JS_InitClass(cx, JS_GetGlobalObject(cx), 0, &si_class, 0, 0, s->props, si_funcs, 0, 0);
	if (!obj) {
		JS_ReportError(cx,"unable to initialize si class");
		return 0;
	}
	dprintf(dlevel,"Defining %s aliases\n",si_class.name);
	if (!JS_DefineAliases(cx, obj, si_aliases)) {
		JS_ReportError(cx,"unable to define aliases");
		return 0;
	}
	dprintf(dlevel,"Defining %s constants\n",si_class.name);
	if (!JS_DefineConstants(cx, JS_GetGlobalObject(cx), si_constants)) {
		JS_ReportError(cx,"unable to define constants");
		return 0;
	}
	JS_SetPrivate(cx,obj,priv);
	dprintf(dlevel,"done!\n");
	return obj;
}

int si_jsinit(si_session_t *s) {
	JS_EngineAddInitFunc(s->ap->js, "can", js_init_candev, s);
	JS_EngineAddInitFunc(s->ap->js, "smanet", js_init_smanetdev, s);
	return JS_EngineAddInitFunc(s->ap->js, "si", JSSunnyIsland, s);
}

#endif

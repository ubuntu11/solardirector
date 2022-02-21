
#ifdef JS

#define DEBUG_JSFUNCS 0
#define dlevel 4

#ifdef DEBUG
#undef DEBUG
#endif
#define DEBUG DEBUG_JSFUNCS
#include "debug.h"
#include "sb.h"
#include "jsobj.h"
#include "jsarray.h"

enum SMA_OBJECT_PROPERTY_ID {
	SMA_OBJECT_PROPERTY_ID_KEY,
	SMA_OBJECT_PROPERTY_ID_LABEL,
	SMA_OBJECT_PROPERTY_ID_PATH,
	SMA_OBJECT_PROPERTY_ID_UNIT,
	SMA_OBJECT_PROPERTY_ID_MULT
};

static JSBool sb_object_getprop(JSContext *cx, JSObject *obj, jsval id, jsval *rval) {
	int prop_id;
	sb_object_t *o;

	o = JS_GetPrivate(cx,obj);
	if (!o) return JS_FALSE;
	if(JSVAL_IS_INT(id)) {
		prop_id = JSVAL_TO_INT(id);
		dprintf(dlevel,"prop_id: %d\n", prop_id);
		switch(prop_id) {
                case SMA_OBJECT_PROPERTY_ID_KEY:
                        *rval = STRING_TO_JSVAL(JS_NewStringCopyZ(cx,o->key));
                        break;
                case SMA_OBJECT_PROPERTY_ID_LABEL:
                        *rval = STRING_TO_JSVAL(JS_NewStringCopyZ(cx,o->label));
                        break;
                case SMA_OBJECT_PROPERTY_ID_PATH:
		    {
			JSObject *arr;
			jsval element;
			int i,count;

			for(i=0; i < 8; i++) { if (!o->hier[i]) break; }
			count = i;
			arr = JS_NewArrayObject(cx, count, NULL);
			for(i=0; i < count; i++) {
				element = STRING_TO_JSVAL(JS_NewStringCopyZ(cx,o->hier[i]));
				JS_SetElement(cx, arr, i, &element);
			}
			*rval = OBJECT_TO_JSVAL(arr);
		    }
		    break;
                case SMA_OBJECT_PROPERTY_ID_UNIT:
			*rval = STRING_TO_JSVAL(JS_NewStringCopyZ(cx,o->unit));
			break;
                case SMA_OBJECT_PROPERTY_ID_MULT:
                        JS_NewDoubleValue(cx, o->mult, rval);
                        break;
		default:
			dprintf(0,"unhandled id: %d\n", prop_id);
			*rval = JSVAL_NULL;
			break;
		}
	}
	return JS_TRUE;
}

static JSClass sb_object_class = {
	"sb_object",		/* Name */
	JSCLASS_HAS_PRIVATE,	/* Flags */
	JS_PropertyStub,	/* addProperty */
	JS_PropertyStub,	/* delProperty */
	sb_object_getprop,	/* getProperty */
	JS_PropertyStub,	/* setProperty */
	JS_EnumerateStub,	/* enumerate */
	JS_ResolveStub,		/* resolve */
	JS_ConvertStub,		/* convert */
	JS_FinalizeStub,	/* finalize */
	JSCLASS_NO_OPTIONAL_MEMBERS
};

JSObject *JSSBObject(JSContext *cx, sb_object_t *o) {
	JSPropertySpec sb_object_props[] = {
		{ "key",SMA_OBJECT_PROPERTY_ID_KEY,JSPROP_ENUMERATE },
		{ "label",SMA_OBJECT_PROPERTY_ID_LABEL,JSPROP_ENUMERATE },
		{ "path",SMA_OBJECT_PROPERTY_ID_PATH,JSPROP_ENUMERATE },
		{ "unit",SMA_OBJECT_PROPERTY_ID_UNIT,JSPROP_ENUMERATE },
		{ "mult",SMA_OBJECT_PROPERTY_ID_MULT,JSPROP_ENUMERATE },
		{ 0 }
	};
	JSObject *obj;

	dprintf(dlevel,"defining %s object\n",sb_object_class.name);
	obj = JS_InitClass(cx, JS_GetGlobalObject(cx), 0, &sb_object_class, 0, 0, sb_object_props, 0, 0, 0);
	if (!obj) {
		JS_ReportError(cx,"unable to initialize %s", sb_object_class.name);
		return 0;
	}
	JS_SetPrivate(cx,obj,o);
	dprintf(dlevel,"done!\n");
	return obj;
}


enum SB_RESULTS_PROPERTY_ID {
	SB_RESULTS_PROPERTY_ID_OBJECT,
	SB_RESULTS_PROPERTY_ID_LOW,
	SB_RESULTS_PROPERTY_ID_HIGH,
	SB_RESULTS_PROPERTY_ID_SELECTIONS,
	SB_RESULTS_PROPERTY_ID_VALUES
};

static JSBool sb_results_getprop(JSContext *cx, JSObject *obj, jsval id, jsval *rval) {
	int prop_id;
	sb_result_t *res;

	res = JS_GetPrivate(cx,obj);
	if (!res) return JS_FALSE;
	if(JSVAL_IS_INT(id)) {
		prop_id = JSVAL_TO_INT(id);
		dprintf(dlevel,"prop_id: %d\n", prop_id);
		switch(prop_id) {
		case SB_RESULTS_PROPERTY_ID_OBJECT:
			*rval = OBJECT_TO_JSVAL(JSSBObject(cx,res->obj));
			break;
		case SB_RESULTS_PROPERTY_ID_LOW:
			*rval = INT_TO_JSVAL(res->low);
			break;
		case SB_RESULTS_PROPERTY_ID_HIGH:
			*rval = INT_TO_JSVAL(res->high);
			break;
		case SB_RESULTS_PROPERTY_ID_SELECTIONS:
			*rval = type_to_jsval(cx,DATA_TYPE_STRING_LIST,res->selects,list_count(res->selects));
			break;
		case SB_RESULTS_PROPERTY_ID_VALUES:
		    {
			JSObject *arr;
			jsval element;
			sb_value_t *vp;
			char value[1024];
			int i;

//			printf("value count: %d\n", list_count(res->values));
			arr = JS_NewArrayObject(cx, list_count(res->values), NULL);
			i = 0;
			list_reset(res->values);
			while((vp = list_get_next(res->values)) != 0) {
				element = type_to_jsval(cx,vp->type,vp->data,vp->len);
//				dprintf(dlevel,"type: %s\n", js_typestr(element));
				*value = 0;
				conv_type(DATA_TYPE_STRING,value,sizeof(value),vp->type,vp->data,vp->len);
				dprintf(dlevel,"value: %s\n", value);
//				element = STRING_TO_JSVAL(JS_NewStringCopyZ(cx,value));
				JS_SetElement(cx, arr, i++, &element);
			}
			*rval = OBJECT_TO_JSVAL(arr);
		    }
		    break;
		default:
			dprintf(0,"unhandled id: %d\n", prop_id);
			*rval = JSVAL_NULL;
			break;
		}
	}
	return JS_TRUE;
}

static JSClass sb_results_class = {
	"sb_results",		/* Name */
	JSCLASS_HAS_PRIVATE,	/* Flags */
	JS_PropertyStub,	/* addProperty */
	JS_PropertyStub,	/* delProperty */
	sb_results_getprop,	/* getProperty */
	JS_PropertyStub,	/* setProperty */
	JS_EnumerateStub,	/* enumerate */
	JS_ResolveStub,		/* resolve */
	JS_ConvertStub,		/* convert */
	JS_FinalizeStub,	/* finalize */
	JSCLASS_NO_OPTIONAL_MEMBERS
};

JSObject *JSSBResults(JSContext *cx, sb_result_t *res) {
	JSPropertySpec sb_results_props[] = {
		{ "object",SB_RESULTS_PROPERTY_ID_OBJECT,JSPROP_ENUMERATE },
		{ "low",SB_RESULTS_PROPERTY_ID_LOW,JSPROP_ENUMERATE },
		{ "high",SB_RESULTS_PROPERTY_ID_HIGH,JSPROP_ENUMERATE },
		{ "selections",SB_RESULTS_PROPERTY_ID_SELECTIONS,JSPROP_ENUMERATE },
		{ "values",SB_RESULTS_PROPERTY_ID_VALUES,JSPROP_ENUMERATE },
		{ 0 }
	};
	JSObject *obj;

	dprintf(dlevel,"defining %s object\n",sb_results_class.name);
	obj = JS_InitClass(cx, JS_GetGlobalObject(cx), 0, &sb_results_class, 0, 0, sb_results_props, 0, 0, 0);
	if (!obj) {
		JS_ReportError(cx,"unable to initialize %s", sb_results_class.name);
		return 0;
	}
	JS_SetPrivate(cx,obj,res);
	dprintf(dlevel,"done!\n");
	return obj;
}

/*************************************************************************/

#define	SB_PROPERTY_ID_RESULTS 1024

static JSBool sb_getprop(JSContext *cx, JSObject *obj, jsval id, jsval *rval) {
	int prop_id;
	sb_session_t *s;
	sb_result_t *res;
	config_property_t *p;

	s = JS_GetPrivate(cx,obj);
	if(JSVAL_IS_INT(id)) {
		prop_id = JSVAL_TO_INT(id);
		dprintf(dlevel,"prop_id: %d\n", prop_id);
		switch(prop_id) {
		case SB_PROPERTY_ID_RESULTS:
		    {
			JSObject *arr;
			jsval element;
			int i;

			arr = JS_NewArrayObject(cx, list_count(s->results), NULL);
			i = 0;
			list_reset(s->results);
			while((res = list_get_next(s->results)) != 0) {
				element = OBJECT_TO_JSVAL(JSSBResults(cx,res));
				JS_SetElement(cx, arr, i++, &element);
			}
			*rval = OBJECT_TO_JSVAL(arr);
		    }
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

static JSBool sb_setprop(JSContext *cx, JSObject *obj, jsval id, jsval *rval) {
	sb_session_t *s;
	int ok;

	s = JS_GetPrivate(cx,obj);
	ok = config_jssetprop(cx, obj, id, rval, s->ap->cp, s->props);
	return ok;
}


static JSClass sb_class = {
	"sb",			/* Name */
	JSCLASS_HAS_PRIVATE,	/* Flags */
	JS_PropertyStub,	/* addProperty */
	JS_PropertyStub,	/* delProperty */
	sb_getprop,		/* getProperty */
	sb_setprop,		/* setProperty */
	JS_EnumerateStub,	/* enumerate */
	JS_ResolveStub,		/* resolve */
	JS_ConvertStub,		/* convert */
	JS_FinalizeStub,	/* finalize */
	JSCLASS_NO_OPTIONAL_MEMBERS
};

#if 0
//static JSBool jsget_relays(JSContext *cx, JSObject *obj, uintN argc, jsval *argv, jsval *rval) {
static JSBool jsget_relays(JSContext *cx, uintN argc, jsval *vp) {
	sb_session_t *s;
	JSObject *obj;

	obj = JS_THIS_OBJECT(cx, vp);
	if (!obj) return JS_FALSE;
	s = JS_GetPrivate(cx, obj);
	if (s->can_connected && sb_can_read_relays(s)) return JS_FALSE;
	/Conreturn JS_TRUE;
}

static JSBool jscan_read(JSContext *cx, uintN argc, jsval *vp) {
	sb_session_t *s;
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
	r = sb_can_read(s,id,data,len);
	dprintf(4,"r: %d\n", r);
	if (r) return JS_FALSE;
	*vp = type_to_jsval(cx,DATA_TYPE_U8_ARRAY,data,len);
	return JS_TRUE;
}

static JSBool jscan_write(JSContext *cx, uintN argc, jsval *vp) {
	sb_session_t *s;
	uint8_t data[8];
	int id,len,r;
	jsval *argv = vp + 2;
	JSObject *obj;

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
	r = sb_can_write(s,id,data,len);
	dprintf(dlevel,"r: %d\n", r);
	*vp = INT_TO_JSVAL(r);
	return JS_TRUE;
}

static JSBool jsinit_can(JSContext *cx, uintN argc, jsval *vp) {
	sb_session_t *s;
	JSObject *obj;

	obj = JS_THIS_OBJECT(cx, vp);
	if (!obj) return JS_FALSE;
	s = JS_GetPrivate(cx, obj);
	*vp = BOOLEAN_TO_JSVAL(sb_can_init(s));
	return JS_TRUE;
}

static JSBool jsinit_smanet(JSContext *cx, uintN argc, jsval *vp) {
	sb_session_t *s;
	JSObject *obj;

	obj = JS_THIS_OBJECT(cx, vp);
	if (!obj) return JS_FALSE;
	s = JS_GetPrivate(cx, obj);
	*vp = BOOLEAN_TO_JSVAL(sb_smanet_init(s));
	return JS_TRUE;
}

JSObject *js_init_candev(JSContext *cx, void *priv) {
	sb_session_t *s = priv;
	JSObject *newobj;
	jsval can;

	newobj = jscan_new(cx,s->can,s->can_handle,s->can_transport,s->can_target,s->can_topts,&s->can_connected);
	can = OBJECT_TO_JSVAL(newobj);
	JS_DefineProperty(cx, JS_GetGlobalObject(cx), "can", can, 0, 0, 0);
	return newobj;
}

JSObject *js_init_smanetdev(JSContext *cx, void *priv) {
	sb_session_t *s = priv;
	JSObject *newobj;
	jsval val;

	dprintf(dlevel,"s: %p, smanet: %p\n", s, (s ? s->smanet : 0));
	newobj = jssmanet_new(cx,s->smanet,s->smanet_transport,s->smanet_target,s->smanet_topts);
	val = OBJECT_TO_JSVAL(newobj);
	JS_DefineProperty(cx, JS_GetGlobalObject(cx), "smanet", val, 0, 0, 0);
	return newobj;
}
#endif

static JSBool sb_jsopen(JSContext *cx, uintN argc, jsval *vp) {
	sb_session_t *s;
	JSObject *obj;

	obj = JS_THIS_OBJECT(cx, vp);
	if (!obj) return JS_FALSE;
	s = JS_GetPrivate(cx, obj);
	*vp = BOOLEAN_TO_JSVAL(sb_driver.open(s));
	return JS_TRUE;
}

static JSBool sb_jsmkfields(JSContext *cx, uintN argc, jsval *vp) {
	JSObject *obj;
	JSClass *classp;
	jsval *argv;
	jsuint count;
	char **keys,*fields;
	int len,i;

	if (argc != 1) {
		JS_ReportError(cx,"mkfields requires 1 argument (keys:array)\n");
		return JS_FALSE;
	}

 	argv = JS_ARGV(cx,vp);
	obj = JSVAL_TO_OBJECT(argv[0]);
	classp = OBJ_GET_CLASS(cx,obj);
	if (!classp) {
		JS_ReportError(cx, "mkfields: argument must be array\n");
		return JS_FALSE;
	}
	dprintf(dlevel,"class: %s\n", classp->name);
	if (classp && strcmp(classp->name,"Array")) {
		JS_ReportError(cx, "mkfields: argument must be array\n");
		return JS_FALSE;
	}

	/* Get the array length */
	if (!js_GetLengthProperty(cx, obj, &count)) {
		JS_ReportError(cx,"unable to get array length");
		return 0;
	}
	dprintf(dlevel,"count: %d\n", count);

	keys = malloc(count * sizeof(char *));
	dprintf(dlevel,"keys: %p\n", keys);
	if (!keys) {
		JS_ReportError(cx,"mkfields: malloc(%d): %s\n", count * sizeof(char *),strerror(errno));
		return JS_FALSE;
	}
	memset(keys,0,count * sizeof(char *));
	len = jsval_to_type(DATA_TYPE_STRING_ARRAY,keys,count,cx,argv[0]);
	dprintf(dlevel,"len: %d\n", len);
	fields = sb_mkfields(keys,len);
	if (!fields) {
		JS_ReportError(cx,"mkfields: error!\n");
		return JS_FALSE;
	}
	/* string array dest must be freed */
	for(i=0; i < len; i++) free(keys[i]);
	free(keys);
	*vp = STRING_TO_JSVAL(JS_NewStringCopyZ(cx,fields));
	free(fields);
	return JS_TRUE;
}

static JSBool sb_jsrequest(JSContext *cx, uintN argc, jsval *vp) {
	sb_session_t *s;
	JSObject *obj;
	char *func, *fields;

	obj = JS_THIS_OBJECT(cx, vp);
	if (!obj) return JS_FALSE;
	s = JS_GetPrivate(cx, obj);
	if (argc != 2) {
		JS_ReportError(cx,"request requires 2 arguments (func:string, fields:string)\n");
		return JS_FALSE;
	}

	func = fields = 0;
	if (!JS_ConvertArguments(cx, argc, JS_ARGV(cx,vp), "s s", &func, &fields)) return JS_FALSE;
	dprintf(1,"func: %s, fields: %s\n", func, fields);

	*vp = BOOLEAN_TO_JSVAL(sb_request(s,func,fields));
	return JS_TRUE;
}

static JSBool sb_jsclose(JSContext *cx, uintN argc, jsval *vp) {
	sb_session_t *s;
	JSObject *obj;

	obj = JS_THIS_OBJECT(cx, vp);
	if (!obj) return JS_FALSE;
	s = JS_GetPrivate(cx, obj);
	*vp = BOOLEAN_TO_JSVAL(sb_driver.close(s));
	return JS_TRUE;
}

JSObject *JSSunnyBoy(JSContext *cx, void *priv) {
	JSPropertySpec sb_props[] = {
		{ "results", SB_PROPERTY_ID_RESULTS, JSPROP_ENUMERATE | JSPROP_READONLY },
		{ 0 }
	};
	JSFunctionSpec sb_funcs[] = {
		JS_FN("open",sb_jsopen,0,0,0),
		JS_FN("mkfields",sb_jsmkfields,0,0,0),
		JS_FN("request",sb_jsrequest,3,3,0),
		JS_FN("close",sb_jsclose,0,0,0),
		{ 0 }
	};
	JSAliasSpec sb_aliases[] = {
		{ 0 }
	};
	JSConstantSpec sb_constants[] = {
		JS_STRCONST(SB_LOGIN),
		JS_STRCONST(SB_LOGOUT),
		JS_STRCONST(SB_GETVAL),
		JS_STRCONST(SB_LOGGER),
		JS_STRCONST(SB_ALLVAL),
		JS_STRCONST(SB_ALLPAR),
		JS_STRCONST(SB_SETPAR),
		{ 0 }
	};
	JSObject *obj;
	sb_session_t *s = priv;

	dprintf(dlevel,"s->props: %p, cp: %p\n",s->props,s->ap->cp);
	if (!s->props) {

		s->props = config_to_props(s->ap->cp, "sb", sb_props);
		dprintf(dlevel,"s->props: %p\n",s->props);
		if (!s->props) {
			log_error("unable to create props: %s\n", config_get_errmsg(s->ap->cp));
			return 0;
		}
	}

	dprintf(dlevel,"Defining %s object\n",sb_class.name);
	obj = JS_InitClass(cx, JS_GetGlobalObject(cx), 0, &sb_class, 0, 0, s->props, sb_funcs, 0, 0);
	if (!obj) {
		JS_ReportError(cx,"unable to initialize si class");
		return 0;
	}
	dprintf(dlevel,"Defining %s aliases\n",sb_class.name);
	if (!JS_DefineAliases(cx, obj, sb_aliases)) {
		JS_ReportError(cx,"unable to define aliases");
		return 0;
	}
	dprintf(dlevel,"Defining %s constants\n",sb_class.name);
	if (!JS_DefineConstants(cx, JS_GetGlobalObject(cx), sb_constants)) {
		JS_ReportError(cx,"unable to define constants");
		return 0;
	}
	JS_SetPrivate(cx,obj,priv);
	dprintf(dlevel,"done!\n");
	return obj;
}

int sb_jsinit(sb_session_t *s) {
//	JS_EngineAddInitFunc(s->ap->js, "can", js_init_candev, s);
//	JS_EngineAddInitFunc(s->ap->js, "smanet", js_init_smanetdev, s);
	return JS_EngineAddInitFunc(s->ap->js, "sb", JSSunnyBoy, s);
}

#endif

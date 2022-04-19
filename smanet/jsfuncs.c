
#ifdef JS
#include "smanet.h"
#include "jsengine.h"
#include "debug.h"
#include "transports.h"
#include "jsobj.h"
#include "jsarray.h"

#define dlevel 6

enum SMANET_PROPERTY_ID {
	SMANET_PROPERTY_ID_CONNECTED,
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
	SMANET_PROPERTY_ID_ERRMSG,
	SMANET_PROPERTY_ID_DOARRAY,
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
		case SMANET_PROPERTY_ID_TRANSPORT:
			*rval = type_to_jsval(cx,DATA_TYPE_STRING,s->transport,strlen(s->transport));
			break;
		case SMANET_PROPERTY_ID_TARGET:
			*rval = type_to_jsval(cx,DATA_TYPE_STRING,s->target,strlen(s->target));
			break;
		case SMANET_PROPERTY_ID_TOPTS:
			*rval = type_to_jsval(cx,DATA_TYPE_STRING,s->topts,strlen(s->topts));
			break;
		case SMANET_PROPERTY_ID_CONNECTED:
			*rval = BOOLEAN_TO_JSVAL(s->connected);
			break;
		case SMANET_PROPERTY_ID_CHANNELS:
			/* XXX TODO */
			*rval = JSVAL_VOID;
			break;
		case SMANET_PROPERTY_ID_SERIAL:
			*rval = INT_TO_JSVAL(s->serial);
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
		case SMANET_PROPERTY_ID_ERRMSG:
			*rval = type_to_jsval(cx,DATA_TYPE_STRING,s->errmsg,strlen(s->errmsg));
			break;
		case SMANET_PROPERTY_ID_DOARRAY:
			*rval = BOOLEAN_TO_JSVAL(s->doarray);
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
	if (!s) {
		JS_ReportError(cx, "smanet_setprop: private is null!\n");
		return JS_FALSE;
	}
	if (JSVAL_IS_INT(id)) {
		prop_id = JSVAL_TO_INT(id);
		dprintf(dlevel,"smanet_setprop: prop_id: %d", prop_id);
		switch(prop_id) {
		case SMANET_PROPERTY_ID_DOARRAY:
			s->doarray = JSVAL_TO_BOOLEAN(*vp);
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

static char *js_string(JSContext *cx, jsval val) {
	JSString *str;
	char *bytes;

	str = JS_ValueToString(cx, val);
	if (!str) return 0;
	bytes = JS_EncodeString(cx, str);
	if (!bytes) return 0;
	return bytes;
}

static JSClass js_mr_class = {
	"SMANETResults",	/* Name */
	JSCLASS_HAS_PRIVATE,	/* Flags */
	JS_PropertyStub,	/* addProperty */
	JS_PropertyStub,	/* delProperty */
	JS_PropertyStub,	/* getProperty */
	JS_PropertyStub,	/* setProperty */
	JS_EnumerateStub,	/* enumerate */
	JS_ResolveStub,		/* resolve */
	JS_ConvertStub,		/* convert */
	JS_FinalizeStub,	/* finalize */
	JSCLASS_NO_OPTIONAL_MEMBERS
};

static jsval _mr_to_array(JSContext *cx, smanet_multreq_t *mr, int count) {
	JSObject *arr,*ea;
	jsval ae,ne,ve;
	int i;

//	obj = js_NewObject(cx, &js_mr_class, NULL, NULL, 0);

	arr = JS_NewArrayObject(cx, count, NULL);
	for(i=0; i < count; i++) {
		/* Create a sub array of [ name, value ] */
		ea = JS_NewArrayObject(cx, 2, NULL);
		dprintf(1,"mr[%d]: value: %f, text: %s\n", i, mr[i].value, mr[i].text);
		ne = type_to_jsval(cx,DATA_TYPE_STRING,mr[i].name,strlen(mr[i].name));
		JS_SetElement(cx, ea, 0, &ne);
		if (mr[i].text) {
			ve = STRING_TO_JSVAL(JS_NewStringCopyZ(cx,mr[i].text));
		} else {
			JS_NewNumberValue(cx, mr[i].value, &ve);
		}
		JS_SetElement(cx, ea, 1, &ve);
		ae = OBJECT_TO_JSVAL(ea);
		JS_SetElement(cx, arr, i, &ae);
	}
	return OBJECT_TO_JSVAL(arr);
}

static jsval _mr_to_object(JSContext *cx, smanet_multreq_t *mr, int count) {
	JSObject *obj;
	jsval val;
	int i,flags;

	obj = js_NewObject(cx, &js_mr_class, NULL, NULL, 0);

	flags = JSPROP_ENUMERATE | JSPROP_READONLY;
	for(i=0; i < count; i++) {
		dprintf(1,"mr[%d]: value: %f, text: %s\n", i, mr[i].value, mr[i].text);
		if (mr[i].text) {
			val = STRING_TO_JSVAL(JS_NewStringCopyZ(cx,mr[i].text));
		} else {
			JS_NewNumberValue(cx, mr[i].value, &val);
		}
//		dprintf(0,"adding: %s\n", mr[i].name);
		JS_DefineProperty(cx, obj, mr[i].name, val, JS_PropertyStub, JS_PropertyStub, flags);
	}
	return OBJECT_TO_JSVAL(obj);
}

static smanet_multreq_t *_array_to_mr(JSContext *cx, JSObject *obj, unsigned int *count) {
	smanet_multreq_t *mr;
	JSString *str;
	jsval element;
	int i,size;

	if (!js_GetLengthProperty(cx, obj, count)) return 0;

	size = *count*sizeof(smanet_multreq_t);
	mr = malloc(size);
	if (!mr) {
		log_syserror("_array_to_mr: malloc(%d)",size);
		return 0;
	}
	for(i=0; i < *count; i++) {
		JS_GetElement(cx, obj, (jsint)i, &element);
		str = JS_ValueToString(cx,element);
		if (!str) continue;
		mr[i].name = JS_EncodeString(cx, str);
		if (!mr[i].name) continue;
//		dprintf(0,"====> element[%d]: %s\n", i, mr[i].name);
	}

	return mr;
}

static JSBool js_smanet_get(JSContext *cx, JSObject *obj, uintN argc, jsval *argv, jsval *rval) {
	smanet_session_t *s;
	int i;

	/* There are 2 forms to the get call: first, all strings ("a","b","c") 
	   OR a single array (["a","b","c"]) */
	s = JS_GetPrivate(cx, obj);
	if (!s) {
		sprintf(s->errmsg,"SMANET private is null!");
		goto js_smanet_get_error;
	}
	if (!s->connected) {
		sprintf(s->errmsg,"SMANET.get: not connected");
		goto js_smanet_get_error;
	}
#ifdef SMANET_THREADSAFE
	dprintf(dlevel,"locking...\n");
	smanet_lock(s);
#endif
	dprintf(dlevel,"argc: %d\n", argc);
	if (argc < 1) {
		sprintf(s->errmsg,"SMANET.get: at least 1 argument is required");
		goto js_smanet_get_error;
	}
	/* if more than 1 arg ... make sure they're all strings */
	if (argc > 1) {
		smanet_multreq_t *mr;
		int mr_size;

		int allstr = 1;
		for(i = 0; i < argc; i++) {
//			printf("==> argv[%d]: %s\n", i,jstypestr(cx,argv[i]));
			if (!JSVAL_IS_STRING(argv[i])) {
				allstr = 0;
				break;
			}
		}
		if (!allstr) {
			sprintf(s->errmsg,"SMANET.get: if more than 1 argument, they must all be strings");
			goto js_smanet_get_error;
		}

		/* Alloc the MR struct */
		mr_size = argc * sizeof(smanet_multreq_t);
		dprintf(1,"mr_size: %d\n", mr_size);
		mr = malloc(mr_size);
		if (!mr) {
			sprintf(s->errmsg,"SMANET.get: malloc(%d): %s\n", mr_size, strerror(errno));
			goto js_smanet_get_error;
		}
		for(i=0; i < argc; i++) mr[i].name = js_string(cx, argv[i]);
		if (smanet_get_multvalues(s,mr,argc)) {
			free(mr);
			sprintf(s->errmsg,"SMANET.get: error calling smanet_get_multvalues");
			goto js_smanet_get_error;
		}
		*rval = (s->doarray ? _mr_to_array(cx,mr,argc) : _mr_to_object(cx,mr,argc));
		for(i=0; i < argc; i++) JS_free(cx, mr[i].name);
		free(mr);
	} else {
		/* Treat a string as a single name */
		if (JSVAL_IS_STRING(argv[0])) {
			char *name,*text;
			double value;

			name = js_string(cx,argv[0]);
			if (!name) {
				sprintf(s->errmsg,"SMANET.get: unable to get name");
				goto js_smanet_get_error;
			}
			dprintf(dlevel,"name: %s\n", name);
			if (smanet_get_value(s, name, &value, &text)) {
				JS_ReportError(cx, "error getting value: %s",s->errmsg);
				goto js_smanet_get_error;
			}
			dprintf(dlevel,"value: %f, text: %s\n", value, text);
			if (text) {
				*rval = type_to_jsval(cx,DATA_TYPE_STRING,text,strlen(text));
			} else {
				JS_NewDoubleValue(cx, value, rval);
			}

		/* If object, must be array of strings */
		} else if (JSVAL_IS_OBJECT(argv[0])) {
			JSObject *arr;
			JSClass *classp;
			smanet_multreq_t *mr;
			unsigned int count;

			/* Get the object */
			arr = JSVAL_TO_OBJECT(argv[0]);

			/* Make sure the object is an array */
			classp = OBJ_GET_CLASS(cx,arr);
			if (!classp) {
				sprintf(s->errmsg, "SMANET.get: object is not an array");
				goto js_smanet_get_error;
			}
			dprintf(dlevel,"class: %s\n", classp->name);
			if (classp && strcmp(classp->name,"Array")) {
				sprintf(s->errmsg, "SMANET.get: object is not an array");
				goto js_smanet_get_error;
			}
			mr = _array_to_mr(cx,arr,&count);
			if (!mr) {
				sprintf(s->errmsg,"SMANET.get: error getting array values");
				goto js_smanet_get_error;
			}
			if (smanet_get_multvalues(s,mr,count)) {
				free(mr);
				goto js_smanet_get_error;
			}
			*rval = (s->doarray ? _mr_to_array(cx,mr,count) : _mr_to_object(cx,mr,count));
			for(i=0; i < count; i++) JS_free(cx, mr[i].name);
			free(mr);
		} else {
			sprintf(s->errmsg,"SMANET.get: single argument must be string or array");
			goto js_smanet_get_error;
		}
	}
#ifdef SMANET_THREADSAFE
	dprintf(dlevel,"unlocking...\n");
	smanet_unlock(s);
#endif
	return JS_TRUE;

js_smanet_get_error:
#ifdef SMANET_THREADSAFE
	dprintf(dlevel,"unlocking...\n");
	smanet_unlock(s);
#endif
	JS_ReportError(cx,"%s",s->errmsg);
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
#ifdef SMANET_THREADSAFE
	dprintf(dlevel,"locking...\n");
	smanet_lock(s);
#endif
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
#ifdef SMANET_THREADSAFE
	dprintf(dlevel,"unlocking...\n");
	smanet_unlock(s);
#endif
	return JS_TRUE;

jsset_error:
#ifdef SMANET_THREADSAFE
	dprintf(dlevel,"unlocking...\n");
	smanet_unlock(s);
#endif
	return JS_TRUE;
}

static JSBool jsconnect(JSContext *cx, uintN argc, jsval *vp) {
	smanet_session_t *s;
	JSObject *obj;
	int r;

	obj = JS_THIS_OBJECT(cx, vp);
	if (!obj) return JS_FALSE;
	s = JS_GetPrivate(cx, obj);
	if (!s) return JS_FALSE;

	if (argc == 3) {
		if (!JS_ConvertArguments(cx, argc, JS_ARGV(cx,vp), "s s s", &s->transport, &s->target, &s->topts))
			return JS_FALSE;
	}
	dprintf(dlevel,"transport: %s, target: %s, topts:%s\n", s->transport, s->target, s->topts);

	if (!s->transport || !s->target) {
		JS_ReportError(cx, "smanet_connect: transport and/or target not set");
		return JS_FALSE;
	}

	r = smanet_connect(s, s->transport, s->target, s->topts);
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

JSObject *jssmanet_new(JSContext *cx, JSObject *parent, smanet_session_t *s, char *transport, char *target, char *topts) {
	JSObject *newobj;

	s->transport = transport;
	s->target = target;
	s->topts = topts;
	dprintf(dlevel,"s: %p\n", s);
	newobj = js_InitSMANETClass(cx,parent);
	JS_SetPrivate(cx,newobj,s);
	return newobj;
}

JSObject *js_InitSMANETClass(JSContext *cx, JSObject *gobj) {
	JSPropertySpec smanet_props[] = { 
		{ "transport", SMANET_PROPERTY_ID_TRANSPORT, JSPROP_ENUMERATE | JSPROP_READONLY },
		{ "target", SMANET_PROPERTY_ID_TARGET, JSPROP_ENUMERATE | JSPROP_READONLY },
		{ "topts", SMANET_PROPERTY_ID_TOPTS, JSPROP_ENUMERATE | JSPROP_READONLY },
		{ "connected", SMANET_PROPERTY_ID_CONNECTED, JSPROP_ENUMERATE | JSPROP_READONLY },
		{ "channels", SMANET_PROPERTY_ID_CHANNELS, JSPROP_ENUMERATE | JSPROP_READONLY },
		{ "serial",SMANET_PROPERTY_ID_SERIAL,JSPROP_ENUMERATE | JSPROP_READONLY  },
		{ "type",SMANET_PROPERTY_ID_TYPE,JSPROP_ENUMERATE | JSPROP_READONLY  },
		{ "src",SMANET_PROPERTY_ID_SRC,JSPROP_ENUMERATE | JSPROP_READONLY  },
		{ "dest",SMANET_PROPERTY_ID_DEST,JSPROP_ENUMERATE | JSPROP_READONLY  },
		{ "timeouts",SMANET_PROPERTY_ID_TIMEOUTS,JSPROP_ENUMERATE | JSPROP_READONLY },
		{ "commands",SMANET_PROPERTY_ID_COMMANDS,JSPROP_ENUMERATE | JSPROP_READONLY  },
		{ "errmsg",SMANET_PROPERTY_ID_ERRMSG,JSPROP_ENUMERATE | JSPROP_READONLY },
		{ "return_array",SMANET_PROPERTY_ID_DOARRAY,JSPROP_ENUMERATE },
		{0}
	};
	JSFunctionSpec smanet_funcs[] = {
		JS_FN("connect",jsconnect,3,3,0),
		JS_FN("disconnect",jsdisconnect,0,0,0),
		JS_FN("get_channels",jsget_channels,0,0,0),
		JS_FN("load_channels",jsload_channels,1,1,0),
		JS_FN("save_channels",jssave_channels,1,1,0),
		JS_FS("get",js_smanet_get,1,1,0),
		JS_FS("set",jsset,2,2,0),
		{ 0 }
	};
	JSObject *obj;

	dprintf(dlevel,"creating %s class...\n",smanet_class.name);
	obj = JS_InitClass(cx, gobj, 0, &smanet_class, smanet_ctor, 3, smanet_props, smanet_funcs, 0, 0);
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

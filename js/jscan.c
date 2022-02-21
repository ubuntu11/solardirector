
/*
Copyright (c) 2021, Stephen P. Shoecraft
All rights reserved.

This source code is licensed under the BSD-style license found in the
LICENSE file in the root directory of this source tree.
*/

#include "jsapi.h"
#include "jsobj.h"
#include "transports.h"
#ifdef WINDOWS
//#include <winsock2.h>
//#include <windows.h>
struct __attribute__((packed, aligned(1))) can_frame {
	long can_id;
	unsigned char can_dlc;
        unsigned char data[8];
};
#else
#include <linux/can.h>
#endif

#define dlevel 6

JSObject *JSCAN(JSContext *cx, void *priv);

struct can_private {
	char *transport;
	char *target;
	char *topts;
#if 0
	char transport[DRIVER_TRANSPORT_SIZE];
	char target[DRIVER_TARGET_SIZE];
	char topts[DRIVER_TOPTS_SIZE];
#endif
	solard_driver_t *tp;
	void *tp_handle;
	bool *connected;
	bool internal_connected;
};
typedef struct can_private can_private_t;

solard_driver_t *can_transports[] = { &can_driver, &rdev_driver, 0 };

enum CAN_PROPERTY_ID {
	CAN_PROPERTY_ID_CONNECTED,
	CAN_PROPERTY_ID_TRANSPORT,
	CAN_PROPERTY_ID_TARGET,
	CAN_PROPERTY_ID_TOPTS,
};

static JSBool can_getprop(JSContext *cx, JSObject *obj, jsval id, jsval *rval) {
	int prop_id;
	can_private_t *p;
//	register int i;

	p = JS_GetPrivate(cx,obj);
	if(JSVAL_IS_INT(id)) {
		prop_id = JSVAL_TO_INT(id);
		dprintf(4,"prop_id: %d\n", prop_id);
		switch(prop_id) {
                case CAN_PROPERTY_ID_CONNECTED:
                        *rval = BOOLEAN_TO_JSVAL(*p->connected);
                        break;
#if 0
                case CAN_PROPERTY_ID_AUTOCONNECT:
                        *rval = BOOLEAN_TO_JSVAL(p->autoconnect);
                        break;
		case CAN_PROPERTY_ID_NAME:
			*rval = STRING_TO_JSVAL(JS_NewStringCopyZ(cx,bp->name));
			*rval = type_to_jsval(cx, DATA_TYPE_STRING, bp->name, strlen(bp->name));
			break;
		case CAN_PROPERTY_ID_CAPACITY:
			JS_NewDoubleValue(cx, bp->capacity, rval);
			break;
		case CAN_PROPERTY_ID_VOLTAGE:
			JS_NewDoubleValue(cx, bp->voltage, rval);
			break;
		case CAN_PROPERTY_ID_CURRENT:
			JS_NewDoubleValue(cx, bp->current, rval);
			break;
		case CAN_PROPERTY_ID_NTEMPS:
			dprintf(4,"ntemps: %d\n", bp->ntemps);
			*rval = INT_TO_JSVAL(bp->ntemps);
			break;
		case CAN_PROPERTY_ID_TEMPS:
		       {
				JSObject *rows;
				jsval val;

//				dprintf(4,"ntemps: %d\n", bp->ntemps);
				rows = JS_NewArrayObject(cx, bp->ntemps, NULL);
				for(i=0; i < bp->ntemps; i++) {
//					dprintf(4,"temps[%d]: %f\n", i, bp->temps[i]);
					JS_NewDoubleValue(cx, bp->temps[i], &val);
					JS_SetElement(cx, rows, i, &val);
				}
				*rval = OBJECT_TO_JSVAL(rows);
			}
			break;
		case CAN_PROPERTY_ID_NCELLS:
			dprintf(4,"ncells: %d\n", bp->ncells);
			*rval = INT_TO_JSVAL(bp->ncells);
			break;
		case CAN_PROPERTY_ID_CELLVOLT:
		       {
				JSObject *rows;
				jsval val;

				rows = JS_NewArrayObject(cx, bp->ncells, NULL);
				for(i=0; i < bp->ncells; i++) {
					JS_NewDoubleValue(cx, bp->cells[i].voltage, &val);
					JS_SetElement(cx, rows, i, &val);
				}
				*rval = OBJECT_TO_JSVAL(rows);
			}
			break;
		case CAN_PROPERTY_ID_CELLRES:
		       {
				JSObject *rows;
				jsval val;

				rows = JS_NewArrayObject(cx, bp->ncells, NULL);
				for(i=0; i < bp->ncells; i++) {
					JS_NewDoubleValue(cx, bp->cells[i].resistance, &val);
					JS_SetElement(cx, rows, i, &val);
				}
				*rval = OBJECT_TO_JSVAL(rows);
			}
			break;
		case CAN_PROPERTY_ID_CELL_MIN:
			JS_NewDoubleValue(cx, bp->cell_min, rval);
			break;
		case CAN_PROPERTY_ID_CELL_MAX:
			JS_NewDoubleValue(cx, bp->cell_max, rval);
			break;
		case CAN_PROPERTY_ID_CELL_DIFF:
			JS_NewDoubleValue(cx, bp->cell_diff, rval);
			break;
		case CAN_PROPERTY_ID_CELL_AVG:
			JS_NewDoubleValue(cx, bp->cell_avg, rval);
			break;
		case CAN_PROPERTY_ID_CELL_TOTAL:
			JS_NewDoubleValue(cx, bp->cell_total, rval);
			break;
		case CAN_PROPERTY_ID_BALANCEBITS:
			*rval = INT_TO_JSVAL(bp->balancebits);
			break;
		case CAN_PROPERTY_ID_ERRCODE:
			*rval = INT_TO_JSVAL(bp->errcode);
			break;
		case CAN_PROPERTY_ID_ERRMSG:
			*rval = STRING_TO_JSVAL(JS_NewStringCopyZ(cx,bp->errmsg));
			break;
		case CAN_PROPERTY_ID_STATE:
			*rval = INT_TO_JSVAL(bp->state);
			break;
		case CAN_PROPERTY_ID_LAST_UPDATE:
			*rval = INT_TO_JSVAL(bp->last_update);
			break;
#endif
		default:
			JS_ReportError(cx, "not a property");
			*rval = JSVAL_NULL;
		}
	}
	return JS_TRUE;
}

static JSClass can_class = {
	"CAN",
	JSCLASS_HAS_PRIVATE,
	JS_PropertyStub,
	JS_PropertyStub,
	can_getprop,
	JS_PropertyStub,
	JS_EnumerateStub,
	JS_ResolveStub,
	JS_ConvertStub,
	JS_FinalizeStub,
	JSCLASS_NO_OPTIONAL_MEMBERS
};

static JSBool jscan_read(JSContext *cx, uintN argc, jsval *vp) {
	can_private_t *p;
	int len,r;
	uint32_t id;
	struct can_frame frame;
	jsval *argv = vp + 2;
	JSObject *obj;

	obj = JS_THIS_OBJECT(cx, vp);
	if (!obj) return JS_FALSE;
	p = JS_GetPrivate(cx, obj);

	if (argc != 2) {
		JS_ReportError(cx,"can_read requires 2 arguments (id: number, length: number)\n");
		return JS_FALSE;
	}

	if (!jsval_to_type(DATA_TYPE_INT,&id,0,cx,argv[0])) return JS_FALSE;
	if (!jsval_to_type(DATA_TYPE_U32,&len,0,cx,argv[1])) return JS_FALSE;
	dprintf(dlevel,"id: %03x, len: %d\n", id, len);
	if (len > 8) len = 8;
	r = p->tp->read(p->tp_handle,&id,&frame,sizeof(frame));
	dprintf(0,"r: %d\n", r);
	if (r != 16) *p->connected = false;
	*vp = type_to_jsval(cx,DATA_TYPE_U8_ARRAY,frame.data,frame.can_dlc);
	return JS_TRUE;
}

static JSBool jscan_write(JSContext *cx, uintN argc, jsval *vp) {
	can_private_t *p;
	uint8_t data[8];
	struct can_frame frame;
	int len,bytes;
	uint32_t id;
	jsval *argv = vp + 2;
	JSObject *obj, *array = 0;
	JSClass *classp;

	obj = JS_THIS_OBJECT(cx, vp);
	if (!obj) return JS_FALSE;
	p = JS_GetPrivate(cx, obj);

	if (argc != 2) {
		JS_ReportError(cx,"can_write requires 2 arguments (id: number, data: array)\n");
		return JS_FALSE;
	}

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

	dprintf(dlevel,"id: %d, array: %p\n", id, array);

	if (!jsval_to_type(DATA_TYPE_U32,&id,0,cx,argv[0])) return JS_FALSE;
	len = jsval_to_type(DATA_TYPE_U8_ARRAY,data,8,cx,argv[1]);
	dprintf(dlevel,"id: %03x, len: %d\n", len);

        memset(&frame,0,sizeof(frame));
        frame.can_id = id;
        frame.can_dlc = len;
        memcpy(&frame.data,data,len);
        bytes = p->tp->write(p->tp_handle,&id,&frame,sizeof(frame));
        dprintf(dlevel,"bytes: %d\n", bytes);
	if (bytes != 16) *p->connected = false;
	*vp = BOOLEAN_TO_JSVAL(bytes != 16);
	return JS_TRUE;
}

static int do_connect(JSContext *cx, can_private_t *p) {
       /* Find the driver */
	p->tp = find_driver(can_transports,p->transport);
	if (!p->tp) {
		JS_ReportError(cx,"unable to find transport: %s", p->transport);
		return JS_FALSE;
	}

	/* Create a new driver instance */
	p->tp_handle = p->tp->new(p->target, p->topts);
	if (!p->tp_handle) {
		JS_ReportError(cx, "%s_new: %s", p->transport, strerror(errno));
		return JS_FALSE;
	}

	/* Open the transport */
	dprintf(dlevel,"opening...\n");
	if (!p->tp->open) {
		JS_ReportError(cx, "internal error: transport has no open function");
		return JS_FALSE;
	}
	if (p->tp->open(p->tp_handle) == 0) *p->connected = true;

	return JS_TRUE;
}

static JSBool jscan_connect(JSContext *cx, uintN argc, jsval *vp) {
	can_private_t *p;
//	jsval *argv = JS_ARGV(cx,vp);
	JSObject *obj;

	obj = JS_THIS_OBJECT(cx, vp);
	if (!obj) return JS_FALSE;
	p = JS_GetPrivate(cx, obj);

	if (argc > 2) {
		if (!JS_ConvertArguments(cx, argc, JS_ARGV(cx,vp), "s s s", &p->transport, &p->target, &p->topts))
			return JS_FALSE;
	}
	dprintf(dlevel,"transport: %s, target: %s, topts:%s\n", p->transport, p->target, p->topts);

	if (!p->transport) {
		JS_ReportError(cx, "can_connect: transport not set");
		return JS_FALSE;
	}

	return do_connect(cx,p);
}

static JSBool can_ctor(JSContext *cx, JSObject *obj, uintN argc, jsval *argv, jsval *rval) {
	can_private_t *p;
	JSObject *newobj;
	int r;

	p = calloc(sizeof(*p),1);
	if (!p) {
		JS_ReportError(cx,"error allocating memory");
		return JS_FALSE;
	}
	p->connected = &p->internal_connected;
	if (argc == 3) {
		if (!JS_ConvertArguments(cx, argc, argv, "s s s", &p->transport, &p->target, &p->topts))
			return JS_FALSE;
	} else if (argc > 1) {
		JS_ReportError(cx, "CAN requires 3 arguments (transport, target, topts)");
		return JS_FALSE;
	}

	dprintf(dlevel,"transport: %s, target: %s, topts:%s\n", p->transport, p->target, p->topts);

	r = do_connect(cx,p);

	newobj = js_InitCANClass(cx,JS_GetGlobalObject(cx));
	JS_SetPrivate(cx,newobj,p);
	*rval = OBJECT_TO_JSVAL(newobj);

	return r;
}

JSObject *jscan_new(JSContext *cx, void *tp, void *handle, char *transport, char *target, char *topts, int *con) {
	can_private_t *p;
	JSObject *newobj;

	p = calloc(sizeof(*p),1);
	if (!p) {
		JS_ReportError(cx,"jscan_new: error allocating memory");
		return 0;
	}
	p->tp = tp;
	p->tp_handle = handle;
	p->transport = transport;
	p->target = target;
	p->topts = topts;
	p->connected = con;

	newobj = js_InitCANClass(cx,JS_GetGlobalObject(cx));
	JS_SetPrivate(cx,newobj,p);
	return newobj;
}

JSObject *js_InitCANClass(JSContext *cx, JSObject *gobj) {
	JSPropertySpec can_props[] = { 
		{ "connected", CAN_PROPERTY_ID_CONNECTED, JSPROP_ENUMERATE | JSPROP_READONLY },
//		{ "autoconnect", CAN_PROPERTY_ID_AUTOCONNECT, JSPROP_ENUMERATE },
		{ "transport",CAN_PROPERTY_ID_TRANSPORT,JSPROP_ENUMERATE },
		{ "target",CAN_PROPERTY_ID_TARGET,JSPROP_ENUMERATE | JSPROP_READONLY  },
		{ "topts",CAN_PROPERTY_ID_TOPTS,JSPROP_ENUMERATE | JSPROP_READONLY  },
		{0}
	};
	JSFunctionSpec can_funcs[] = {
		JS_FN("connect",jscan_connect,3,3,0),
		JS_FN("read",jscan_read,1,1,0),
		JS_FN("write",jscan_write,2,2,0),
		{ 0 }
	};
	JSObject *obj;

	dprintf(5,"defining %s object\n",can_class.name);
	obj = JS_InitClass(cx, gobj, gobj, &can_class, can_ctor, 3, can_props, can_funcs, 0, 0);
	if (!obj) {
		JS_ReportError(cx,"unable to initialize can class");
		return 0;
	}
	dprintf(5,"done!\n");
	return obj;
}


#include <string.h>
#include "jsapi.h"
#include "jsatom.h"
#include "jsstr.h"
#include "jspubtd.h"
#include "jsobj.h"
#include "jsarray.h"

#define DEBUG_JSCONV 0
#define dlevel 6

#ifdef DEBUG
#undef DEBUG
#endif
#define DEBUG DEBUG_JSCONV
#include "debug.h"

#define TYPEOF(cx,v)    (JSVAL_IS_NULL(v) ? JSTYPE_NULL : JS_TypeOfValue(cx,v))

char *jstypestr(JSContext *cx, jsval vp) {
	return (char *)JS_GetTypeName(cx, JS_TypeOfValue(cx, vp));
}

jsval type_to_jsval(JSContext *cx, int type, void *dest, int len) {
	jsval val;

//	printf("type: %04x(%s)\n",type, typestr(type));
	dprintf(dlevel,"type: %d(%s), dest: %p, len: %d\n", type, typestr(type), dest, len);

	switch (type) {
	case DATA_TYPE_BYTE:
		val = INT_TO_JSVAL(*((uint8_t *)dest));
		break;
	case DATA_TYPE_SHORT:
		val = INT_TO_JSVAL(*((short *)dest));
		break;
	case DATA_TYPE_BOOL:
		val = BOOLEAN_TO_JSVAL(*((int *)dest));
		break;
	case DATA_TYPE_INT:
		val = INT_TO_JSVAL(*((int *)dest));
		break;
	case DATA_TYPE_FLOAT:
		JS_NewDoubleValue(cx, *((float *)dest), &val);
		break;
	case DATA_TYPE_DOUBLE:
		JS_NewNumberValue(cx, *((float *)dest), &val);
		break;
	case DATA_TYPE_STRING:
		val = STRING_TO_JSVAL(JS_NewStringCopyZ(cx,(char *)dest));
		break;
	case DATA_TYPE_S32_ARRAY:
	case DATA_TYPE_BOOL_ARRAY:
		{
			JSObject *arr;
			jsval element;
			int i,*ia;

			ia = (int *)dest;
			arr = JS_NewArrayObject(cx, len, NULL);
			for(i=0; i < len; i++) {
				JS_NewNumberValue(cx, ia[i], &element);
				JS_SetElement(cx, arr, i, &element);
			}
			val = OBJECT_TO_JSVAL(arr);
		}
		break;
	case DATA_TYPE_F32_ARRAY:
		{
			JSObject *arr;
			jsval element;
			float *fa;
			int i;

			fa = (float *)dest;
			arr = JS_NewArrayObject(cx, len, NULL);
			for(i=0; i < len; i++) {
//				dprintf(0,"adding[%d]: %f\n", i, fa[i]);
				JS_NewDoubleValue(cx, fa[i], &element);
				JS_SetElement(cx, arr, i, &element);
			}
			val = OBJECT_TO_JSVAL(arr);
		}
		break;
	case DATA_TYPE_STRING_ARRAY:
		{
			JSObject *arr;
			jsval element;
			char **sa;
			int i;

			sa = (char **)dest;
			arr = JS_NewArrayObject(cx, len, NULL);
			for(i=0; i < len; i++) {
				element = STRING_TO_JSVAL(JS_NewStringCopyZ(cx,sa[i]));
				JS_SetElement(cx, arr, i, &element);
			}
			val = OBJECT_TO_JSVAL(arr);
		}
		break;
	case DATA_TYPE_U8_ARRAY:
		{
			JSObject *arr;
			jsval element;
			uint8_t *ua;
			int i;

			ua = (uint8_t *)dest;
			arr = JS_NewArrayObject(cx, len, NULL);
			for(i=0; i < len; i++) {
				JS_NewNumberValue(cx, ua[i], &element);
				JS_SetElement(cx, arr, i, &element);
			}
			val = OBJECT_TO_JSVAL(arr);
		}
		break;
#if 0
	case JSTYPE_VOID:
		dprintf(dlevel,"void!\n");
		break;
	case JSTYPE_OBJECT:
		dprintf(dlevel,"obj!\n");
		break;
	case JSTYPE_FUNCTION:
		dprintf(dlevel,"func!\n");
		break;
	case JSTYPE_NUMBER:
		if (JSVAL_IS_INT(val)) {
			i = JSVAL_TO_INT(val);
			conv_type(dtype,dest,dlen,DATA_TYPE_INT,&i,0);
		} else if (JSVAL_IS_DOUBLE(val)) {
			d = *JSVAL_TO_DOUBLE(val);
			conv_type(dtype,dest,dlen,DATA_TYPE_DOUBLE,&d,0);
		} else {
			dprintf(dlevel,"unknown number type!\n");
		}
		break;
	case JSTYPE_BOOLEAN:
		i = JSVAL_TO_BOOLEAN(val);
		conv_type(dtype,dest,dlen,DATA_TYPE_BOOL,&i,0);
		break;
	case JSTYPE_NULL:
		str = (char *)JS_TYPE_STR(jstype);
		conv_type(dtype,dest,dlen,DATA_TYPE_STRING,str,strlen(str));
		break;
	case JSTYPE_XML:
		dprintf(dlevel,"xml!\n");
		break;
#endif
	default:
		val = JSVAL_NULL;
		printf("type_to_jsval: unhandled type: %04x(%s)\n",type,typestr(type));
		break;
	}
	return val;
}

int jsval_to_type(int dtype, void *dest, int dlen, JSContext *cx, jsval val) {
	int jstype;
	int i,r;
	char *str;
	double d;

	jstype = TYPEOF(cx,val);
	dprintf(1,"jstype: %d(%s), dtype: %d(%s), dest: %p, dlen: %d\n", jstype, jstypestr(cx,val), dtype, typestr(dtype), dest, dlen);
	r = 0;
	switch (jstype) {
	case JSTYPE_VOID:
		dprintf(dlevel,"void!\n");
		break;
	case JSTYPE_OBJECT:
		{
			JSObject *obj = JSVAL_TO_OBJECT(val);
#if DEBUG
			JSClass *classp = OBJ_GET_CLASS(cx, obj);
			dprintf(1,"class: %s\n", classp->name);
#endif
			if (OBJ_IS_DENSE_ARRAY(cx,obj)) {
				unsigned int count;
				jsval element;
				JSString *str;
				char **values;
				int size;

				if (!js_GetLengthProperty(cx, obj, &count)) {
					JS_ReportError(cx,"unable to get array length");
					return 0;
				}
				/* Just turn everything into a DATA_TYPE_STRING_ARRAY */
				size = count*sizeof(char *);
				values = calloc(size,1);
				if (!values) {
					log_syserror("jsval_to_type: calloc(%d,1)",size);
					return 0;
				}
				for(i=0; i < count; i++) {
					JS_GetElement(cx, obj, (jsint)i, &element);
					str = JS_ValueToString(cx,element);
					if (!str) continue;
					values[i] = JS_EncodeString(cx, str);
					if (!values[i]) continue;
					dprintf(1,"====> element[%d]: %s\n", i, values[i]);
				}
				/* Convert it */
				r = conv_type(dtype,dest,dlen,DATA_TYPE_STRING_ARRAY,values,count);
				/* Got back and free the values */
				for(i=0; i < count; i++) JS_free(cx, values[i]);
				free(values);
			} else {
				log_error("jsval_to_type: object is not an array\n");
				return 0;
			}
		}
		break;
	case JSTYPE_FUNCTION:
		dprintf(dlevel,"func!\n");
		break;
	case JSTYPE_STRING:
		str = (char *)js_GetStringBytes(cx, JSVAL_TO_STRING(val));
		r = conv_type(dtype,dest,dlen,DATA_TYPE_STRING,str,strlen(str));
		break;
	case JSTYPE_NUMBER:
		dprintf(dlevel,"IS_INT: %d, IS_DOUBLE: %d\n", JSVAL_IS_INT(val), JSVAL_IS_DOUBLE(val));
		if (JSVAL_IS_INT(val)) {
			i = JSVAL_TO_INT(val);
			dprintf(dlevel,"i: %d\n", i);
			r = conv_type(dtype,dest,dlen,DATA_TYPE_INT,&i,0);
		} else if (JSVAL_IS_DOUBLE(val)) {
			d = *JSVAL_TO_DOUBLE(val);
			dprintf(dlevel,"d: %f\n", d);
			r = conv_type(dtype,dest,dlen,DATA_TYPE_DOUBLE,&d,0);
		} else {
			dprintf(dlevel,"unknown number type!\n");
		}
		break;
	case JSTYPE_BOOLEAN:
		i = JSVAL_TO_BOOLEAN(val);
		r = conv_type(dtype,dest,dlen,DATA_TYPE_BOOL,&i,0);
		break;
	case JSTYPE_NULL:
		str = (char *)JS_TYPE_STR(jstype);
		r = conv_type(dtype,dest,dlen,DATA_TYPE_STRING,str,strlen(str));
		break;
	case JSTYPE_XML:
		dprintf(dlevel,"xml!\n");
		break;
	default:
		printf("jsval_to_type: unknown JS type: %d\n",jstype);
		break;
	}
	return r;
}

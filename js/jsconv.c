
#include <string.h>
#include "jsapi.h"
#include "jsatom.h"
#include "jsstr.h"
#include "jspubtd.h"
#include "jsobj.h"
#include "jsarray.h"

#define DEBUG_JSCONV 1
#define dlevel 6

#ifdef DEBUG
#undef DEBUG
#endif
#define DEBUG DEBUG_JSCONV
#include "debug.h"

#define TYPEOF(cx,v)    (JSVAL_IS_NULL(v) ? JSTYPE_NULL : JS_TypeOfValue(cx,v))

char *jstypestr(JSContext *cx, jsval val) {
//	printf("val: %d\n", val);
	if (!val) return "null";
	return (char *)JS_GetTypeName(cx, JS_TypeOfValue(cx, val));
}

#if 0
char *jstypestr(JSContext *cx, jsval v) {
	if (JSVAL_IS_NULL(v)) return "null";
	else if (JSVAL_IS_VOID(v)) return "void";
	else if (JSVAL_IS_OBJECT(v)) return "object";
	else if (JSVAL_IS_INT(v)) return "int";
	else if (JSVAL_IS_DOUBLE(v)) return "double";
	else if (JSVAL_IS_STRING(v)) return "string";
	else if (JSVAL_IS_BOOLEAN(v)) return "bool";
	else return "unknown";
}
#endif

jsval type_to_jsval(JSContext *cx, int type, void *src, int len) {
	jsval val;

//	printf("type_to_jsval: type: %04x(%s)\n",type, typestr(type));
	dprintf(dlevel,"type: %d(%s), src: %p, len: %d\n", type, typestr(type), src, len);
	if (!src) return JSVAL_NULL;

	switch (type) {
	case DATA_TYPE_NULL:
		val = JSVAL_NULL;
		break;
	case DATA_TYPE_BYTE:
		val = INT_TO_JSVAL(*((uint8_t *)src));
		break;
	case DATA_TYPE_SHORT:
		val = INT_TO_JSVAL(*((short *)src));
		break;
	case DATA_TYPE_BOOL:
		val = BOOLEAN_TO_JSVAL(*((int *)src));
		break;
	case DATA_TYPE_INT:
		val = INT_TO_JSVAL(*((int *)src));
		break;
	case DATA_TYPE_U32:
		val = INT_TO_JSVAL(*((uint32_t *)src));
		break;
	case DATA_TYPE_FLOAT:
		JS_NewDoubleValue(cx, *((float *)src), &val);
		break;
	case DATA_TYPE_DOUBLE:
//		printf("f64: %lf\n",*((double *)src));
		JS_NewNumberValue(cx, *((double *)src), &val);
		break;
	case DATA_TYPE_STRING:
		val = STRING_TO_JSVAL(JS_NewStringCopyZ(cx,(char *)src));
		break;
	case DATA_TYPE_S32_ARRAY:
	case DATA_TYPE_BOOL_ARRAY:
		{
			JSObject *arr;
			jsval element;
			int i,*ia;

			ia = (int *)src;
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

			fa = (float *)src;
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

			sa = (char **)src;
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

			ua = (uint8_t *)src;
			arr = JS_NewArrayObject(cx, len, NULL);
			for(i=0; i < len; i++) {
				JS_NewNumberValue(cx, ua[i], &element);
				JS_SetElement(cx, arr, i, &element);
			}
			val = OBJECT_TO_JSVAL(arr);
		}
		break;
	case DATA_TYPE_STRING_LIST:
		{
			list l = (list) src;
			JSObject *arr;
			jsval element;
			int i,count;
			char *p;

			count = list_count(l);
			dprintf(dlevel,"count: %d\n", count);
			arr = JS_NewArrayObject(cx, count, NULL);
			i = 0;
			list_reset(l);
			while((p = list_get_next(l)) != 0) {
				dprintf(dlevel,"adding string: %s\n", p);
				element = STRING_TO_JSVAL(JS_NewStringCopyZ(cx,p));
				JS_SetElement(cx, arr, i++, &element);
			}
			val = OBJECT_TO_JSVAL(arr);
		}
		break;
	default:
		val = JSVAL_NULL;
		log_error("type_to_jsval: unhandled type: %04x(%s)\n",type,typestr(type));
		break;
	}
	dprintf(dlevel,"returning type: %s\n",jstypestr(cx,val));
	return val;
}

int jsval_to_type(int dtype, void *dest, int dlen, JSContext *cx, jsval val) {
	int jstype;
	int i,r;
	char *str;
	double d;

	jstype = TYPEOF(cx,val);
	dprintf(dlevel,"jstype: %d(%s), dtype: %d(%s), dest: %p, dlen: %d\n", jstype, jstypestr(cx,val), dtype, typestr(dtype), dest, dlen);
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
//					dprintf(1,"====> element[%d]: %s\n", i, values[i]);
				}
				/* Convert it */
				r = conv_type(dtype,dest,dlen,DATA_TYPE_STRING_ARRAY,values,count);
				dprintf(dlevel,"r: %d\n", r);
				/* Got back and free the values */
				for(i=0; i < count; i++) JS_free(cx, values[i]);
				free(values);
				dprintf(dlevel,"dest: %p\n", dest);
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

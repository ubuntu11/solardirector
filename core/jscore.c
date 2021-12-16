
#include "common.h"
#include "config.h"
#include "jsapi.h"
#include "jsatom.h"
#include "jsstr.h"
#include "jspubtd.h"

#define dlevel 6

#define TYPEOF(cx,v)    (JSVAL_IS_NULL(v) ? JSTYPE_NULL : JS_TypeOfValue(cx,v))

char *jstypestr(JSContext *cx, jsval vp) {
	return (char *)JS_GetTypeName(cx, JS_TypeOfValue(cx, vp));
}

jsval typetojsval(JSContext *cx, enum DATA_TYPE type, void *dest, int len) {
	jsval val;

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
	case DATA_TYPE_LONG:
		val = INT_TO_JSVAL(*((long *)dest));
		break;
	case DATA_TYPE_FLOAT:
		JS_NewDoubleValue(cx, *((float *)dest), &val);
		break;
	case DATA_TYPE_STRING:
		val = STRING_TO_JSVAL(JS_NewStringCopyZ(cx,(char *)dest));
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
		dprintf(dlevel,"unhandled type!\n");
		break;
	}
	return val;
}

void jsvaltotype(enum DATA_TYPE dtype, void *dest, int dlen, JSContext *cx, jsval val) {
	int jstype;
	int i;
	char *str;
	double d;

	jstype = TYPEOF(cx,val);
	dprintf(dlevel,"jstype: %d(%s), dtype: %d(%s), dest: %p, dlen: %d\n", jstype, jstypestr(cx,val), dtype, typestr(dtype), dest, dlen);

	switch (jstype) {
	case JSTYPE_VOID:
		dprintf(dlevel,"void!\n");
		break;
	case JSTYPE_OBJECT:
		dprintf(dlevel,"obj!\n");
		break;
	case JSTYPE_FUNCTION:
		dprintf(dlevel,"func!\n");
		break;
	case JSTYPE_STRING:
		str = (char *)js_GetStringBytes(cx, JSVAL_TO_STRING(val));
		conv_type(dtype,dest,dlen,DATA_TYPE_STRING,str,strlen(str));
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
	default:
		dprintf(dlevel,"unknown type: %d\n",jstype);
		break;
	}
}

JSPropertySpec *configtoprops(config_t *cp, char *name, JSPropertySpec *add) {
	JSPropertySpec *props,*pp,*app;
	config_section_t *section;
	config_property_t *p;
	int count;

	section = config_get_section(cp,name);
	if (!section) {
		sprintf(cp->errmsg,"unable to find config section: %s\n", name);
		return 0;
	}

	/* Get total count */
	count = list_count(section->items);
	if (add) for(app = add; app->name; app++) count++;
	/* Add 1 for termination */
	count++;
	dprintf(dlevel,"count: %d\n", count);
	props = calloc(count * sizeof(JSPropertySpec),1);
	if (!props) {
		sprintf(cp->errmsg,"calloc props(%d): %s\n", count, strerror(errno));
		return 0;
	}

	dprintf(dlevel,"adding config...\n");
	pp = props;
	list_reset(section->items);
	while((p = list_get_next(section->items)) != 0) {
		if (p->flags & CONFIG_FLAG_FILEONLY) continue;
		pp->name = p->name;
		pp->tinyid = p->id;
		pp->flags = JSPROP_ENUMERATE;
		if (p->flags & CONFIG_FLAG_READONLY) pp->flags |= JSPROP_READONLY;
		pp++;
	}

	/* Now add the addl props, if any */
	if (add) for(app = add; app->name; app++) *pp++ = *app;

	dprintf(dlevel+1,"dump:\n");
	for(pp = props; pp->name; pp++) 
		dprintf(dlevel+1,"prop: name: %s, id: %d, flags: %02x\n", pp->name, pp->tinyid, pp->flags);

	config_build_propmap(cp);

	return props;
}

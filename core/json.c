
/*
Copyright (c) 2021, Stephen P. Shoecraft
All rights reserved.

This source code is licensed under the BSD-style license found in the
LICENSE file in the root directory of this source tree.
*/

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "json.h"
#include "utils.h"

#define DEBUG_JSON 1
#define dlevel 6

#ifdef DEBUG
#undef DEBUG
#endif
#define DEBUG DEBUG_JSON
#include "debug.h"

#ifdef DEBUG_MEM
static int _init = 0;
#endif

#include "parson.c"


#define VAL(v) ((JSON_Value *)(v))
#define OBJ(o) ((JSON_Object *)(o))
#define OBJV(o) parson_object_get_wrapping_value(OBJ(o))
#define ARR(a) ((JSON_Array *)(a))
#define ARRV(a) parson_array_get_wrapping_value(ARR(a))

char *json_typestr(int type) {
	char *typenames[] = { "None","Null","String","Number","Object","Array","Boolean" };
	if (type < JSONNull || type > JSONBoolean) return "unknown";
	return typenames[type];
}
json_value_t *json_parse(char *str) {
	return (json_value_t *)parson_parse_string(str);
}

/* Values */
enum JSON_TYPE json_value_get_type(json_value_t *v) { return(VAL(v)->type); }
char *json_value_get_string(json_value_t *v) { return (char *)parson_string(VAL(v)); }
double json_value_get_number(json_value_t *v) { return parson_number(VAL(v)); }
json_object_t *json_value_get_object(json_value_t *v) { return (json_object_t *)parson_object(VAL(v)); }
json_array_t *json_value_get_array(json_value_t *v) { return (json_array_t *) parson_array(VAL(v)); }
int json_value_get_boolean(json_value_t *v) { return parson_boolean(VAL(v)); }
int json_destroy_value(json_value_t *v);

/* Objects */
json_object_t *json_create_object(void);
json_value_t *json_object_get_value(json_object_t *o) { return (json_value_t *)OBJV(o); }
char *json_object_get_string(json_object_t *o,char *name) { return (char *)parson_object_get_string(OBJ(o),name); }
int json_object_get_boolean(json_object_t *,char *,int);
double json_object_get_number(json_object_t *o,char *n) { return parson_object_get_number(OBJ(o),n); }
json_object_t *json_object_get_object(json_object_t *,char *);
json_array_t *json_object_get_array(json_object_t *o,char *n) { return (json_array_t *)parson_object_get_array(OBJ(o),n); }
char *json_object_dotget_string(json_object_t *o,char *n) { return (char *)parson_object_dotget_string(OBJ(o),n); }
int json_object_dotget_boolean(json_object_t *o,char *n) { return parson_object_dotget_boolean(OBJ(o),n); }
double json_object_dotget_number(json_object_t *o,char *n) { return parson_object_dotget_number(OBJ(o),n); }
json_object_t *json_object_dotget_object(json_object_t *o,char *n) {
	return (json_object_t *)parson_object_dotget_object(OBJ(o), n);
}
json_array_t *json_object_dotget_array(json_object_t *o,char *n) {
	return (json_array_t *)parson_object_dotget_array(OBJ(o), n);
}
json_value_t *json_object_dotget_value(json_object_t *, char *);
int json_object_delete_value(json_object_t *o, char *name) {
	return parson_object_remove_internal(OBJ(o), name, 1);
}
int json_object_set_string(json_object_t *,char *,char *);
int json_object_set_boolean(json_object_t *,char *,int);
int json_object_set_number(json_object_t *,char *,double);
int json_object_set_object(json_object_t *o,char *name,json_object_t *no) {
	return parson_object_set_value(OBJ(o),name,OBJV(no));
}
int json_object_add_array(json_object_t *o,char *n,json_array_t *a) {
	printf("===> adding array...\n");
	return parson_object_add(OBJ(o),n,ARRV(a));
}
int json_object_add_value(json_object_t *o,char *n,json_value_t *v) {
	return parson_object_add(OBJ(o),n,VAL(v));
}
int json_object_set_array(json_object_t *,char *,json_array_t *);
int json_object_set_value(json_object_t *,char *,json_value_t *);
int json_destroy_object(json_object_t *);
int json_object_dotset_value(json_object_t *o, char *label, json_value_t *v) {
	return parson_object_dotset_value((JSON_Object *)o, label, (JSON_Value *)v);
}

/* Arrays */
json_array_t *json_create_array(void);
json_value_t *json_array_get_value(json_array_t *a) { return (json_value_t *)ARRV(a); }
int json_array_add_string(json_array_t *,char *);
int json_array_add_strings(json_array_t *,char *);
int json_array_add_boolean(json_array_t *,int);
int json_array_add_number(json_array_t *,double);
int json_array_add_object(json_array_t *,json_object_t *);
int json_array_add_array(json_array_t *,json_array_t *);
int json_array_add_value(json_array_t *,json_value_t *);
int json_destroy_array(json_array_t *);

json_object_t *json_create_object(void) {
#if defined(DEBUG_MEM) && !defined(__WIN64)
	if (!_init) {
		parson_set_allocation_functions(mem_malloc,mem_free);
		_init = 1;
	}
#endif
	return (json_object_t *)parson_object(parson_value_init_object());
}

json_array_t *json_create_array(void) {
	return (json_array_t *)parson_array(parson_value_init_array());
}

int json_destroy_value(json_value_t *v) {
	parson_value_free((JSON_Value *)v);
	return 0;
}

int json_destroy_object(json_object_t *o) {
	parson_value_free(parson_object_get_wrapping_value((JSON_Object *)o));
	return 0;
}

int json_destroy_array(json_array_t *a) {
	parson_value_free(parson_array_get_wrapping_value((JSON_Array *)a));
	return 0;
}


#if 0
int json_add_string(json_value_t *o, char *name, char *value) {
	dprintf(5,"object: %p, name: %s, value: %s\n", o, name, value);
	return json_object_set_string(json_object(o), name, value);
}

int json_add_number(json_value_t *o, char *name, double value) {
	dprintf(5,"object: %p, name: %s, value: %f\n", o, name, value);
	return json_object_set_number(json_object(o), name, value);
}

int json_add_boolean(json_value_t *o, char *name, int value) {
	dprintf(5,"object: %p, name: %s, value: %d\n", o, name, value);
	return json_object_set_boolean(json_object(o), name, value);
}

int json_add_value(json_value_t *v, char *name, json_value_t *value) {
//	XXX json_object_set_value does not copy passed value so it shouldn't be freed afterwards.
	dprintf(5,"object: %p, name: %s, value: %p\n", v, name, value);
	return json_object_set_value(json_object(v), name, value);
}

int json_add_array(json_value_t *v, char *name, json_value_t *array) {
	return json_object_set_value(json_object(v), name, array);
}

int json_add_list(json_object_t *j, char *label, list values) {
	JSON_Value *value;
	JSON_Array *array;
	char *p;

	value = json_value_init_array();
	array = json_array(value);
	list_reset(values);
	while((p = list_get_next(values)) != 0) {
		dprintf(5,"adding: %s\n", p);
		parson_array_append_string(array,p);
	}
	return 0;
}

#endif

/*****************************************************/
/* json_object */
int json_object_set_string(json_object_t *o, char *name, char *value) {
	dprintf(5,"object: %p, name: %s, value: %s\n", o, name, value);
	return parson_object_set_string(OBJ(o), name, value);
}

int json_object_set_number(json_object_t *o, char *name, double value) {
	dprintf(5,"object: %p, name: %s, value: %f\n", o, name, value);
	return parson_object_set_number(OBJ(o), name, value);
}

int json_object_set_boolean(json_object_t *o, char *name, int value) {
	dprintf(5,"object: %p, name: %s, value: %d\n", o, name, value);
	return parson_object_set_boolean(OBJ(o), name, value);
}

int json_object_set_value(json_object_t *o, char *name, json_value_t *value) {
//	XXX json_object_set_value does not copy passed value so it shouldn't be freed afterwards.
	dprintf(5,"object: %p, name: %s, value: %p\n", o, name, value);
	return parson_object_set_value(OBJ(o), name, VAL(value));
}

int json_object_set_array(json_object_t *o, char *name, json_array_t *a) {
	return parson_object_set_value(OBJ(o), name, ARRV(a));
}

/*****************************************************/
/* json_array */
int json_array_add_string(json_array_t *a,char *value) {
	return parson_array_append_string(ARR(a),value);
}

int json_array_add_strings(json_array_t *a,char *value) {
	register char *p;
	char temp[128];
	int i;

	i = 0;
	for(p = value; *p; p++) {
		if (*p == ',') {
			dprintf(5,"temp: %s\n", temp);
			parson_array_append_string(ARR(a),temp);
			i = 0;
		} else
			temp[i++] = *p;
	}
	if (i > 0) {
		temp[i] = 0;
		dprintf(5,"temp: %s\n", temp);
		parson_array_append_string(ARR(a),temp);
	}
	return 0;
}

int json_array_add_number(json_array_t *a,double value) {
	return parson_array_append_number(ARR(a),value);
}

int json_array_add_value(json_array_t *a,json_value_t *v) {
	return parson_array_append_value(ARR(a),VAL(v));
}

int json_array_add_object(json_array_t *a,json_object_t *o) {
	return parson_array_append_value(ARR(a),OBJV(o));
}

/*****************************************************/
int json_tostring(json_value_t *j, char *dest, int dest_len, int pretty) {
//	char *p;

	return pretty ? parson_serialize_to_buffer_pretty(VAL(j),dest,dest_len) : parson_serialize_to_buffer(VAL(j),dest,dest_len);
}

char *json_dumps(json_value_t *o, int pretty) {
	return (pretty ? parson_serialize_to_string_pretty(VAL(o)) : parson_serialize_to_string(VAL(o)));
}

int json_dumps_r(json_value_t *v, char *buf, int buflen) {
	return parson_serialize_to_buffer(VAL(v),buf,buflen);
}

json_value_t *json_from_tab(json_proctab_t *tab) {
	json_proctab_t *p;
	json_object_t *o;
	int *ip;
	float *fp;
	double *dp;

	o = json_create_object();
	if (!o) return 0;
	for(p=tab; p->field; p++) {
		dprintf(dlevel,"p: field: %s, ptr: %p, len: %d, cb: %p\n", p->field, p->ptr, p->len, p->cb);
		if (p->cb) p->cb(p->field,p->ptr,p->len,json_object_get_value(o));
		else {
			dprintf(dlevel,"p->type: %d\n", p->type);
			switch(p->type) {
			case DATA_TYPE_STRING:
				json_object_set_string(o,p->field,p->ptr);
				break;
			case DATA_TYPE_INT:
				ip = p->ptr;
				json_object_set_number(o,p->field,*ip);
				break;
			case DATA_TYPE_FLOAT:
				fp = p->ptr;
				json_object_set_number(o,p->field,*fp);
				break;
			case DATA_TYPE_DOUBLE:
				dp = p->ptr;
				json_object_set_number(o,p->field,*dp);
				break;
			default:
				dprintf(dlevel,"json_from_type: unhandled type: %d\n", p->type);
				break;
			}
		}
	}
	return json_object_get_value(o);
}

json_value_t *json_from_cfgtab(cfg_proctab_t *ct) {
	json_value_t *v;
	cfg_proctab_t *ctp;
	json_proctab_t *jt;
	int i,count;

	count = 0;
	for(ctp = ct; ctp->keyword; ctp++) count++;
	jt = malloc(sizeof(*jt)*count);
	memset(jt,0,sizeof(*jt)*count);
	i = 0;
	for(ctp = ct; ctp->keyword; ctp++) {
		jt[i].field = ctp->keyword;
		jt[i].type = ctp->type;
		jt[i].ptr = ctp->dest;
		jt[i].len = ctp->dlen;
		i++;
	}
	v = json_from_tab(jt);
	free(jt);
	return v;
}


int json_to_tab(json_proctab_t *tab, json_value_t *v) {
	json_proctab_t *p;
	json_object_t *o;
	char *s;
	double d;
	int i,n;

	/* must be object */
	dprintf(dlevel,"v: %p\n", v);
	if (!v) return 1;
	if (json_value_get_type(v) != JSON_TYPE_OBJECT) return 1;

	o = json_value_get_object(v);
        for (i = 0; i < o->count; i++) {
		dprintf(5,"name: %s\n", o->names[i]);
		for(p=tab; p->field; p++) {
			if (strcmp(p->field,o->names[i])==0) {
				if (p->cb) p->cb(p->field,p->ptr,p->len,o->values[i]);
				else {
					json_value_t *vv = o->values[i];
					switch(json_value_get_type(vv)) {
					case JSON_TYPE_STRING:
						s = json_value_get_string(vv);
						conv_type(p->type,p->ptr,p->len,DATA_TYPE_STRING,s,strlen(s));
						break;
					case JSON_TYPE_NUMBER:
						d =  json_value_get_number(vv);
						conv_type(p->type,p->ptr,p->len,DATA_TYPE_DOUBLE,&d,0);
						break;
					case JSON_TYPE_BOOLEAN:
						n = json_value_get_boolean(vv);
						conv_type(p->type,p->ptr,p->len,DATA_TYPE_LOGICAL,&n,0);
						break;
					default:
						break;
					}
				}
				break;
			}
		}
        }
	return 0;
}

json_value_t *json_from_type(int type, void *src, int len) {
	double d;
	json_value_t *v;

	v = 0;
	dprintf(dlevel,"type: %d(%s)\n", type, typestr(type));
	if (type == DATA_TYPE_BOOLEAN) {
		v = (json_value_t *)parson_value_init_boolean(*((int *)src));
	} else if (DATA_TYPE_ISARRAY(type)) {
		printf("dont handle arrays atm\n");
	} else if (DATA_TYPE_ISNUMBER(type)) {
		conv_type(DATA_TYPE_DOUBLE,&d,0,type,src,len);
		dprintf(dlevel,"d: %f\n", d);
		v = (json_value_t *)parson_value_init_number(d);
	} else if (DATA_TYPE_ISLIST(type)) {
		printf("dont handle lists atm\n");
	} else if (type == DATA_TYPE_STRING) {
		v = (json_value_t *)parson_value_init_string(src);
	} else  {
		printf("dont handle %s atm\n",typestr(type));
	}
	return v;
}

int json_to_type(int dt, void *dest, int len, json_value_t *v) {
	int i;
	double d;
	char *s;

	dprintf(dlevel,"dt: %d(%s), json_type: %s\n", dt, typestr(dt), json_typestr(VAL(v)->type));
	switch(VAL(v)->type) {
	case JSONString:
		s = (char *)parson_string(VAL(v));
		return conv_type(dt,dest,len,DATA_TYPE_STRING,s,strlen(s));
		break;
	case JSONNumber:
		d = parson_number(VAL(v));
		return conv_type(dt,dest,len,DATA_TYPE_DOUBLE,&d,0);
		break;
	case JSONBoolean:
		i = parson_boolean(VAL(v));
		return conv_type(dt,dest,len,DATA_TYPE_BOOL,&i,0);
		break;
	case JSONObject:
		{
			JSON_Object *o;
			int i;

			o = parson_object(VAL(v));
			for (i = 0; i < o->count; i++) {
				dprintf(5,"name: %s\n", o->names[i]);
			}
			return 1;
		}
		break;
	default:
		dprintf(dlevel,"invalid type: %d (%s)\n", VAL(v)->type, json_typestr(VAL(v)->type));
		return -1;
		break;
	}
}

int json_iter(char *name, json_value_t *v, json_ifunc_t *func, void *ctx) {
	JSON_Object *o;
	JSON_Array *a;
        int i;

        dprintf(dlevel,"type: %d (%s)\n", VAL(v)->type, json_typestr(VAL(v)->type));

        switch(VAL(v)->type) {
        case JSONObject:
                o = parson_object(VAL(v));
                dprintf(dlevel,"count: %d\n", o->count);
                for(i=0; i < o->count; i++) if (json_iter(o->names[i],(json_value_t *)o->values[i],func,ctx)) return 1;
                break;
        case JSONArray:
                a = parson_array(VAL(v));
                dprintf(dlevel,"count: %d\n", a->count);
                for(i=0; i < a->count; i++) if (json_iter(0,(json_value_t *)a->items[i],func,ctx)) return 1;
                break;
	default:
		{
			char value[256];
			json_to_type(DATA_TYPE_STRING,value,sizeof(value)-1,v);
			if (name) return func(ctx,name,value);
			else return func(ctx,value,0);
		}
		break;
        }
        return 0;
}


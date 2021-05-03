
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
#include "parson.h"
#include "utils.h"
#include "debug.h"

#ifdef DEBUG_MEM
static int _init = 0;
#endif

char *json_typestr(int type) {
	char *typenames[] = { "None","Null","String","Number","Object","Array","Boolean" };
	if (type < JSONNull || type > JSONBoolean) return "unknown";
	return typenames[type];
}

json_value_t *json_create_object(void) {
#ifdef DEBUG_MEM
	if (!_init) {
		json_set_allocation_functions(mem_malloc,mem_free);
		_init = 1;
	}
#endif
	return json_value_init_object();
}

json_value_t *json_create_array(void) {
	return json_value_init_array();
}

int json_destroy(json_value_t *v) {
	json_value_free(v);
	return 0;
}

json_value_t *json_parse(char *str) {
	return json_parse_string(str);
}

void json_object_set_scope(JSON_Object *o, int type, char *scope, int nvalues, void *values) {
	int i,*ip;
	float *fp;

	json_object_set_string(o,"scope",scope);
	switch(type) {
	case DATA_TYPE_FLOAT:
		fp = (float *) values;
		if (strcmp(scope,"range")==0) {
			JSON_Value *sv;

			sv = json_value_init_object();
			if (nvalues) json_object_set_number(json_object(sv),"min",fp[0]);
			if (nvalues > 1) json_object_set_number(json_object(sv),"max",fp[1]);
			if (nvalues > 2) json_object_set_number(json_object(sv),"step",fp[2]);
			json_object_set_value(o,"values",sv);
		} else if (strcmp(scope,"select")==0 || strcmp(scope,"mask")==0) {
			JSON_Value *av;

			av = json_value_init_array();
			for(i=0; i < nvalues; i++) json_array_append_number(json_array(av),fp[i]);
			json_object_set_value(o,"values",av);
		}
		break;
	case DATA_TYPE_INT:
	case DATA_TYPE_STRING:
		ip = (int *) values;
		if (strcmp(scope,"range")==0) {
			JSON_Value *sv;

			sv = json_value_init_object();
			if (nvalues) json_object_set_number(json_object(sv),"min",ip[0]);
			if (nvalues > 1) json_object_set_number(json_object(sv),"max",ip[1]);
			if (nvalues > 2 && type == DATA_TYPE_INT) json_object_set_number(json_object(sv),"step",ip[2]);
			json_object_set_value(o,"values",sv);
		} else if (strcmp(scope,"select")==0 || strcmp(scope,"mask")==0) {
			JSON_Value *av;

			av = json_value_init_array();
			for(i=0; i < nvalues; i++) json_array_append_number(json_array(av),ip[i]);
			json_object_set_value(o,"values",av);
		}
		break;
	}
}

json_object_t *json_create_descriptor(json_descriptor_t d) {
	JSON_Value *v,*tv;
	JSON_Object *o,*to;
	int i;

	dprintf(5,"name: %s, type: %d, scope: %s, nvalues: %d, values: %p, nlabels: %d, labels: %p\n", d.name, d.type, d.scope, d.nvalues, d.values, d.nlabels, d.labels);

	v = json_value_init_object();
	o = json_object(v);
	tv = json_value_init_object();
	to = json_object(tv);

	switch(d.type) {
	case DATA_TYPE_BOOL:
		json_object_set_string(to,"type","bool");
		break;
	case DATA_TYPE_BYTE:
		json_object_set_string(to,"type","u8");
		break;
	case DATA_TYPE_SHORT:
		json_object_set_string(to,"type","i16");
		break;
	case DATA_TYPE_INT:
		json_object_set_string(to,"type","i32");
		break;
	case DATA_TYPE_FLOAT:
		json_object_set_string(to,"type","f32");
		break;
	case DATA_TYPE_DOUBLE:
		json_object_set_string(to,"type","f64");
		break;
	case DATA_TYPE_STRING:
		json_object_set_string(to,"type","string");
		break;
	case DATA_TYPE_DATE:
		json_object_set_string(to,"type","date");
		break;
	default:
		json_object_set_string(to,"type","unknown");
		break;
	}
	if (d.nvalues && d.values) json_object_set_scope(to,d.type,d.scope,d.nvalues,d.values);
	if (d.nlabels && d.labels) {
		JSON_Value *av;

		if (d.nlabels > 1) {
			av = json_value_init_array();
			for(i=0; i < d.nlabels; i++) json_array_append_string(json_array(av),d.labels[i]);
			json_object_set_value(to,"labels",av);
		} else {
			json_object_set_string((JSON_Object *)to,"label",d.labels[0]);
		}
	}
	if (d.units) json_object_set_string(to,"units",d.units);
	if (d.scale > 0.0 || d.scale < 0.0 ) json_object_set_number(to,"scale",d.scale);
	json_object_set_value(o,d.name,tv);
	return (json_object_t *)v;
}

int json_add_string(json_value_t *o, char *name, char *value) {
	dprintf(5,"object: %p, name: %s, value: %s\n", o, name, value);
	return json_object_set_string(json_object(o), name, value);
}

int json_add_number(json_value_t *o, char *name, double value) {
	dprintf(5,"object: %p, name: %s, value: %f\n", o, name, value);
	return json_object_set_number(json_object(o), name, value);
}

int json_add_value(json_value_t *v, char *name, json_value_t *value) {
//	XXX json_object_set_value does not copy passed value so it shouldn't be freed afterwards.
	dprintf(5,"object: %p, name: %s, value: %p\n", v, name, value);
	return json_object_set_value(json_object(v), name, value);
}

int json_add_descriptor(json_value_t *o,char *name,json_descriptor_t d) {
	JSON_Value *v;

	v = (JSON_Value *) json_create_descriptor(d);
	dprintf(5,"o: %p, name: %s, v: %p\n", o, name, v);
	return json_object_set_value(json_object(o),name,v);
}

char *json_get_string(json_value_t *v, char *name) {
	return (char *)json_object_get_string(json_object(v),name);
}

/*****************************************************/

int json_array_add_string(json_value_t *a,char *value) {
	return json_array_append_string(json_array(a),value);
}

int json_array_add_strings(json_value_t *a,char *value) {
	register char *p;
	char temp[128];
	int i;

	i = 0;
	for(p = value; *p; p++) {
		if (*p == ',') {
			dprintf(5,"temp: %s\n", temp);
			json_array_append_string(json_array(a),temp);
			i = 0;
		} else
			temp[i++] = *p;
	}
	if (i > 0) {
		temp[i] = 0;
		dprintf(5,"temp: %s\n", temp);
		json_array_append_string(json_array(a),temp);
	}
	return 0;
}

int json_array_add_number(json_value_t *a,double value) {
	return json_array_append_number(json_array(a),value);
}

int json_array_add_value(json_value_t *a,json_value_t *o) {
	return json_array_append_value(json_array(a),o);
}

int json_array_add_descriptor(json_value_t *a,json_descriptor_t d) {
	JSON_Value *v;

	v = (JSON_Value *) json_create_descriptor(d);
	dprintf(5,"a: %p, v: %p\n", a, v);
	return json_array_append_value(json_array(a),v);
}

int json_tostring(json_value_t *j, char *dest, int dest_len, int pretty) {
	char *p;
	p = pretty ? json_serialize_to_string_pretty(j) : json_serialize_to_string((JSON_Value *)j);
	dprintf(5,"p: %s\n", p);
	dprintf(5,"dest_len: %d\n", dest_len);
	dest[0] = 0;
	strncat(dest,p,dest_len);
	dprintf(5,"dest: %s\n", dest);
	json_free_serialized_string(p);
	return 0;
}

char *json_dumps(json_value_t *o, int pretty) {
	return (pretty ? json_serialize_to_string_pretty(o) : json_serialize_to_string(o));
}

int json_dumps_r(json_value_t *v, char *buf, int buflen) {
	return json_serialize_to_buffer(v,buf,buflen);
}

int json_add_list(json_object_t *j, char *label, list values) {
	JSON_Value *value;
	JSON_Array *array;
	char *p;

//	JSON_Array  *json_array  (const JSON_Value *value);
	value = json_value_init_array();
	array = json_array(value);
//	JSON_Status json_array_append_string(JSON_Array *array, const char *string);
	list_reset(values);
	while((p = list_get_next(values)) != 0) {
		dprintf(5,"adding: %s\n", p);
		json_array_append_string(array,p);
	}
//	return json_object_dotset_value(j->root_object, label, json_parse_string(temp));
	return 0;
}

json_value_t *json_from_tab(json_proctab_t *tab) {
	json_proctab_t *p;
	json_value_t *v;
	int *ip;
	float *fp;
	double *dp;

	v = json_create_object();
	if (!v) return 0;
	for(p=tab; p->field; p++) {
		if (p->cb) p->cb(p->field,p->ptr,p->len,v);
		else {
			switch(p->type) {
			case DATA_TYPE_STRING:
				json_add_string(v,p->field,p->ptr);
				break;
			case DATA_TYPE_INT:
				ip = p->ptr;
				json_add_number(v,p->field,*ip);
				break;
			case DATA_TYPE_FLOAT:
				fp = p->ptr;
				json_add_number(v,p->field,*fp);
				break;
			case DATA_TYPE_DOUBLE:
				dp = p->ptr;
				json_add_number(v,p->field,*dp);
				break;
			default:
				dprintf(1,"json_from_type: unhandled type: %d\n", p->type);
				break;
			}
		}
	}
	return v;
}

int json_to_tab(json_proctab_t *tab, json_value_t *v) {
	json_proctab_t *p;
	json_object_t *o;
	register int i;

	/* must be object */
	if (v->type != JSONObject) return 1;

	o = v->value.object;
        for (i = 0; i < o->count; i++) {
		dprintf(5,"name: %s\n", o->names[i]);
		for(p=tab; p->field; p++) {
			if (strcmp(p->field,o->names[i])==0) {
				if (p->cb) p->cb(p->field,p->ptr,p->len,o->values[i]);
				else {
					json_value_t *vv = o->values[i];
					switch(vv->type) {
					case JSONString:
						conv_type(p->type,p->ptr,p->len,DATA_TYPE_STRING,vv->value.string.chars,0);
						break;
					case JSONNumber:
						conv_type(p->type,p->ptr,p->len,DATA_TYPE_DOUBLE,&vv->value.number,0);
						break;
					case JSONBoolean:
						conv_type(p->type,p->ptr,p->len,DATA_TYPE_LOGICAL,&vv->value.boolean,0);
						break;
					}
				}
				break;
			}
		}
        }
	return 1;
}

void json_conv_value(enum DATA_TYPE dt, void *dest, int len, json_value_t *v) {
	switch(v->type) {
	case JSONString:
		conv_type(dt,dest,len,DATA_TYPE_STRING,v->value.string.chars,v->value.string.length);
		break;
	case JSONNumber:
		conv_type(dt,dest,len,DATA_TYPE_DOUBLE,&v->value.number,0);
		break;
	case JSONBoolean:
		conv_type(dt,dest,len,DATA_TYPE_INT,&v->value.boolean,0);
		break;
	default:
		dprintf(1,"invalid type: %d (%s)\n", v->type, json_typestr(v->type));
		break;
	}
}


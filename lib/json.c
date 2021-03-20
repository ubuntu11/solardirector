
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

json_object_t *json_create_object(void) {
	json_object_t *o;

#ifdef DEBUG_MEM
	if (!_init) {
		json_set_allocation_functions(mem_malloc,mem_free);
		_init = 1;
	}
#endif
	o = (json_object_t *) json_value_init_object();
	dprintf(5,"o: %p\n", o);
	return o;
}

json_value_t *json_create_array(void) {
	return json_value_init_array();
}

#if 0
json_array_t *json_create_array(void) {
	json_array_t *a;

	a = (json_array_t *) json_value_init_array();
	dprintf(5,"a: %p\n", a);
	return a;
}
#endif

int json_destroy(json_object_t *o) {
	json_value_free((JSON_Value *)o);
	return 0;
}

JSON_Value *json_parse(char *str) {
	return json_parse_string(str);
}

#if 0
void json_object_set(JSON_Object *o, int type, char *label, void *value) {
	JSON_Value *vv;
	double *dv;
	int *iv;

	switch(type) {
//	case JSONString:
	case DATA_TYPE_STRING:
		json_object_set_string(o,label,(char *)value);
		break;
	case JSONNumber:
		dv = value;
		json_object_set_number(o,label,*dv);
		break;
	case JSONObject:
		vv = (JSON_Value *) value;
		json_object_set_value(o,label,vv);
		break;
	case JSONArray:
		vv = (JSON_Value *) value;
		json_object_set_value(o,label,vv);
		break;
	case JSONBoolean:
		iv = value;
		json_object_set_boolean(o,label,*iv);
		break;
	default:
		break;
	}
}
#endif

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
	case DATA_TYPE_INT:
		json_object_set_string(to,"type","i32");
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

int json_add_string(json_object_t *o, char *name, char *value) {
	dprintf(5,"object: %p, name: %s, value: %s\n", o, name, value);
	return json_object_set_string(json_object((JSON_Value *)o), name, value);
}

int json_add_number(json_object_t *o, char *name, double value) {
	dprintf(5,"object: %p, name: %s, value: %f\n", o, name, value);
	return json_object_set_number(json_object((JSON_Value *)o), name, value);
}

int json_add_array(json_object_t *o, char *name, json_value_t *value) {
//	XXX json_object_set_value does not copy passed value so it shouldn't be freed afterwards.
	dprintf(5,"object: %p, name: %s, value: %p\n", o, name, value);
	return json_object_set_value(json_object((JSON_Value *)o), name, value);
}

int json_add_object(json_object_t *o, char *name, json_object_t *value) {
//	XXX json_object_set_value does not copy passed value so it shouldn't be freed afterwards.
	dprintf(5,"object: %p, name: %s, value: %p\n", o, name, value);
	return json_object_set_value(json_object((JSON_Value *)o), name, (JSON_Value *)value);
}

int json_add_descriptor(json_object_t *o,char *name,json_descriptor_t d) {
	JSON_Value *v;

	v = (JSON_Value *) json_create_descriptor(d);
	dprintf(5,"o: %p, name: %s, v: %p\n", o, name, v);
	return json_object_set_value(json_object((JSON_Value *)o),name,v);
}

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

int json_array_add_object(json_value_t *a,json_object_t *o) {
	return json_array_append_value(json_array(a),(JSON_Value *)o);
}

int json_array_add_descriptor(json_value_t *a,json_descriptor_t d) {
	JSON_Value *v;

	v = (JSON_Value *) json_create_descriptor(d);
	dprintf(5,"a: %p, v: %p\n", a, v);
	return json_array_append_value(json_array(a),v);
}

int json_tostring(json_object_t *j, char *dest, int dest_len, int pretty) {
	char *p;
	p = pretty ? json_serialize_to_string_pretty((JSON_Value *)j) : json_serialize_to_string((JSON_Value *)j);
	dprintf(5,"p: %s\n", p);
	dprintf(5,"dest_len: %d\n", dest_len);
	dest[0] = 0;
	strncat(dest,p,dest_len);
	dprintf(5,"dest: %s\n", dest);
	json_free_serialized_string(p);
	return 0;
}

char *json_dumps(json_object_t *o, int pretty) {
	return (pretty ? json_serialize_to_string_pretty((JSON_Value *)o) : json_serialize_to_string((JSON_Value *)o));
}

#if 0
json_data_t *json_create(void) {
	json_data_t *j;

	j = calloc(sizeof(*j),1);
	if (!j) {
		log_write(LOG_SYSERR,"json_create: calloc");
		return 0;
	}
	j->root_value = json_value_init_object();
	if (!j->root_value) goto json_create_error;
	j->root_object = json_value_get_object(j->root_value);
	if (!j->root_object) goto json_create_error;

	return j;
json_create_error:
	free(j);
	return 0;
}
#endif

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

#if 0
int tab_to_json(JSON_Object *root_object, cfg_proctab_t *tab) {
	return 1;
}

int json_to_tab(cfg_proctab_t *tab, JSON_Object *root_object) {
	return 1;
}
#endif

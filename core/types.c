
/*
Copyright (c) 2021, Stephen P. Shoecraft
All rights reserved.

This source code is licensed under the BSD-style license found in the
LICENSE file in the root directory of this source tree.
*/

#include <stdio.h>
#include <string.h>

#define DEBUG_TYPES 1
#define DLEVEL 4 

#ifdef DEBUG
#undef DEBUG
#define DEBUG DEBUG_TYPES
#endif
#include "debug.h"

static struct {
        int type;
        char *name;
	int size;
} type_info[] = {
        { DATA_TYPE_NULL,"NULL",0 },
        { DATA_TYPE_VOID,"VOID",0 },
	{ DATA_TYPE_STRING,"STRING",0 },
        { DATA_TYPE_S8,"S8",1 },
        { DATA_TYPE_S16,"S16",sizeof(int16_t) },
        { DATA_TYPE_S32,"S32",sizeof(int32_t) },
        { DATA_TYPE_INT,"INT",sizeof(int) },
        { DATA_TYPE_LONG,"LONG",sizeof(long) },
        { DATA_TYPE_S64,"S64",sizeof(int64_t) },
        { DATA_TYPE_QUAD,"QUAD",sizeof(long long) },
        { DATA_TYPE_U8,"U8",1 },
        { DATA_TYPE_U16,"U16",sizeof(uint16_t) },
        { DATA_TYPE_U32,"U32",sizeof(uint32_t) },
        { DATA_TYPE_U64,"U64",sizeof(uint64_t) },
        { DATA_TYPE_F32,"F32",sizeof(float) },
        { DATA_TYPE_FLOAT,"FLOAT",sizeof(float) },
        { DATA_TYPE_F64,"F64",sizeof(double) },
        { DATA_TYPE_DOUBLE,"DOUBLE",sizeof(double) },
        { DATA_TYPE_F128,"F128",sizeof(long double) },
        { DATA_TYPE_BOOL,"BOOL",sizeof(int) },
        { DATA_TYPE_STRING,"STRING",0 },
        { DATA_TYPE_S8_ARRAY,"S8_ARRAY",0 },
        { DATA_TYPE_S16_ARRAY,"S16_ARRAY",0 },
        { DATA_TYPE_S32_ARRAY,"S32_ARRAY",0 },
        { DATA_TYPE_S64_ARRAY,"S64_ARRAY",0 },
        { DATA_TYPE_U8_ARRAY,"U8_ARRAY",0 },
        { DATA_TYPE_U16_ARRAY,"U16_ARRAY",0 },
        { DATA_TYPE_U32_ARRAY,"U32_ARRAY",0 },
        { DATA_TYPE_U64_ARRAY,"U64_ARRAY",0 },
        { DATA_TYPE_F32_ARRAY,"F32_ARRAY",0 },
        { DATA_TYPE_F64_ARRAY,"F64_ARRAY",0 },
        { DATA_TYPE_F128_ARRAY,"F128_ARRAY",0 },
        { DATA_TYPE_BOOL_ARRAY,"BOOL_ARRAY",0 },
        { DATA_TYPE_STRING_ARRAY,"STRING_ARRAY",0 },
        { DATA_TYPE_STRING_LIST,"STRING_LIST",0 },
        { 0, 0 }
};

char *typestr(int type) {
        register int x;

//	printf("type: %d\n", type);
        for(x=0; type_info[x].name; x++) {
//		printf("name: %s, type: %d\n", type_info[x].name, type_info[x].type);
                if (type_info[x].type == type) {
//			printf("found\n");
                        return(type_info[x].name);
		}
        }
//	printf("NOT found\n");
        return("*BAD TYPE*");
}

int typesize(int type) {
        register int x;

        for(x=0; type_info[x].name; x++) {
                if (type_info[x].type == type)
                        return(type_info[x].size);
        }
        return(0);
}

int name2dt(char *name) {
        register int x;

        for(x=0; type_info[x].name; x++) {
                if (strcmp(name,type_info[x].name)==0)
                        return type_info[x].type;
        }
        return DATA_TYPE_UNKNOWN;
}

#ifdef JS
#include "jsapi.h"
#include "jsengine.h"
static JSBool jstypes_typestr(JSContext *cx, uintN argc, jsval *vp) {
	int type;
	char *str;
	jsval *argv = vp + 2;

	if (argc != 1) {
		JS_ReportError(cx, "typestr: 1 argument required (type:number)");
		return JS_FALSE;
	}
	type = JSVAL_TO_INT(argv[0]);
	printf("==> type: %d\n", type);
	str = typestr(type);
	printf("==> str: %s\n", str);
	*vp = type_to_jsval(cx, DATA_TYPE_STRING, str, strlen(str));
	return JS_TRUE;
}

static int jstypes_init(JSContext *cx, JSObject *parent, void *priv) {
	JSObject *obj = JS_GetGlobalObject(cx);
	char *name;
	int i,count,namesz;
	JSFunctionSpec funcs[] = {
		JS_FN("typestr",jstypes_typestr,1,1,0),
		{ 0 }
	};
	JSConstantSpec *consts;

	if (!JS_DefineFunctions(cx, obj, funcs)) {
		dprintf(1,"error defining types funcs\n");
		return 1;
	}

	for(i=count=0; type_info[i].name; i++) count++;
	consts = malloc((count+1)*sizeof(JSConstantSpec));
	if (!consts) {
		log_syserror("JSTypes: malloc");
		return 1;
	}
#define PREFIX "DATA_TYPE_"
	for(i=0; i < count; i++) {
		namesz = sizeof(PREFIX)+strlen(type_info[i].name)+1;
		name = malloc(namesz);
		if (!name) break;
		snprintf(name,namesz,"%s%s",PREFIX,type_info[i].name);
//		printf("==> ADDING: name: %s, type: %d\n", name, type_info[i].type);
		consts[i].name = name;
		consts[i].value = NUMBER_TO_JSVAL(type_info[i].type);
	}
	consts[i].name = 0;
	if(!JS_DefineConstants(cx, obj, consts)) {
		dprintf(1,"error defining types constants\n");
		return 1;
	}
	return 0;
}

int types_jsinit(JSEngine *e) {
	return JS_EngineAddInitFunc(e, "types", jstypes_init, 0);
}
#endif

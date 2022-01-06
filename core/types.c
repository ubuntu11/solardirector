
/*
Copyright (c) 2021, Stephen P. Shoecraft
All rights reserved.

This source code is licensed under the BSD-style license found in the
LICENSE file in the root directory of this source tree.
*/

#include <stdio.h>
#include <string.h>

#define DEBUG_TYPES 0
#define DLEVEL 4 

#ifdef DEBUG
#undef DEBUG
#endif
#if DEBUG_TYPES
#define DEBUG 1
#endif
#include "debug.h"

static struct {
        int type;
        char *name;
	int size;
} type_info[] = {
        { DATA_TYPE_NULL,"void",0 },
	{ DATA_TYPE_STRING,"string",0 },
        { DATA_TYPE_S8,"S8",1 },
        { DATA_TYPE_S16,"S16",sizeof(short) },
        { DATA_TYPE_S32,"S32",sizeof(long) },
        { DATA_TYPE_S64,"S64",sizeof(long long) },
        { DATA_TYPE_U8,"U8",1 },
        { DATA_TYPE_U16,"U16",sizeof(short) },
        { DATA_TYPE_U32,"U32",sizeof(long) },
        { DATA_TYPE_U64,"U64",sizeof(long long) },
        { DATA_TYPE_F32,"F32",sizeof(float) },
        { DATA_TYPE_F64,"F64",sizeof(double) },
        { DATA_TYPE_F128,"F128",sizeof(long double) },
        { DATA_TYPE_BOOL,"bool",sizeof(int) },
        { DATA_TYPE_STRING,"string",0 },
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

        for(x=0; type_info[x].name; x++) {
                if (type_info[x].type == type)
                        return(type_info[x].name);
        }
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

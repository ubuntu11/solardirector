
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
} type_info[] = {
        { DATA_TYPE_UNKNOWN,"UNKNOWN" },
        { DATA_TYPE_CHAR,"CHAR" },
        { DATA_TYPE_STRING,"STRING" },
        { DATA_TYPE_BYTE,"BYTE" },
        { DATA_TYPE_SHORT,"SHORT" },
        { DATA_TYPE_INT,"INT" },
        { DATA_TYPE_LONG,"LONG" },
        { DATA_TYPE_QUAD,"QUAD" },
        { DATA_TYPE_FLOAT,"FLOAT" },
        { DATA_TYPE_DOUBLE,"DOUBLE" },
        { DATA_TYPE_LOGICAL,"BOOLEAN" },
        { DATA_TYPE_DATE,"DATE" },
        { DATA_TYPE_LIST,"LIST" },
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

int name2dt(char *name) {
        register int x;

        for(x=0; type_info[x].name; x++) {
                if (strcmp(name,type_info[x].name)==0)
                        return type_info[x].type;
        }
        return DATA_TYPE_UNKNOWN;
}


/*
Copyright (c) 2021, Stephen P. Shoecraft
All rights reserved.

This source code is licensed under the BSD-style license found in the
LICENSE file in the root directory of this source tree.
*/

#ifndef __SD_TYPES_H
#define __SD_TYPES_H

#include <stdint.h>
#include <sys/types.h>

#if 0
        DATA_TYPE_UNKNOWN = 0,          /* Unknown */
        DATA_TYPE_CHAR,                 /* Array of chars w/no null */
        DATA_TYPE_STRING,               /* Array of chars w/null */
        DATA_TYPE_BYTE,                 /* char -- 8 bit */
        DATA_TYPE_SHORT,                /* Short -- 16 bit */
        DATA_TYPE_INT,                  /* Integer -- 16 | 32 bit */
        DATA_TYPE_LONG,                 /* Long -- 32 bit */
        DATA_TYPE_QUAD,                 /* Quadword - 64 bit */
        DATA_TYPE_FLOAT,                /* Float -- 32 bit */
        DATA_TYPE_DOUBLE,               /* Double -- 64 bit */
        DATA_TYPE_BOOL,              /* (int) Yes/No,True/False,on/off */
        DATA_TYPE_DATE,                 /* (char 23) DD-MMM-YYYY HH:MM:SS.HH */
        DATA_TYPE_LIST,                 /* Itemlist */
        DATA_TYPE_MAX                   /* Max data type number */
#endif

enum DATA_TYPE {
	DATA_TYPE_NULL = 0,
	DATA_TYPE_CHAR,
	DATA_TYPE_STRING,
	DATA_TYPE_I8,
	DATA_TYPE_I16,
	DATA_TYPE_I32,
	DATA_TYPE_LONG,
	DATA_TYPE_I64,
	DATA_TYPE_F32,
	DATA_TYPE_F64,
	DATA_TYPE_BOOL,
        DATA_TYPE_TIMESTAMP,		/* (char 22) YYYYMMDD HH:MM:SS.HHHH */
        DATA_TYPE_LIST,                 /* Itemlist */
	DATA_TYPE_I128,
	DATA_TYPE_U8,
	DATA_TYPE_U16,
	DATA_TYPE_U32,
	DATA_TYPE_U64,
	DATA_TYPE_U128,
        DATA_TYPE_DATE,                 /* (char 23) DD/MM/YYYY */
	DATA_TYPE_BITMASK,		/* Char 64 0s and 1s */
	DATA_TYPE_IPV4,			/* Char 64 0s and 1s */
	DATA_TYPE_IPV6,			/* Char 64 0s and 1s */
	DATA_TYPE_PHONE,		/* +CC (AAA)NNN-NNNN */
	DATA_TYPE_EMAIL,		/* a@b.d */
	DATA_TYPE_MAX,
};

#define DATA_TYPE_UNKNOWN DATA_TYPE_NULL
#define DATA_TYPE_BYTE DATA_TYPE_I8
#define	DATA_TYPE_SHORT DATA_TYPE_I16
#define	DATA_TYPE_INT DATA_TYPE_I32
#define	DATA_TYPE_LONG DATA_TYPE_I32
#define	DATA_TYPE_QUAD DATA_TYPE_I64
#define DATA_TYPE_UBYTE DATA_TYPE_U8
#define DATA_TYPE_USHORT DATA_TYPE_U16
#define DATA_TYPE_UINT DATA_TYPE_U32
#define DATA_TYPE_ULONG DATA_TYPE_U32
#define DATA_TYPE_UQUAD DATA_TYPE_U64
#define DATA_TYPE_FLOAT DATA_TYPE_F32
#define DATA_TYPE_DOUBLE DATA_TYPE_F64
#define DATA_TYPE_LOGICAL DATA_TYPE_BOOL
#define DATA_TYPE_STATE DATA_TYPE_BOOL

//typedef uint8_t bool_t;
typedef int bool_t;

#endif

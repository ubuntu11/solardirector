
#include "utils.h"
#include "debug.h"

char *type2str(int type) {
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
        DATA_TYPE_LOGICAL,              /* (int) Yes/No,True/False,on/off */
        DATA_TYPE_DATE,                 /* (char 23) DD-MMM-YYYY HH:MM:SS.HH */
        DATA_TYPE_LIST,                 /* Itemlist */
        DATA_TYPE_MAX                   /* Max data type number */

	char *typestr[] = {
		"NULL",
		"CHAR",
		"STRING",
		"I8",
		"I16",
		"I32",
		"I64",
		"F32",
		"F64",
		"BOOL",
		"DATE",
		"LIST",
		"I128",
		"U8",
		"U16",
		"U32",
		"U64",
		"U128",
		"TIMESTAMP",
		"BITMASK",
		"IPV4",
		"IPV6",
		"PHONE",
		"EMAIL",
		"MAX",
	};
	char *r;

	dprintf(1,"type: %d\n", type);
	r = (type >= 0 && type < DATA_TYPE_MAX ? typestr[type] : "<invalid type>");
	dprintf(1,"returning: %s\n", r);
	return r;
}

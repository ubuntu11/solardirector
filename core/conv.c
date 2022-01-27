
/*
Copyright (c) 2021, Stephen P. Shoecraft
All rights reserved.

This source code is licensed under the BSD-style license found in the
LICENSE file in the root directory of this source tree.
*/

#define DEBUG_CONV 1
#define dlevel 4

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <strings.h>
#include <limits.h>
#include <stdlib.h>
#include <errno.h>
#include <ctype.h>
#include "utils.h"
#include "list.h"

#ifdef DEBUG
#undef DEBUG
#endif
#if DEBUG_CONV
#define DEBUG 1
#endif
#include "debug.h"

#ifdef __WIN32
#include <inttypes.h>
#endif

typedef enum {
    STR2INT_SUCCESS,
    STR2INT_OVERFLOW,
    STR2INT_UNDERFLOW,
    STR2INT_INCONVERTIBLE
} str2int_errno;

/* Convert string s to int out.
 *
 * @param[out] out The converted int. Cannot be NULL.
 *
 * @param[in] s Input string to be converted.
 *
 *     The format is the same as strtol,
 *     except that the following are inconvertible:
 *
 *     - empty string
 *     - leading whitespace
 *     - any trailing characters that are not part of the number
 *
 *     Cannot be NULL.
 *
 * @param[in] base Base to interpret string in. Same range as strtol (2 to 36).
 *
 * @return Indicates if the operation succeeded, or why it failed.
 */
str2int_errno str2int(int *out, char *s, int base) {
    char *end;
    if (s[0] == '\0' || isspace(s[0]))
        return STR2INT_INCONVERTIBLE;
    errno = 0;
    long l = strtol(s, &end, base);
    /* Both checks are needed because INT_MAX == LONG_MAX is possible. */
    if (l > INT_MAX || (errno == ERANGE && l == LONG_MAX))
        return STR2INT_OVERFLOW;
    if (l < INT_MIN || (errno == ERANGE && l == LONG_MIN))
        return STR2INT_UNDERFLOW;
    if (*end != '\0')
        return STR2INT_INCONVERTIBLE;
    *out = l;
    return STR2INT_SUCCESS;
}

void str2list(list *dest,char *src) {
	char temp[2048];
	int tmax,tlen,q;
	register char *p;
	register int i;

	dprintf(dlevel,"dest: %p, src: %s", dest, src);
	*dest = list_create();
	dprintf(dlevel,"src: %s",src);
	if (!src) return;

	p = src;
	tmax = sizeof(temp)-1;
	i = q = 0;
	while(*p) {
		dprintf(dlevel,"p: %c, q: %d\n", *p, q);
		if (*p == '\"' && !q)
			q = 1;
		else if (*p == '\"' && q)
			q = 0;
		else if ((*p == ',' && !q) || i >= tmax) {
			trim(temp);
			tlen = strlen(temp);
			dprintf(dlevel,"adding: %s (%d)\n", temp, tlen);
			if (tlen) list_add(*dest,temp,tlen+1);
			i = 0;
		} else {
			temp[i++] = *p;
		}
		p++;
	}
	if (i) {
		temp[i] = 0;
		trim(temp);
		tlen = strlen(temp);
		dprintf(dlevel,"adding: %s (%d)\n", temp, tlen);
		if (tlen) list_add(*dest,temp,tlen+1);
	}
#if DEBUG
	dprintf(dlevel,"list:");
	list_reset(*dest);
	while( (p = list_get_next(*dest)) != 0) dprintf(dlevel,"\t%s",p);
#endif
	return;
}

int conv_type(int dt,void *d,int dl,int st,void *s,int sl) {
	register int i;
	int return_len;

	return_len = -1;
	switch(dt) {
	case DATA_TYPE_STRING:
	    {
		char *dest = d;
		switch(st) {
		case DATA_TYPE_STRING:
			*dest = 0;
			strncat(dest,s,dl);
			trim(dest);
			break;
		case DATA_TYPE_S8:
			return_len = snprintf(d,dl,"%d",*((int8_t *)s));
			break;
		case DATA_TYPE_S16:
			return_len = snprintf(d,dl,"%d",*((int16_t *)s));
			break;
		case DATA_TYPE_S32:
			return_len = snprintf(d,dl,"%d",*((int32_t *)s));
			break;
		case DATA_TYPE_S64:
			return_len = snprintf(d,dl,"%lld",*((long long *)s));
			break;
		case DATA_TYPE_U8:
			return_len = snprintf(d,dl,"%u",*((uint8_t *)s));
			break;
		case DATA_TYPE_U16:
			return_len = snprintf(d,dl,"%d",*((uint16_t *)s));
			break;
		case DATA_TYPE_U32:
			return_len = snprintf(d,dl,"%d",*((uint32_t *)s));
			break;
		case DATA_TYPE_U64:
			return_len = snprintf(d,dl,"%lld",*((unsigned long long int *)s));
			break;
		case DATA_TYPE_F32:
			return_len = snprintf(d,dl,"%f",*((float *)s));
			break;
		case DATA_TYPE_F64:
			return_len = snprintf(d,dl,"%lf",*((double *)s));
			break;
		case DATA_TYPE_BOOL:
			return_len = snprintf(d,dl,"%s", *((int *)s) ? "true" : "false");
			break;
		case DATA_TYPE_F32_ARRAY:
		    {
			float *fa = s;
			char *p = d;

			*p = 0;
			for(i=0; i < sl; i++) p += sprintf(p,"%s%f",(i ? "," : ""),fa[i]);
			return_len = strlen((char *)d);
		    }
		    break;
		case DATA_TYPE_S8_ARRAY:
		case DATA_TYPE_S16_ARRAY:
		case DATA_TYPE_S32_ARRAY:
		case DATA_TYPE_S64_ARRAY:
		case DATA_TYPE_U8_ARRAY:
		case DATA_TYPE_U16_ARRAY:
		case DATA_TYPE_U32_ARRAY:
		case DATA_TYPE_U64_ARRAY:
		case DATA_TYPE_F64_ARRAY:
		case DATA_TYPE_STRING_ARRAY:
		case DATA_TYPE_STRING_LIST:
		    {
			int first,dlen,slen;
			char *dest = d;
			list l;
			register char *p;

			dprintf(dlevel,"dest: %p, dlen: %d, list: %p, sl: %d", dest, dl, s, sl);
			if (!s) break;
			l = (list)s;
			dprintf(dlevel,"l: %p\n", l);
			if (!l) break;
			*dest = 0;
			dlen = 0;
			first = 1;
			list_reset(l);
			while( (p = list_get_next(l)) != 0) {
				dprintf(dlevel,"p: %s\n", p);
				slen = strlen(p);
				dprintf(dlevel,"list2str: ptr: %s, len: %d",p,slen);
				dprintf(dlevel,"dlen: %d, slen: %d, dl: %d",dlen,slen,dl);
				if (dlen + slen >= dl) break;
				if (first == 0) strcat(dest,",");
				else first = 0;
				strcat(dest,p);
			}
			dprintf(dlevel,"dest: %s",dest);
		    }
		    break;
		default:
			dprintf(0,"**** conv dt: %04x(%s) unhandled st: %04x(%s)\n", dt, typestr(dt), st, typestr(st));
			break;
		}
	    }
	    break;
	case DATA_TYPE_S8:
	    {
		switch(st) {
		case DATA_TYPE_STRING:
			*((int8_t *)d) = strtol(s,0,0);
			break;
		case DATA_TYPE_S8:
			*((int8_t *)d) = *((int8_t *)s);
			break;
		case DATA_TYPE_S16:
			*((int8_t *)d) = *((int16_t *)s);
			break;
		case DATA_TYPE_S32:
			*((int8_t *)d) = *((int32_t *)s);
			break;
		case DATA_TYPE_S64:
			*((int8_t *)d) = *((int64_t *)s);
			break;
		case DATA_TYPE_U8:
			*((int8_t *)d) = *((uint8_t *)s);
			break;
		case DATA_TYPE_U16:
			*((int8_t *)d) = *((uint16_t *)s);
			break;
		case DATA_TYPE_U32:
			*((int8_t *)d) = *((uint32_t *)s);
			break;
		case DATA_TYPE_U64:
			*((int8_t *)d) = *((uint64_t *)s);
			break;
		case DATA_TYPE_F32:
			*((int8_t *)d) = *((float *)s);
			break;
		case DATA_TYPE_F64:
			*((int8_t *)d) = *((double *)s);
			break;
		case DATA_TYPE_F128:
			*((int8_t *)d) = *((long double *)s);
			break;
		case DATA_TYPE_BOOL:
			*((int8_t *)d) = *((int *)s);
			break;
		case DATA_TYPE_STRING_ARRAY:
		    {
			char **sa = (char **)s;
			for(i=0; i < sl; i++) dprintf(1,"sa[%d]: %s\n", i, sa[i]);
			*((int8_t *)d) = strtol(sa[0],0,0);
		    }
		    break;
		case DATA_TYPE_S8_ARRAY:
		case DATA_TYPE_S16_ARRAY:
		case DATA_TYPE_S32_ARRAY:
		case DATA_TYPE_S64_ARRAY:
		case DATA_TYPE_U8_ARRAY:
		case DATA_TYPE_U16_ARRAY:
		case DATA_TYPE_U32_ARRAY:
		case DATA_TYPE_U64_ARRAY:
		case DATA_TYPE_F32_ARRAY:
		case DATA_TYPE_F64_ARRAY:
		case DATA_TYPE_STRING_LIST:
		default:
			dprintf(0,"**** conv dt: %04x(%s) unhandled st: %04x(%s)\n", dt, typestr(dt), st, typestr(st));
			break;
		}
	    }
	    break;
	case DATA_TYPE_S16:
	    {
		switch(st) {
		case DATA_TYPE_STRING:
			*((int16_t *)d) = strtol(s,0,0);
			break;
		case DATA_TYPE_S8:
			*((int16_t *)d) = *((int8_t *)s);
			break;
		case DATA_TYPE_S16:
			*((int16_t *)d) = *((int16_t *)s);
			break;
		case DATA_TYPE_S32:
			*((int16_t *)d) = *((int32_t *)s);
			break;
		case DATA_TYPE_S64:
			*((int16_t *)d) = *((int64_t *)s);
			break;
		case DATA_TYPE_U8:
			*((int16_t *)d) = *((uint8_t *)s);
			break;
		case DATA_TYPE_U16:
			*((int16_t *)d) = *((uint16_t *)s);
			break;
		case DATA_TYPE_U32:
			*((int16_t *)d) = *((uint32_t *)s);
			break;
		case DATA_TYPE_U64:
			*((int16_t *)d) = *((uint64_t *)s);
			break;
		case DATA_TYPE_F32:
			*((int16_t *)d) = *((float *)s);
			break;
		case DATA_TYPE_F64:
			*((int16_t *)d) = *((double *)s);
			break;
		case DATA_TYPE_F128:
			*((int16_t *)d) = *((long double *)s);
			break;
		case DATA_TYPE_BOOL:
			*((int16_t *)d) = *((int *)s);
			break;
		case DATA_TYPE_STRING_ARRAY:
		    {
			char **sa = (char **)s;
			*((int16_t *)d) = strtol(sa[0],0,0);
		    }
		    break;
		case DATA_TYPE_S8_ARRAY:
		case DATA_TYPE_S16_ARRAY:
		case DATA_TYPE_S32_ARRAY:
		case DATA_TYPE_S64_ARRAY:
		case DATA_TYPE_U8_ARRAY:
		case DATA_TYPE_U16_ARRAY:
		case DATA_TYPE_U32_ARRAY:
		case DATA_TYPE_U64_ARRAY:
		case DATA_TYPE_F32_ARRAY:
		case DATA_TYPE_F64_ARRAY:
		case DATA_TYPE_STRING_LIST:
		default:
			dprintf(0,"**** conv dt: %04x(%s) unhandled st: %04x(%s)\n", dt, typestr(dt), st, typestr(st));
			break;
		}
	    }
	    break;
	case DATA_TYPE_S32:
	    {
		switch(st) {
		case DATA_TYPE_STRING:
			*((int32_t *)d) = strtol(s,0,0);
			break;
		case DATA_TYPE_S8:
			*((int32_t *)d) = *((int8_t *)s);
			break;
		case DATA_TYPE_S16:
			*((int32_t *)d) = *((int16_t *)s);
			break;
		case DATA_TYPE_S32:
			*((int32_t *)d) = *((int32_t *)s);
			break;
		case DATA_TYPE_S64:
			*((int32_t *)d) = *((int64_t *)s);
			break;
		case DATA_TYPE_U8:
			*((int32_t *)d) = *((uint8_t *)s);
			break;
		case DATA_TYPE_U16:
			*((int32_t *)d) = *((uint16_t *)s);
			break;
		case DATA_TYPE_U32:
			*((int32_t *)d) = *((uint32_t *)s);
			break;
		case DATA_TYPE_U64:
			*((int32_t *)d) = *((uint64_t *)s);
			break;
		case DATA_TYPE_F32:
			*((int32_t *)d) = *((float *)s);
			break;
		case DATA_TYPE_F64:
			*((int32_t *)d) = *((double *)s);
			break;
		case DATA_TYPE_F128:
			*((int32_t *)d) = *((long double *)s);
			break;
		case DATA_TYPE_BOOL:
			*((int32_t *)d) = *((int *)s);
			break;
		case DATA_TYPE_STRING_ARRAY:
		    {
			char **sa = (char **)s;
			for(i=0; i < sl; i++) dprintf(1,"sa[%d]: %s\n", i, sa[i]);
			*((int32_t *)d) = strtol(sa[0],0,0);
		    }
		    break;
		case DATA_TYPE_S8_ARRAY:
		case DATA_TYPE_S16_ARRAY:
		case DATA_TYPE_S32_ARRAY:
		case DATA_TYPE_S64_ARRAY:
		case DATA_TYPE_U8_ARRAY:
		case DATA_TYPE_U16_ARRAY:
		case DATA_TYPE_U32_ARRAY:
		case DATA_TYPE_U64_ARRAY:
		case DATA_TYPE_F32_ARRAY:
		case DATA_TYPE_F64_ARRAY:
		case DATA_TYPE_STRING_LIST:
		default:
			dprintf(0,"**** conv dt: %04x(%s) unhandled st: %04x(%s)\n", dt, typestr(dt), st, typestr(st));
			break;
		}
	    }
	    break;
	case DATA_TYPE_S64:
	    {
		switch(st) {
		case DATA_TYPE_STRING:
			*((int64_t *)d) = strtol(s,0,0);
			break;
		case DATA_TYPE_S8:
			*((int64_t *)d) = *((int8_t *)s);
			break;
		case DATA_TYPE_S16:
			*((int64_t *)d) = *((int16_t *)s);
			break;
		case DATA_TYPE_S32:
			*((int64_t *)d) = *((int32_t *)s);
			break;
		case DATA_TYPE_S64:
			*((int64_t *)d) = *((int64_t *)s);
			break;
		case DATA_TYPE_U8:
			*((int64_t *)d) = *((uint8_t *)s);
			break;
		case DATA_TYPE_U16:
			*((int64_t *)d) = *((uint16_t *)s);
			break;
		case DATA_TYPE_U32:
			*((int64_t *)d) = *((uint32_t *)s);
			break;
		case DATA_TYPE_U64:
			*((int64_t *)d) = *((uint64_t *)s);
			break;
		case DATA_TYPE_F32:
			*((int64_t *)d) = *((float *)s);
			break;
		case DATA_TYPE_F64:
			*((int64_t *)d) = *((double *)s);
			break;
		case DATA_TYPE_F128:
			*((int64_t *)d) = *((long double *)s);
			break;
		case DATA_TYPE_BOOL:
			*((int64_t *)d) = *((int *)s);
			break;
		case DATA_TYPE_STRING_ARRAY:
		    {
			char **sa = (char **)s;
			*((int64_t *)d) = strtoll(sa[0],0,0);
		    }
		    break;
		case DATA_TYPE_S8_ARRAY:
		case DATA_TYPE_S16_ARRAY:
		case DATA_TYPE_S32_ARRAY:
		case DATA_TYPE_S64_ARRAY:
		case DATA_TYPE_U8_ARRAY:
		case DATA_TYPE_U16_ARRAY:
		case DATA_TYPE_U32_ARRAY:
		case DATA_TYPE_U64_ARRAY:
		case DATA_TYPE_F32_ARRAY:
		case DATA_TYPE_F64_ARRAY:
		case DATA_TYPE_STRING_LIST:
		default:
			dprintf(0,"**** conv dt: %04x(%s) unhandled st: %04x(%s)\n", dt, typestr(dt), st, typestr(st));
			break;
		}
	    }
	    break;
	case DATA_TYPE_U8:
	    {
		switch(st) {
		case DATA_TYPE_STRING:
			*((uint8_t *)d) = strtol(s,0,0);
			break;
		case DATA_TYPE_S8:
			*((uint8_t *)d) = *((int8_t *)s);
			break;
		case DATA_TYPE_S16:
			*((uint8_t *)d) = *((int16_t *)s);
			break;
		case DATA_TYPE_S32:
			*((uint8_t *)d) = *((int32_t *)s);
			break;
		case DATA_TYPE_S64:
			*((uint8_t *)d) = *((int64_t *)s);
			break;
		case DATA_TYPE_U8:
			*((uint8_t *)d) = *((uint8_t *)s);
			break;
		case DATA_TYPE_U16:
			*((uint8_t *)d) = *((uint16_t *)s);
			break;
		case DATA_TYPE_U32:
			*((uint8_t *)d) = *((uint32_t *)s);
			break;
		case DATA_TYPE_U64:
			*((uint8_t *)d) = *((uint64_t *)s);
			break;
		case DATA_TYPE_F32:
			*((uint8_t *)d) = *((float *)s);
			break;
		case DATA_TYPE_F64:
			*((uint8_t *)d) = *((double *)s);
			break;
		case DATA_TYPE_F128:
			*((uint8_t *)d) = *((long double *)s);
			break;
		case DATA_TYPE_BOOL:
			*((uint8_t *)d) = *((int *)s);
			break;
		case DATA_TYPE_STRING_ARRAY:
		    {
			char **sa = (char **)s;
			*((uint8_t *)d) = strtol(sa[0],0,0);
		    }
		    break;
		case DATA_TYPE_S8_ARRAY:
		case DATA_TYPE_S16_ARRAY:
		case DATA_TYPE_S32_ARRAY:
		case DATA_TYPE_S64_ARRAY:
		case DATA_TYPE_U8_ARRAY:
		case DATA_TYPE_U16_ARRAY:
		case DATA_TYPE_U32_ARRAY:
		case DATA_TYPE_U64_ARRAY:
		case DATA_TYPE_F32_ARRAY:
		case DATA_TYPE_F64_ARRAY:
		case DATA_TYPE_STRING_LIST:
		default:
			dprintf(0,"**** conv dt: %04x(%s) unhandled st: %04x(%s)\n", dt, typestr(dt), st, typestr(st));
			break;
		}
	    }
	    break;
	case DATA_TYPE_U16:
	    {
		switch(st) {
		case DATA_TYPE_STRING:
			*((uint16_t *)d) = strtol(s,0,0);
			break;
		case DATA_TYPE_S8:
			*((uint16_t *)d) = *((int8_t *)s);
			break;
		case DATA_TYPE_S16:
			*((uint16_t *)d) = *((int16_t *)s);
			break;
		case DATA_TYPE_S32:
			*((uint16_t *)d) = *((int32_t *)s);
			break;
		case DATA_TYPE_S64:
			*((uint16_t *)d) = *((int64_t *)s);
			break;
		case DATA_TYPE_U8:
			*((uint16_t *)d) = *((uint8_t *)s);
			break;
		case DATA_TYPE_U16:
			*((uint16_t *)d) = *((uint16_t *)s);
			break;
		case DATA_TYPE_U32:
			*((uint16_t *)d) = *((uint32_t *)s);
			break;
		case DATA_TYPE_U64:
			*((uint16_t *)d) = *((uint64_t *)s);
			break;
		case DATA_TYPE_F32:
			*((uint16_t *)d) = *((float *)s);
			break;
		case DATA_TYPE_F64:
			*((uint16_t *)d) = *((double *)s);
			break;
		case DATA_TYPE_F128:
			*((uint16_t *)d) = *((long double *)s);
			break;
		case DATA_TYPE_BOOL:
			*((uint16_t *)d) = *((int *)s);
			break;
		case DATA_TYPE_STRING_ARRAY:
		    {
			char **sa = (char **)s;
			*((uint16_t *)d) = strtol(sa[0],0,0);
		    }
		    break;
		case DATA_TYPE_S8_ARRAY:
		case DATA_TYPE_S16_ARRAY:
		case DATA_TYPE_S32_ARRAY:
		case DATA_TYPE_S64_ARRAY:
		case DATA_TYPE_U8_ARRAY:
		case DATA_TYPE_U16_ARRAY:
		case DATA_TYPE_U32_ARRAY:
		case DATA_TYPE_U64_ARRAY:
		case DATA_TYPE_F32_ARRAY:
		case DATA_TYPE_F64_ARRAY:
		case DATA_TYPE_STRING_LIST:
		default:
			dprintf(0,"**** conv dt: %04x(%s) unhandled st: %04x(%s)\n", dt, typestr(dt), st, typestr(st));
			break;
		}
	    }
	    break;
	case DATA_TYPE_U32:
	    {
		switch(st) {
		case DATA_TYPE_STRING:
			*((uint32_t *)d) = strtoul(s,0,0);
			break;
		case DATA_TYPE_S8:
			*((uint32_t *)d) = *((int8_t *)s);
			break;
		case DATA_TYPE_S16:
			*((uint32_t *)d) = *((int16_t *)s);
			break;
		case DATA_TYPE_S32:
			*((uint32_t *)d) = *((int32_t *)s);
			break;
		case DATA_TYPE_S64:
			*((uint32_t *)d) = *((int64_t *)s);
			break;
		case DATA_TYPE_U8:
			*((uint32_t *)d) = *((uint8_t *)s);
			break;
		case DATA_TYPE_U16:
			*((uint32_t *)d) = *((uint16_t *)s);
			break;
		case DATA_TYPE_U32:
			*((uint32_t *)d) = *((uint32_t *)s);
			break;
		case DATA_TYPE_U64:
			*((uint32_t *)d) = *((uint64_t *)s);
			break;
		case DATA_TYPE_F32:
			*((uint32_t *)d) = *((float *)s);
			break;
		case DATA_TYPE_F64:
			*((uint32_t *)d) = *((double *)s);
			break;
		case DATA_TYPE_F128:
			*((uint32_t *)d) = *((long double *)s);
			break;
		case DATA_TYPE_BOOL:
			*((uint32_t *)d) = *((int *)s);
			break;
		case DATA_TYPE_STRING_ARRAY:
		    {
			char **sa = (char **)s;
			*((uint32_t *)d) = strtol(sa[0],0,0);
		    }
		case DATA_TYPE_S8_ARRAY:
		case DATA_TYPE_S16_ARRAY:
		case DATA_TYPE_S32_ARRAY:
		case DATA_TYPE_S64_ARRAY:
		case DATA_TYPE_U8_ARRAY:
		case DATA_TYPE_U16_ARRAY:
		case DATA_TYPE_U32_ARRAY:
		case DATA_TYPE_U64_ARRAY:
		case DATA_TYPE_F32_ARRAY:
		case DATA_TYPE_F64_ARRAY:
		case DATA_TYPE_STRING_LIST:
		default:
			dprintf(0,"**** conv dt: %04x(%s) unhandled st: %04x(%s)\n", dt, typestr(dt), st, typestr(st));
			break;
		}
	    }
	    break;
	case DATA_TYPE_U64:
	    {
		switch(st) {
		case DATA_TYPE_STRING:
			*((uint64_t *)d) = strtol(s,0,0);
			break;
		case DATA_TYPE_S8:
			*((uint64_t *)d) = *((int8_t *)s);
			break;
		case DATA_TYPE_S16:
			*((uint64_t *)d) = *((int16_t *)s);
			break;
		case DATA_TYPE_S32:
			*((uint64_t *)d) = *((int32_t *)s);
			break;
		case DATA_TYPE_S64:
			*((uint64_t *)d) = *((int64_t *)s);
			break;
		case DATA_TYPE_U8:
			*((uint64_t *)d) = *((uint8_t *)s);
			break;
		case DATA_TYPE_U16:
			*((uint64_t *)d) = *((uint16_t *)s);
			break;
		case DATA_TYPE_U32:
			*((uint64_t *)d) = *((uint32_t *)s);
			break;
		case DATA_TYPE_U64:
			*((uint64_t *)d) = *((uint64_t *)s);
			break;
		case DATA_TYPE_F32:
			*((uint64_t *)d) = *((float *)s);
			break;
		case DATA_TYPE_F64:
			*((uint64_t *)d) = *((double *)s);
			break;
		case DATA_TYPE_F128:
			*((uint64_t *)d) = *((long double *)s);
			break;
		case DATA_TYPE_BOOL:
			*((uint64_t *)d) = *((int *)s);
			break;
		case DATA_TYPE_STRING_ARRAY:
		    {
			char **sa = (char **)s;
			*((uint64_t *)d) = strtol(sa[0],0,0);
		    }
		    break;
		case DATA_TYPE_S8_ARRAY:
		case DATA_TYPE_S16_ARRAY:
		case DATA_TYPE_S32_ARRAY:
		case DATA_TYPE_S64_ARRAY:
		case DATA_TYPE_U8_ARRAY:
		case DATA_TYPE_U16_ARRAY:
		case DATA_TYPE_U32_ARRAY:
		case DATA_TYPE_U64_ARRAY:
		case DATA_TYPE_F32_ARRAY:
		case DATA_TYPE_F64_ARRAY:
		case DATA_TYPE_STRING_LIST:
		default:
			dprintf(0,"**** conv dt: %04x(%s) unhandled st: %04x(%s)\n", dt, typestr(dt), st, typestr(st));
			break;
		}
	    }
	    break;
	case DATA_TYPE_F32:
	    {
		switch(st) {
		case DATA_TYPE_STRING:
			*((float *)d) = strtod(s,0);
			break;
		case DATA_TYPE_S8:
			*((float *)d) = *((int8_t *)s);
			break;
		case DATA_TYPE_S16:
			*((float *)d) = *((int16_t *)s);
			break;
		case DATA_TYPE_S32:
			*((float *)d) = *((int32_t *)s);
			break;
		case DATA_TYPE_S64:
			*((float *)d) = *((int64_t *)s);
			break;
		case DATA_TYPE_U8:
			*((float *)d) = *((uint8_t *)s);
			break;
		case DATA_TYPE_U16:
			*((float *)d) = *((uint16_t *)s);
			break;
		case DATA_TYPE_U32:
			*((float *)d) = *((uint32_t *)s);
			break;
		case DATA_TYPE_U64:
			*((float *)d) = *((uint64_t *)s);
			break;
		case DATA_TYPE_F32:
			*((float *)d) = *((float *)s);
			break;
		case DATA_TYPE_F64:
			*((float *)d) = *((double *)s);
			break;
		case DATA_TYPE_F128:
			*((float *)d) = *((long double *)s);
			break;
		case DATA_TYPE_BOOL:
			*((float *)d) = *((int *)s);
			break;
		case DATA_TYPE_S8_ARRAY:
		case DATA_TYPE_S16_ARRAY:
		case DATA_TYPE_S32_ARRAY:
		case DATA_TYPE_S64_ARRAY:
		case DATA_TYPE_U8_ARRAY:
		case DATA_TYPE_U16_ARRAY:
		case DATA_TYPE_U32_ARRAY:
		case DATA_TYPE_U64_ARRAY:
		case DATA_TYPE_F32_ARRAY:
		case DATA_TYPE_F64_ARRAY:
		case DATA_TYPE_STRING_ARRAY:
		case DATA_TYPE_STRING_LIST:
		default:
			dprintf(0,"**** conv dt: %04x(%s) unhandled st: %04x(%s)\n", dt, typestr(dt), st, typestr(st));
			break;
		}
	    }
	    break;
	case DATA_TYPE_F64:
	    {
		switch(st) {
		case DATA_TYPE_STRING:
			*((double *)d) = strtod(s,0);
			break;
		case DATA_TYPE_S8:
			*((double *)d) = *((int8_t *)s);
			break;
		case DATA_TYPE_S16:
			*((double *)d) = *((int16_t *)s);
			break;
		case DATA_TYPE_S32:
			*((double *)d) = *((int32_t *)s);
			break;
		case DATA_TYPE_S64:
			*((double *)d) = *((int64_t *)s);
			break;
		case DATA_TYPE_U8:
			*((double *)d) = *((uint8_t *)s);
			break;
		case DATA_TYPE_U16:
			*((double *)d) = *((uint16_t *)s);
			break;
		case DATA_TYPE_U32:
			*((double *)d) = *((uint32_t *)s);
			break;
		case DATA_TYPE_U64:
			*((double *)d) = *((uint64_t *)s);
			break;
		case DATA_TYPE_F32:
			*((double *)d) = *((float *)s);
			break;
		case DATA_TYPE_F64:
			*((double *)d) = *((double *)s);
			break;
		case DATA_TYPE_F128:
			*((double *)d) = *((long double *)s);
			break;
		case DATA_TYPE_BOOL:
			*((double *)d) = *((int *)s);
			break;
		case DATA_TYPE_S8_ARRAY:
		case DATA_TYPE_S16_ARRAY:
		case DATA_TYPE_S32_ARRAY:
		case DATA_TYPE_S64_ARRAY:
		case DATA_TYPE_U8_ARRAY:
		case DATA_TYPE_U16_ARRAY:
		case DATA_TYPE_U32_ARRAY:
		case DATA_TYPE_U64_ARRAY:
		case DATA_TYPE_F32_ARRAY:
		case DATA_TYPE_F64_ARRAY:
		case DATA_TYPE_STRING_ARRAY:
		case DATA_TYPE_STRING_LIST:
		default:
			dprintf(0,"**** conv dt: %04x(%s) unhandled st: %04x(%s)\n", dt, typestr(dt), st, typestr(st));
			break;
		}
	    }
	    break;
	case DATA_TYPE_BOOL:
	    {
		int *da = d;
		switch(st) {
		case DATA_TYPE_STRING:
		    {
			char ch, *src = s;
			register char *ptr;

			if (strcmp(src,"-1")==0 || strcasecmp(src,"null")==0) {
				*((int *)d) = -1;
			} else {
				for(ptr = src; *ptr && isspace(*ptr); ptr++);
				ch = toupper(*ptr);
				*((int *)d) = (ch == 'T' || ch == 'Y' || ch == '1' ? 1 : 0);
			}
		    }
		    break;
		case DATA_TYPE_S8:
			*((int *)d) = *((int8_t *)s);
			break;
		case DATA_TYPE_S16:
			*((int *)d) = *((int16_t *)s);
			break;
		case DATA_TYPE_S32:
			*((int *)d) = *((int32_t *)s);
			break;
		case DATA_TYPE_S64:
			*((int *)d) = *((int64_t *)s);
			break;
		case DATA_TYPE_U8:
			*((int *)d) = *((uint8_t *)s);
			break;
		case DATA_TYPE_U16:
			*((int *)d) = *((uint16_t *)s);
			break;
		case DATA_TYPE_U32:
			*((int *)d) = *((uint32_t *)s);
			break;
		case DATA_TYPE_U64:
			*((int *)d) = *((uint64_t *)s);
			break;
		case DATA_TYPE_F32:
			*((int *)d) = *((float *)s);
			break;
		case DATA_TYPE_F64:
			*((int *)d) = *((int *)s);
			break;
		case DATA_TYPE_F128:
			*((int *)d) = *((long int *)s);
			break;
		case DATA_TYPE_BOOL:
			*((int *)d) = *((int *)s);
			break;
		case DATA_TYPE_S8_ARRAY:
			{
				int8_t *sa = s;
				for(i=0; i < sl && i < dl; i++) da[i] = sa[i];
				return_len = dl;
			}
			break;
		case DATA_TYPE_S16_ARRAY:
			{
				int16_t *sa = s;
				for(i=0; i < sl && i < dl; i++) da[i] = sa[i];
				return_len = dl;
			}
			break;
		case DATA_TYPE_S32_ARRAY:
			{
				int32_t *sa = s;
				for(i=0; i < sl && i < dl; i++) da[i] = sa[i];
				return_len = dl;
			}
			break;
		case DATA_TYPE_S64_ARRAY:
			{
				int64_t *sa = s;
				for(i=0; i < sl && i < dl; i++) da[i] = sa[i];
				return_len = dl;
			}
			break;
		case DATA_TYPE_U8_ARRAY:
			{
				uint8_t *sa = s;
				for(i=0; i < sl && i < dl; i++) da[i] = sa[i];
				return_len = dl;
			}
			break;
		case DATA_TYPE_U16_ARRAY:
			{
				uint16_t *sa = s;
				for(i=0; i < sl && i < dl; i++) da[i] = sa[i];
				return_len = dl;
			}
			break;
		case DATA_TYPE_U32_ARRAY:
			{
				uint32_t *sa = s;
				for(i=0; i < sl && i < dl; i++) da[i] = sa[i];
				return_len = dl;
			}
			break;
		case DATA_TYPE_U64_ARRAY:
			{
				uint64_t *sa = s;
				for(i=0; i < sl && i < dl; i++) da[i] = sa[i];
				return_len = dl;
			}
			break;
		case DATA_TYPE_F32_ARRAY:
			{
				float *sa = s;
				for(i=0; i < sl && i < dl; i++) da[i] = sa[i];
				return_len = dl;
			}
			break;
		case DATA_TYPE_F64_ARRAY:
			{
				double *sa = s;
				for(i=0; i < sl && i < dl; i++) da[i] = sa[i];
				return_len = dl;
			}
			break;
		case DATA_TYPE_BOOL_ARRAY:
			{
				int *sa = s;
				for(i=0; i < sl && i < dl; i++) da[i] = sa[i];
				return_len = dl;
			}
			break;
		case DATA_TYPE_STRING_ARRAY:
		case DATA_TYPE_STRING_LIST:
		default:
			dprintf(0,"**** conv dt: %04x(%s) unhandled st: %04x(%s)\n", dt, typestr(dt), st, typestr(st));
			break;
		}
	    }
	    break;
	case DATA_TYPE_S8_ARRAY:
	    {
		int8_t *da = d;
		switch(st) {
		case DATA_TYPE_STRING:
		case DATA_TYPE_S8:
		case DATA_TYPE_S16:
		case DATA_TYPE_S32:
		case DATA_TYPE_S64:
		case DATA_TYPE_U8:
		case DATA_TYPE_U16:
		case DATA_TYPE_U32:
		case DATA_TYPE_U64:
		case DATA_TYPE_F32:
		case DATA_TYPE_F64:
		case DATA_TYPE_BOOL:
		case DATA_TYPE_S8_ARRAY:
			{
				int8_t *sa = s;
				for(i=0; i < sl && i < dl; i++) da[i] = sa[i];
				return_len = dl;
			}
			break;
		case DATA_TYPE_S16_ARRAY:
			{
				int16_t *sa = s;
				for(i=0; i < sl && i < dl; i++) da[i] = sa[i];
				return_len = dl;
			}
			break;
		case DATA_TYPE_S32_ARRAY:
			{
				int32_t *sa = s;
				for(i=0; i < sl && i < dl; i++) da[i] = sa[i];
				return_len = dl;
			}
			break;
		case DATA_TYPE_S64_ARRAY:
			{
				int64_t *sa = s;
				for(i=0; i < sl && i < dl; i++) da[i] = sa[i];
				return_len = dl;
			}
			break;
		case DATA_TYPE_U8_ARRAY:
			{
				uint8_t *sa = s;
				for(i=0; i < sl && i < dl; i++) da[i] = sa[i];
				return_len = dl;
			}
			break;
		case DATA_TYPE_U16_ARRAY:
			{
				uint16_t *sa = s;
				for(i=0; i < sl && i < dl; i++) da[i] = sa[i];
				return_len = dl;
			}
			break;
		case DATA_TYPE_U32_ARRAY:
			{
				uint32_t *sa = s;
				for(i=0; i < sl && i < dl; i++) da[i] = sa[i];
				return_len = dl;
			}
			break;
		case DATA_TYPE_U64_ARRAY:
			{
				uint64_t *sa = s;
				for(i=0; i < sl && i < dl; i++) da[i] = sa[i];
				return_len = dl;
			}
			break;
		case DATA_TYPE_F32_ARRAY:
			{
				float *sa = s;
				for(i=0; i < sl && i < dl; i++) da[i] = sa[i];
				return_len = dl;
			}
			break;
		case DATA_TYPE_F64_ARRAY:
			{
				double *sa = s;
				for(i=0; i < sl && i < dl; i++) da[i] = sa[i];
				return_len = dl;
			}
			break;
		case DATA_TYPE_BOOL_ARRAY:
			{
				int *sa = s;
				for(i=0; i < sl && i < dl; i++) da[i] = sa[i];
				return_len = dl;
			}
			break;
		case DATA_TYPE_STRING_ARRAY:
		case DATA_TYPE_STRING_LIST:
		default:
			dprintf(0,"**** conv dt: %04x(%s) unhandled st: %04x(%s)\n", dt, typestr(dt), st, typestr(st));
			break;
		}
	    }
	    break;
	case DATA_TYPE_S16_ARRAY:
	    {
		int16_t *da = d;
		switch(st) {
		case DATA_TYPE_STRING:
		case DATA_TYPE_S8:
		case DATA_TYPE_S16:
		case DATA_TYPE_S32:
		case DATA_TYPE_S64:
		case DATA_TYPE_U8:
		case DATA_TYPE_U16:
		case DATA_TYPE_U32:
		case DATA_TYPE_U64:
		case DATA_TYPE_F32:
		case DATA_TYPE_F64:
		case DATA_TYPE_BOOL:
		case DATA_TYPE_S8_ARRAY:
			{
				int8_t *sa = s;
				for(i=0; i < sl && i < dl; i++) da[i] = sa[i];
				return_len = dl;
			}
			break;
		case DATA_TYPE_S16_ARRAY:
			{
				int16_t *sa = s;
				for(i=0; i < sl && i < dl; i++) da[i] = sa[i];
				return_len = dl;
			}
			break;
		case DATA_TYPE_S32_ARRAY:
			{
				int32_t *sa = s;
				for(i=0; i < sl && i < dl; i++) da[i] = sa[i];
				return_len = dl;
			}
			break;
		case DATA_TYPE_S64_ARRAY:
			{
				int64_t *sa = s;
				for(i=0; i < sl && i < dl; i++) da[i] = sa[i];
				return_len = dl;
			}
			break;
		case DATA_TYPE_U8_ARRAY:
			{
				uint8_t *sa = s;
				for(i=0; i < sl && i < dl; i++) da[i] = sa[i];
				return_len = dl;
			}
			break;
		case DATA_TYPE_U16_ARRAY:
			{
				uint16_t *sa = s;
				for(i=0; i < sl && i < dl; i++) da[i] = sa[i];
				return_len = dl;
			}
			break;
		case DATA_TYPE_U32_ARRAY:
			{
				uint32_t *sa = s;
				for(i=0; i < sl && i < dl; i++) da[i] = sa[i];
				return_len = dl;
			}
			break;
		case DATA_TYPE_U64_ARRAY:
			{
				uint64_t *sa = s;
				for(i=0; i < sl && i < dl; i++) da[i] = sa[i];
				return_len = dl;
			}
			break;
		case DATA_TYPE_F32_ARRAY:
			{
				float *sa = s;
				for(i=0; i < sl && i < dl; i++) da[i] = sa[i];
				return_len = dl;
			}
			break;
		case DATA_TYPE_F64_ARRAY:
			{
				double *sa = s;
				for(i=0; i < sl && i < dl; i++) da[i] = sa[i];
				return_len = dl;
			}
			break;
		case DATA_TYPE_BOOL_ARRAY:
			{
				int *sa = s;
				for(i=0; i < sl && i < dl; i++) da[i] = sa[i];
				return_len = dl;
			}
			break;
		case DATA_TYPE_STRING_ARRAY:
		case DATA_TYPE_STRING_LIST:
		default:
			dprintf(0,"**** conv dt: %04x(%s) unhandled st: %04x(%s)\n", dt, typestr(dt), st, typestr(st));
			break;
		}
	    }
	    break;
	case DATA_TYPE_S32_ARRAY:
	    {
		int32_t *da = d;
		switch(st) {
		case DATA_TYPE_STRING:
		case DATA_TYPE_S8:
		case DATA_TYPE_S16:
		case DATA_TYPE_S32:
		case DATA_TYPE_S64:
		case DATA_TYPE_U8:
		case DATA_TYPE_U16:
		case DATA_TYPE_U32:
		case DATA_TYPE_U64:
		case DATA_TYPE_F32:
		case DATA_TYPE_F64:
		case DATA_TYPE_BOOL:
		case DATA_TYPE_S8_ARRAY:
			{
				int8_t *sa = s;
				for(i=0; i < sl && i < dl; i++) da[i] = sa[i];
				return_len = dl;
			}
			break;
		case DATA_TYPE_S16_ARRAY:
			{
				int16_t *sa = s;
				for(i=0; i < sl && i < dl; i++) da[i] = sa[i];
				return_len = dl;
			}
			break;
		case DATA_TYPE_S32_ARRAY:
			{
				int32_t *sa = s;
				for(i=0; i < sl && i < dl; i++) da[i] = sa[i];
				return_len = dl;
			}
			break;
		case DATA_TYPE_S64_ARRAY:
			{
				int64_t *sa = s;
				for(i=0; i < sl && i < dl; i++) da[i] = sa[i];
				return_len = dl;
			}
			break;
		case DATA_TYPE_U8_ARRAY:
			{
				uint8_t *sa = s;
				for(i=0; i < sl && i < dl; i++) da[i] = sa[i];
				return_len = dl;
			}
			break;
		case DATA_TYPE_U16_ARRAY:
			{
				uint16_t *sa = s;
				for(i=0; i < sl && i < dl; i++) da[i] = sa[i];
				return_len = dl;
			}
			break;
		case DATA_TYPE_U32_ARRAY:
			{
				uint32_t *sa = s;
				for(i=0; i < sl && i < dl; i++) da[i] = sa[i];
				return_len = dl;
			}
			break;
		case DATA_TYPE_U64_ARRAY:
			{
				uint64_t *sa = s;
				for(i=0; i < sl && i < dl; i++) da[i] = sa[i];
				return_len = dl;
			}
			break;
		case DATA_TYPE_F32_ARRAY:
			{
				float *sa = s;
				for(i=0; i < sl && i < dl; i++) da[i] = sa[i];
				return_len = dl;
			}
			break;
		case DATA_TYPE_F64_ARRAY:
			{
				double *sa = s;
				for(i=0; i < sl && i < dl; i++) da[i] = sa[i];
				return_len = dl;
			}
			break;
		case DATA_TYPE_BOOL_ARRAY:
			{
				int *sa = s;
				for(i=0; i < sl && i < dl; i++) da[i] = sa[i];
				return_len = dl;
			}
			break;
		case DATA_TYPE_STRING_ARRAY:
		case DATA_TYPE_STRING_LIST:
		default:
			dprintf(0,"**** conv dt: %04x(%s) unhandled st: %04x(%s)\n", dt, typestr(dt), st, typestr(st));
			break;
		}
	    }
	    break;
	case DATA_TYPE_S64_ARRAY:
	    {
		int64_t *da = d;
		switch(st) {
		case DATA_TYPE_STRING:
		case DATA_TYPE_S8:
		case DATA_TYPE_S16:
		case DATA_TYPE_S32:
		case DATA_TYPE_S64:
		case DATA_TYPE_U8:
		case DATA_TYPE_U16:
		case DATA_TYPE_U32:
		case DATA_TYPE_U64:
		case DATA_TYPE_F32:
		case DATA_TYPE_F64:
		case DATA_TYPE_BOOL:
		case DATA_TYPE_S8_ARRAY:
			{
				int8_t *sa = s;
				for(i=0; i < sl && i < dl; i++) da[i] = sa[i];
				return_len = dl;
			}
			break;
		case DATA_TYPE_S16_ARRAY:
			{
				int16_t *sa = s;
				for(i=0; i < sl && i < dl; i++) da[i] = sa[i];
				return_len = dl;
			}
			break;
		case DATA_TYPE_S32_ARRAY:
			{
				int32_t *sa = s;
				for(i=0; i < sl && i < dl; i++) da[i] = sa[i];
				return_len = dl;
			}
			break;
		case DATA_TYPE_S64_ARRAY:
			{
				int64_t *sa = s;
				for(i=0; i < sl && i < dl; i++) da[i] = sa[i];
				return_len = dl;
			}
			break;
		case DATA_TYPE_U8_ARRAY:
			{
				uint8_t *sa = s;
				for(i=0; i < sl && i < dl; i++) da[i] = sa[i];
				return_len = dl;
			}
			break;
		case DATA_TYPE_U16_ARRAY:
			{
				uint16_t *sa = s;
				for(i=0; i < sl && i < dl; i++) da[i] = sa[i];
				return_len = dl;
			}
			break;
		case DATA_TYPE_U32_ARRAY:
			{
				uint32_t *sa = s;
				for(i=0; i < sl && i < dl; i++) da[i] = sa[i];
				return_len = dl;
			}
			break;
		case DATA_TYPE_U64_ARRAY:
			{
				uint64_t *sa = s;
				for(i=0; i < sl && i < dl; i++) da[i] = sa[i];
				return_len = dl;
			}
			break;
		case DATA_TYPE_F32_ARRAY:
			{
				float *sa = s;
				for(i=0; i < sl && i < dl; i++) da[i] = sa[i];
				return_len = dl;
			}
			break;
		case DATA_TYPE_F64_ARRAY:
			{
				double *sa = s;
				for(i=0; i < sl && i < dl; i++) da[i] = sa[i];
				return_len = dl;
			}
			break;
		case DATA_TYPE_BOOL_ARRAY:
			{
				int *sa = s;
				for(i=0; i < sl && i < dl; i++) da[i] = sa[i];
				return_len = dl;
			}
			break;
		case DATA_TYPE_STRING_ARRAY:
		case DATA_TYPE_STRING_LIST:
		default:
			dprintf(0,"**** conv dt: %04x(%s) unhandled st: %04x(%s)\n", dt, typestr(dt), st, typestr(st));
			break;
		}
	    }
	    break;
	case DATA_TYPE_U8_ARRAY:
	    {
		uint8_t *da = d;
		switch(st) {
		case DATA_TYPE_STRING_ARRAY:
		    {
			uint8_t *u8 = (uint8_t *)d;
			char **sa = (char **)s;
			int count;

//			dprintf(1,"******* dl: %d, sl: %d\n", dl, sl);
			count = 0;
			for(i=0; i < dl && i < sl; i++) {
//				dprintf(1,"sa[%d]: %s\n", i, sa[i]);
				u8[i] = (unsigned char) strtol(sa[i],0,0);
//				dprintf(1,"u8[%d]: %02x\n", i, u8[i]);
				count++;
			}
			return_len = count;
		    }
		    break;
		case DATA_TYPE_STRING:
		case DATA_TYPE_S8:
		case DATA_TYPE_S16:
		case DATA_TYPE_S32:
		case DATA_TYPE_S64:
		case DATA_TYPE_U8:
		case DATA_TYPE_U16:
		case DATA_TYPE_U32:
		case DATA_TYPE_U64:
		case DATA_TYPE_F32:
		case DATA_TYPE_F64:
		case DATA_TYPE_BOOL:
		case DATA_TYPE_S8_ARRAY:
			{
				int8_t *sa = s;
				for(i=0; i < sl && i < dl; i++) da[i] = sa[i];
				return_len = dl;
			}
			break;
		case DATA_TYPE_S16_ARRAY:
			{
				int16_t *sa = s;
				for(i=0; i < sl && i < dl; i++) da[i] = sa[i];
				return_len = dl;
			}
			break;
		case DATA_TYPE_S32_ARRAY:
			{
				int32_t *sa = s;
				for(i=0; i < sl && i < dl; i++) da[i] = sa[i];
				return_len = dl;
			}
			break;
		case DATA_TYPE_S64_ARRAY:
			{
				int64_t *sa = s;
				for(i=0; i < sl && i < dl; i++) da[i] = sa[i];
				return_len = dl;
			}
			break;
		case DATA_TYPE_U8_ARRAY:
			{
				uint8_t *sa = s;
				for(i=0; i < sl && i < dl; i++) da[i] = sa[i];
				return_len = dl;
			}
			break;
		case DATA_TYPE_U16_ARRAY:
			{
				uint16_t *sa = s;
				for(i=0; i < sl && i < dl; i++) da[i] = sa[i];
				return_len = dl;
			}
			break;
		case DATA_TYPE_U32_ARRAY:
			{
				uint32_t *sa = s;
				for(i=0; i < sl && i < dl; i++) da[i] = sa[i];
				return_len = dl;
			}
			break;
		case DATA_TYPE_U64_ARRAY:
			{
				uint64_t *sa = s;
				for(i=0; i < sl && i < dl; i++) da[i] = sa[i];
				return_len = dl;
			}
			break;
		case DATA_TYPE_F32_ARRAY:
			{
				float *sa = s;
				for(i=0; i < sl && i < dl; i++) da[i] = sa[i];
				return_len = dl;
			}
			break;
		case DATA_TYPE_F64_ARRAY:
			{
				double *sa = s;
				for(i=0; i < sl && i < dl; i++) da[i] = sa[i];
				return_len = dl;
			}
			break;
		case DATA_TYPE_BOOL_ARRAY:
			{
				int *sa = s;
				for(i=0; i < sl && i < dl; i++) da[i] = sa[i];
				return_len = dl;
			}
			break;
		case DATA_TYPE_STRING_LIST:
		default:
			dprintf(0,"**** conv dt: %04x(%s) unhandled st: %04x(%s)\n", dt, typestr(dt), st, typestr(st));
			break;
		}
	    }
	    break;
	case DATA_TYPE_U16_ARRAY:
	    {
		uint16_t *da = d;
		switch(st) {
		case DATA_TYPE_STRING:
		case DATA_TYPE_S8:
		case DATA_TYPE_S16:
		case DATA_TYPE_S32:
		case DATA_TYPE_S64:
		case DATA_TYPE_U8:
		case DATA_TYPE_U16:
		case DATA_TYPE_U32:
		case DATA_TYPE_U64:
		case DATA_TYPE_F32:
		case DATA_TYPE_F64:
		case DATA_TYPE_BOOL:
		case DATA_TYPE_S8_ARRAY:
			{
				int8_t *sa = s;
				for(i=0; i < sl && i < dl; i++) da[i] = sa[i];
				return_len = dl;
			}
			break;
		case DATA_TYPE_S16_ARRAY:
			{
				int16_t *sa = s;
				for(i=0; i < sl && i < dl; i++) da[i] = sa[i];
				return_len = dl;
			}
			break;
		case DATA_TYPE_S32_ARRAY:
			{
				int32_t *sa = s;
				for(i=0; i < sl && i < dl; i++) da[i] = sa[i];
				return_len = dl;
			}
			break;
		case DATA_TYPE_S64_ARRAY:
			{
				int64_t *sa = s;
				for(i=0; i < sl && i < dl; i++) da[i] = sa[i];
				return_len = dl;
			}
			break;
		case DATA_TYPE_U8_ARRAY:
			{
				uint8_t *sa = s;
				for(i=0; i < sl && i < dl; i++) da[i] = sa[i];
				return_len = dl;
			}
			break;
		case DATA_TYPE_U16_ARRAY:
			{
				uint16_t *sa = s;
				for(i=0; i < sl && i < dl; i++) da[i] = sa[i];
				return_len = dl;
			}
			break;
		case DATA_TYPE_U32_ARRAY:
			{
				uint32_t *sa = s;
				for(i=0; i < sl && i < dl; i++) da[i] = sa[i];
				return_len = dl;
			}
			break;
		case DATA_TYPE_U64_ARRAY:
			{
				uint64_t *sa = s;
				for(i=0; i < sl && i < dl; i++) da[i] = sa[i];
				return_len = dl;
			}
			break;
		case DATA_TYPE_F32_ARRAY:
			{
				float *sa = s;
				for(i=0; i < sl && i < dl; i++) da[i] = sa[i];
				return_len = dl;
			}
			break;
		case DATA_TYPE_F64_ARRAY:
			{
				double *sa = s;
				for(i=0; i < sl && i < dl; i++) da[i] = sa[i];
				return_len = dl;
			}
			break;
		case DATA_TYPE_BOOL_ARRAY:
			{
				int *sa = s;
				for(i=0; i < sl && i < dl; i++) da[i] = sa[i];
				return_len = dl;
			}
			break;
		case DATA_TYPE_STRING_ARRAY:
		case DATA_TYPE_STRING_LIST:
		default:
			dprintf(0,"**** conv dt: %04x(%s) unhandled st: %04x(%s)\n", dt, typestr(dt), st, typestr(st));
			break;
		}
	    }
	    break;
	case DATA_TYPE_U32_ARRAY:
	    {
		uint32_t *da = d;
		switch(st) {
		case DATA_TYPE_STRING:
		case DATA_TYPE_S8:
		case DATA_TYPE_S16:
		case DATA_TYPE_S32:
		case DATA_TYPE_S64:
		case DATA_TYPE_U8:
		case DATA_TYPE_U16:
		case DATA_TYPE_U32:
		case DATA_TYPE_U64:
		case DATA_TYPE_F32:
		case DATA_TYPE_F64:
		case DATA_TYPE_BOOL:
		case DATA_TYPE_S8_ARRAY:
			{
				int8_t *sa = s;
				for(i=0; i < sl && i < dl; i++) da[i] = sa[i];
				return_len = dl;
			}
			break;
		case DATA_TYPE_S16_ARRAY:
			{
				int16_t *sa = s;
				for(i=0; i < sl && i < dl; i++) da[i] = sa[i];
				return_len = dl;
			}
			break;
		case DATA_TYPE_S32_ARRAY:
			{
				int32_t *sa = s;
				for(i=0; i < sl && i < dl; i++) da[i] = sa[i];
				return_len = dl;
			}
			break;
		case DATA_TYPE_S64_ARRAY:
			{
				int64_t *sa = s;
				for(i=0; i < sl && i < dl; i++) da[i] = sa[i];
				return_len = dl;
			}
			break;
		case DATA_TYPE_U8_ARRAY:
			{
				uint8_t *sa = s;
				for(i=0; i < sl && i < dl; i++) da[i] = sa[i];
				return_len = dl;
			}
			break;
		case DATA_TYPE_U16_ARRAY:
			{
				uint16_t *sa = s;
				for(i=0; i < sl && i < dl; i++) da[i] = sa[i];
				return_len = dl;
			}
			break;
		case DATA_TYPE_U32_ARRAY:
			{
				uint32_t *sa = s;
				for(i=0; i < sl && i < dl; i++) da[i] = sa[i];
				return_len = dl;
			}
			break;
		case DATA_TYPE_U64_ARRAY:
			{
				uint64_t *sa = s;
				for(i=0; i < sl && i < dl; i++) da[i] = sa[i];
				return_len = dl;
			}
			break;
		case DATA_TYPE_F32_ARRAY:
			{
				float *sa = s;
				for(i=0; i < sl && i < dl; i++) da[i] = sa[i];
				return_len = dl;
			}
			break;
		case DATA_TYPE_F64_ARRAY:
			{
				double *sa = s;
				for(i=0; i < sl && i < dl; i++) da[i] = sa[i];
				return_len = dl;
			}
			break;
		case DATA_TYPE_BOOL_ARRAY:
			{
				int *sa = s;
				for(i=0; i < sl && i < dl; i++) da[i] = sa[i];
				return_len = dl;
			}
			break;
		case DATA_TYPE_STRING_ARRAY:
		case DATA_TYPE_STRING_LIST:
		default:
			dprintf(0,"**** conv dt: %04x(%s) unhandled st: %04x(%s)\n", dt, typestr(dt), st, typestr(st));
			break;
		}
	    }
	    break;
	case DATA_TYPE_U64_ARRAY:
	    {
		uint64_t *da = d;
		switch(st) {
		case DATA_TYPE_STRING:
		case DATA_TYPE_S8:
		case DATA_TYPE_S16:
		case DATA_TYPE_S32:
		case DATA_TYPE_S64:
		case DATA_TYPE_U8:
		case DATA_TYPE_U16:
		case DATA_TYPE_U32:
		case DATA_TYPE_U64:
		case DATA_TYPE_F32:
		case DATA_TYPE_F64:
		case DATA_TYPE_BOOL:
		case DATA_TYPE_S8_ARRAY:
			{
				int8_t *sa = s;
				for(i=0; i < sl && i < dl; i++) da[i] = sa[i];
				return_len = dl;
			}
			break;
		case DATA_TYPE_S16_ARRAY:
			{
				int16_t *sa = s;
				for(i=0; i < sl && i < dl; i++) da[i] = sa[i];
				return_len = dl;
			}
			break;
		case DATA_TYPE_S32_ARRAY:
			{
				int32_t *sa = s;
				for(i=0; i < sl && i < dl; i++) da[i] = sa[i];
				return_len = dl;
			}
			break;
		case DATA_TYPE_S64_ARRAY:
			{
				int64_t *sa = s;
				for(i=0; i < sl && i < dl; i++) da[i] = sa[i];
				return_len = dl;
			}
			break;
		case DATA_TYPE_U8_ARRAY:
			{
				uint8_t *sa = s;
				for(i=0; i < sl && i < dl; i++) da[i] = sa[i];
				return_len = dl;
			}
			break;
		case DATA_TYPE_U16_ARRAY:
			{
				uint16_t *sa = s;
				for(i=0; i < sl && i < dl; i++) da[i] = sa[i];
				return_len = dl;
			}
			break;
		case DATA_TYPE_U32_ARRAY:
			{
				uint32_t *sa = s;
				for(i=0; i < sl && i < dl; i++) da[i] = sa[i];
				return_len = dl;
			}
			break;
		case DATA_TYPE_U64_ARRAY:
			{
				uint64_t *sa = s;
				for(i=0; i < sl && i < dl; i++) da[i] = sa[i];
				return_len = dl;
			}
			break;
		case DATA_TYPE_F32_ARRAY:
			{
				float *sa = s;
				for(i=0; i < sl && i < dl; i++) da[i] = sa[i];
				return_len = dl;
			}
			break;
		case DATA_TYPE_F64_ARRAY:
			{
				double *sa = s;
				for(i=0; i < sl && i < dl; i++) da[i] = sa[i];
				return_len = dl;
			}
			break;
		case DATA_TYPE_BOOL_ARRAY:
			{
				int *sa = s;
				for(i=0; i < sl && i < dl; i++) da[i] = sa[i];
				return_len = dl;
			}
			break;
		case DATA_TYPE_STRING_ARRAY:
		case DATA_TYPE_STRING_LIST:
		default:
			dprintf(0,"**** conv dt: %04x(%s) unhandled st: %04x(%s)\n", dt, typestr(dt), st, typestr(st));
			break;
		}
	    }
	    break;
	case DATA_TYPE_F32_ARRAY:
	    {
		float *da = d;
		switch(st) {
		case DATA_TYPE_STRING:
		case DATA_TYPE_S8:
		case DATA_TYPE_S16:
		case DATA_TYPE_S32:
		case DATA_TYPE_S64:
		case DATA_TYPE_U8:
		case DATA_TYPE_U16:
		case DATA_TYPE_U32:
		case DATA_TYPE_U64:
		case DATA_TYPE_F32:
		case DATA_TYPE_F64:
		case DATA_TYPE_BOOL:
		case DATA_TYPE_S8_ARRAY:
			{
				int8_t *sa = s;
				for(i=0; i < sl && i < dl; i++) da[i] = sa[i];
				return_len = dl;
			}
			break;
		case DATA_TYPE_S16_ARRAY:
			{
				int16_t *sa = s;
				for(i=0; i < sl && i < dl; i++) da[i] = sa[i];
				return_len = dl;
			}
			break;
		case DATA_TYPE_S32_ARRAY:
			{
				int32_t *sa = s;
				for(i=0; i < sl && i < dl; i++) da[i] = sa[i];
				return_len = dl;
			}
			break;
		case DATA_TYPE_S64_ARRAY:
			{
				int64_t *sa = s;
				for(i=0; i < sl && i < dl; i++) da[i] = sa[i];
				return_len = dl;
			}
			break;
		case DATA_TYPE_U8_ARRAY:
			{
				uint8_t *sa = s;
				for(i=0; i < sl && i < dl; i++) da[i] = sa[i];
				return_len = dl;
			}
			break;
		case DATA_TYPE_U16_ARRAY:
			{
				uint16_t *sa = s;
				for(i=0; i < sl && i < dl; i++) da[i] = sa[i];
				return_len = dl;
			}
			break;
		case DATA_TYPE_U32_ARRAY:
			{
				uint32_t *sa = s;
				for(i=0; i < sl && i < dl; i++) da[i] = sa[i];
				return_len = dl;
			}
			break;
		case DATA_TYPE_U64_ARRAY:
			{
				uint64_t *sa = s;
				for(i=0; i < sl && i < dl; i++) da[i] = sa[i];
				return_len = dl;
			}
			break;
		case DATA_TYPE_F32_ARRAY:
			{
				float *sa = s;
				for(i=0; i < sl && i < dl; i++) da[i] = sa[i];
				return_len = dl;
			}
			break;
		case DATA_TYPE_F64_ARRAY:
			{
				double *sa = s;
				for(i=0; i < sl && i < dl; i++) da[i] = sa[i];
				return_len = dl;
			}
			break;
		case DATA_TYPE_BOOL_ARRAY:
			{
				int *sa = s;
				for(i=0; i < sl && i < dl; i++) da[i] = sa[i];
				return_len = dl;
			}
			break;
		case DATA_TYPE_STRING_ARRAY:
		case DATA_TYPE_STRING_LIST:
		default:
			dprintf(0,"**** conv dt: %04x(%s) unhandled st: %04x(%s)\n", dt, typestr(dt), st, typestr(st));
			break;
		}
	    }
	    break;
	case DATA_TYPE_F64_ARRAY:
	    {
		double *da = d;

		switch(st) {
		case DATA_TYPE_STRING:
		    {
			list l;
			char *p;

			str2list(&l,s);
			i = 0;
			list_reset(l);
			while((p = list_get_next(l)) != 0) {
				if (i >= dl) break;
				da[i++] = strtod(p,0);
			}
			list_destroy(l);
			return_len = i;
		    }
		    break;
		case DATA_TYPE_S8:
			da[0] = *((int8_t *)s);
			return_len = 1;
			break;
		case DATA_TYPE_S16:
			da[0] = *((int16_t *)s);
			return_len = 1;
			break;
		case DATA_TYPE_S32:
			da[0] = *((int32_t *)s);
			return_len = 1;
			break;
		case DATA_TYPE_S64:
			da[0] = *((int64_t *)s);
			return_len = 1;
			break;
		case DATA_TYPE_U8:
			da[0] = *((uint8_t *)s);
			return_len = 1;
			break;
		case DATA_TYPE_U16:
			da[0] = *((uint16_t *)s);
			return_len = 1;
			break;
		case DATA_TYPE_U32:
			da[0] = *((uint32_t *)s);
			return_len = 1;
			break;
		case DATA_TYPE_U64:
			da[0] = *((uint64_t *)s);
			return_len = 1;
			break;
		case DATA_TYPE_F32:
			da[0] = *((float *)s);
			return_len = 1;
			break;
		case DATA_TYPE_F64:
			da[0] = *((double *)s);
			return_len = 1;
			break;
		case DATA_TYPE_BOOL:
			da[0] = *((int *)s);
			return_len = 1;
			break;
		case DATA_TYPE_S8_ARRAY:
			{
				int8_t *sa = s;
				for(i=0; i < sl && i < dl; i++) da[i] = sa[i];
				return_len = dl;
			}
			break;
		case DATA_TYPE_S16_ARRAY:
			{
				int16_t *sa = s;
				for(i=0; i < sl && i < dl; i++) da[i] = sa[i];
				return_len = dl;
			}
			break;
		case DATA_TYPE_S32_ARRAY:
			{
				int32_t *sa = s;
				for(i=0; i < sl && i < dl; i++) da[i] = sa[i];
				return_len = dl;
			}
			break;
		case DATA_TYPE_S64_ARRAY:
			{
				int64_t *sa = s;
				for(i=0; i < sl && i < dl; i++) da[i] = sa[i];
				return_len = dl;
			}
			break;
		case DATA_TYPE_U8_ARRAY:
			{
				uint8_t *sa = s;
				for(i=0; i < sl && i < dl; i++) da[i] = sa[i];
				return_len = dl;
			}
			break;
		case DATA_TYPE_U16_ARRAY:
			{
				uint16_t *sa = s;
				for(i=0; i < sl && i < dl; i++) da[i] = sa[i];
				return_len = dl;
			}
			break;
		case DATA_TYPE_U32_ARRAY:
			{
				uint32_t *sa = s;
				for(i=0; i < sl && i < dl; i++) da[i] = sa[i];
				return_len = dl;
			}
			break;
		case DATA_TYPE_U64_ARRAY:
			{
				uint64_t *sa = s;
				for(i=0; i < sl && i < dl; i++) da[i] = sa[i];
				return_len = dl;
			}
			break;
		case DATA_TYPE_F32_ARRAY:
			{
				float *sa = s;
				for(i=0; i < sl && i < dl; i++) da[i] = sa[i];
				return_len = dl;
			}
			break;
		case DATA_TYPE_F64_ARRAY:
			{
				double *sa = s;
				for(i=0; i < sl && i < dl; i++) da[i] = sa[i];
				return_len = dl;
			}
			break;
		case DATA_TYPE_BOOL_ARRAY:
			{
				int *sa = s;
				for(i=0; i < sl && i < dl; i++) da[i] = sa[i];
				return_len = dl;
			}
			break;
		case DATA_TYPE_STRING_ARRAY:
		case DATA_TYPE_STRING_LIST:
		default:
			dprintf(0,"**** conv dt: %04x(%s) unhandled st: %04x(%s)\n", dt, typestr(dt), st, typestr(st));
			break;
		}
	    }
	    break;
	case DATA_TYPE_STRING_ARRAY:
	    {
		switch(st) {
		case DATA_TYPE_STRING:
		case DATA_TYPE_S8:
		case DATA_TYPE_S16:
		case DATA_TYPE_S32:
		case DATA_TYPE_S64:
		case DATA_TYPE_U8:
		case DATA_TYPE_U16:
		case DATA_TYPE_U32:
		case DATA_TYPE_U64:
		case DATA_TYPE_F32:
		case DATA_TYPE_F64:
		case DATA_TYPE_BOOL:
		case DATA_TYPE_S8_ARRAY:
		case DATA_TYPE_S16_ARRAY:
		case DATA_TYPE_S32_ARRAY:
		case DATA_TYPE_S64_ARRAY:
		case DATA_TYPE_U8_ARRAY:
		case DATA_TYPE_U16_ARRAY:
		case DATA_TYPE_U32_ARRAY:
		case DATA_TYPE_U64_ARRAY:
		case DATA_TYPE_F32_ARRAY:
		case DATA_TYPE_F64_ARRAY:
		case DATA_TYPE_STRING_ARRAY:
		case DATA_TYPE_STRING_LIST:
		default:
			dprintf(0,"**** conv dt: %04x(%s) unhandled st: %04x(%s)\n", dt, typestr(dt), st, typestr(st));
			break;
		}
	    }
	    break;
	case DATA_TYPE_STRING_LIST:
	    {
		switch(st) {
		case DATA_TYPE_STRING:
		    {
			char temp[2048];
			int tmax,tlen,q;
			list *dest = d;
			register char *p;
			register int i;

			*dest = list_create();
			if (!s) break;

			p = s;
//			printf("==> src: %s\n", p);
			tmax = sizeof(temp)-1;
			i = q = 0;
			while(*p) {
//				printf("==> p: %c, q: %d, i: %d\n", *p, q, i);
				if (*p == '\"' && !q)
					q = 1;
				else if (*p == '\"' && q)
					q = 0;
				else if ((*p == ',' && !q) || i >= tmax) {
					temp[i] = 0;
					trim(temp);
					tlen = strlen(temp);
//					printf("==> adding: %s (%d)\n", temp, tlen);
					if (tlen) list_add(*dest,temp,tlen+1);
					i = 0;
				} else {
					temp[i++] = *p;
				}
				p++;
			}
			if (i) {
				temp[i] = 0;
				trim(temp);
				tlen = strlen(temp);
//				dprintf(dlevel,"adding: %s (%d)\n", temp, tlen);
				if (tlen) list_add(*dest,temp,tlen+1);
			}
#if 0
#if DEBUG
			dprintf(dlevel,"list:");
			list_reset(*dest);
			while( (p = list_get_next(*dest)) != 0) dprintf(dlevel,"\t%s",p);
#endif
#endif
		    }
		    break;
		case DATA_TYPE_S8:
		case DATA_TYPE_S16:
		case DATA_TYPE_S32:
		case DATA_TYPE_S64:
		case DATA_TYPE_U8:
		case DATA_TYPE_U16:
		case DATA_TYPE_U32:
		case DATA_TYPE_U64:
		case DATA_TYPE_F32:
		case DATA_TYPE_F64:
		case DATA_TYPE_BOOL:
		case DATA_TYPE_S8_ARRAY:
		case DATA_TYPE_S16_ARRAY:
		case DATA_TYPE_S32_ARRAY:
		case DATA_TYPE_S64_ARRAY:
		case DATA_TYPE_U8_ARRAY:
		case DATA_TYPE_U16_ARRAY:
		case DATA_TYPE_U32_ARRAY:
		case DATA_TYPE_U64_ARRAY:
		case DATA_TYPE_F32_ARRAY:
		case DATA_TYPE_F64_ARRAY:
		case DATA_TYPE_STRING_ARRAY:
		case DATA_TYPE_STRING_LIST:
		default:
			dprintf(0,"**** conv dt: %04x(%s) unhandled st: %04x(%s)\n", dt, typestr(dt), st, typestr(st));
			break;
		}
	    }
	    break;
	}

	if (return_len < 0) {
		switch(dt) {
		case DATA_TYPE_STRING:
			return strlen((char *)d);
		case DATA_TYPE_S8:
			return sizeof(byte);
		case DATA_TYPE_S16:
			return sizeof(short);
		case DATA_TYPE_S32:
			return sizeof(int);
		case DATA_TYPE_S64:
			return sizeof(long long);
		case DATA_TYPE_F32:
			return sizeof(float);
		case DATA_TYPE_F64:
			return sizeof(double);
		case DATA_TYPE_BOOL:
			return sizeof(int);
		case DATA_TYPE_STRING_LIST:
			return list_count(*((list *)d));
		default:
			return 0;
		}
	}
	return return_len;
}

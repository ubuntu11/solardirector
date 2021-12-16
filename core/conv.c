
/*
Copyright (c) 2021, Stephen P. Shoecraft
All rights reserved.

This source code is licensed under the BSD-style license found in the
LICENSE file in the root directory of this source tree.
*/

#define DEBUG_CONV 0
#define DLEVEL 4

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <strings.h>
#include <ctype.h>
#include "utils.h"

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

typedef void conv_func_t(char *,int,char *,int);
static long long mystrtoll(const char *nptr, char **endptr, int base) {
#ifdef __WIN32
	return atol(nptr);
#else
	return strtoll(nptr,endptr,base);
#endif
}

/*
 * DATA_TYPE_BYTE conversion functions
*/

void byte2chr(char *dest,int dlen,byte *src,int slen) {
	register int x;

	sprintf(dest,"%d",*src);
	x = strlen(dest);
	while(x < dlen) dest[x++] = ' ';
	return;
}

void byte2str(char *dest,int dlen,byte *src,int slen) {
	sprintf(dest,"%d",*src);
	return;
}

void byte2byte(byte *dest,int dlen,byte *src,int slen) {
	*dest = *src;
	return;
}

void byte2short(short *dest,int dlen,byte *src,int slen) {
	*dest = *src;
	return;
}

void byte2int(int *dest,int dlen,byte *src,int slen) {
	*dest = *src;
	return;
}

void byte2long(long *dest,int dlen,byte *src,int slen) {
	*dest = *src;
	return;
}

void byte2quad(myquad_t *dest,int dlen,byte *src,int slen) {
	*dest = *src;
	return;
}

void byte2float(float *dest,int dlen,byte *src,int slen) {
	*dest = *src;
	return;
}

void byte2dbl(double *dest,int dlen,byte *src,int slen) {
	*dest = *src;
	return;
}

void byte2log(int *dest,int dlen,byte *src,int slen) {
	*dest = (*src == 0 ? 0 : 1);
	return;
}

void byte2date(char *dest,int dlen,byte *src,int slen) {
	*dest = 0;
	return;
}

void byte2list(list *dest,int dlen,byte *src,int slen) {
	if (!*dest) *dest = list_create();
	if (*dest) {
		int x,len;
		len = (slen ? slen : 1);
		for(x=0; x < len; x++) list_add(*dest,(void *)&src[x],sizeof(*src));
	}
	return;
}


/*
 * DATA_TYPE_CHAR conversion functions
*/

void chr2chr(char *dest,int dlen,char *src,int slen) {
	register char *dptr = dest;
	char *dend = dest + dlen;
	register char *sptr = src;
	char *send = src + slen;

	/* Copy the fields */
	dprintf(DLEVEL,"chr2chr: dptr: %p, dlen: %d, dend: %p", dptr,dlen,dend);
	dprintf(DLEVEL,"chr2chr: sptr: %p, slen: %d, send: %p", sptr,slen,send);
	while(sptr < send && dptr < dend) {
		dprintf(DLEVEL,"sptr: %p, *sptr: %c",sptr,*sptr);
		*dptr++ = *sptr++;
		dprintf(DLEVEL,"dptr: %p, *dend: %p",dptr,dend);
	}

	/* Blank fill dest */
	dprintf(DLEVEL,"dptr: %p, dend: %p",dptr,dend);
	while(dptr < dend) *dptr++ = ' ';
	dprintf(DLEVEL,"dptr: %p, dend: %p",dptr,dend);
	dprintf(DLEVEL,"chr2chr: done.");
	return;
}

void chr2str(char *dest,int dlen,char *src,int slen) {
	register char *dptr = dest;
	char *dend = dest + dlen;
	register char *sptr = src;
	char *send = src + slen;

	/* Copy the fields */
	dprintf(DLEVEL,"chr2str: dptr: %p, dlen: %d, dend: %p",
		dptr,dlen,dend);
	dprintf(DLEVEL,"chr2str: sptr: %p, slen: %d, send: %p",
		sptr,slen,send);
	while(sptr < send && dptr < dend) {
		dprintf(DLEVEL,"sptr: %p, *sptr: %c",sptr,*sptr);
		*dptr++ = *sptr++;
		dprintf(DLEVEL,"dptr: %p, *dend: %p",dptr,dend);
	}

	/* Trim & zero-terminate dest */
	dprintf(DLEVEL,"dptr: %p, dend: %p",dptr,dend);
	while(isspace((int)*dptr)) dptr--;
	*dptr = 0;
	dprintf(DLEVEL,"dptr: %p, dend: %p",dptr,dend);
	return;
}

void chr2byte(byte *dest,int dlen,char *src,int slen) {
	char temp[8];
	int len = (slen >= sizeof(temp) ? sizeof(temp)-1 : slen);

	bcopy(src,temp,len);
	temp[len] = 0;
	dprintf(DLEVEL,"chr2byte: temp: %s",temp);
	*dest = atoi(temp);
	return;
}

void chr2short(short *dest,int dlen,char *src,int slen) {
	char temp[16];
	int len = (slen >= sizeof(temp) ? sizeof(temp)-1 : slen);

	bcopy(src,temp,len);
	temp[len] = 0;
	dprintf(DLEVEL,"chr2short: temp: %s",temp);
	*dest = atoi(temp);
	return;
}

void chr2int(int *dest,int dlen,char *src,int slen) {
	char temp[16];
	int len = (slen >= sizeof(temp) ? sizeof(temp)-1 : slen);

	bcopy(src,temp,len);
	temp[len] = 0;
	dprintf(DLEVEL,"chr2int: temp: %s",temp);
	*dest = atoi(temp);
	return;
}

void chr2long(long *dest,int dlen,char *src,int slen) {
	char temp[16];
	int len = (slen >= sizeof(temp) ? sizeof(temp)-1 : slen);

	bcopy(src,temp,len);
	temp[len] = 0;
	dprintf(DLEVEL,"chr2long: temp: %s",temp);
	*dest = atol(temp);
	return;
}

static void chr2quad(myquad_t *dest,int dlen,char *src,int slen) {
	char temp[32];
	int len = (slen >= sizeof(temp) ? sizeof(temp)-1 : slen);

	bcopy(src,temp,len);
	temp[len] = 0;
	dprintf(DLEVEL,"chr2quad: temp: %s",temp);
	*dest = mystrtoll(temp,0,0);
	return;
}

void chr2float(float *dest,int dlen,char *src,int slen) {
	char temp[32];
	int len = (slen >= sizeof(temp) ? sizeof(temp)-1 : slen);

	bcopy(src,temp,len);
	temp[len] = 0;
	dprintf(DLEVEL,"chr2float: temp: %s",temp);
	*dest = (float) atof(temp);
	return;
}

void chr2dbl(double *dest,int dlen,char *src,int slen) {
	char temp[32];
	int len = (slen >= sizeof(temp) ? sizeof(temp)-1 : slen);

	bcopy(src,temp,len);
	temp[len] = 0;
	dprintf(DLEVEL,"chr2dbl: temp: %s",temp);
	*dest = atof(temp);
	return;
}

void chr2log(int *dest,int dlen,char *src,int slen) {
	char temp[16];
	int len = (slen >= sizeof(temp) ? sizeof(temp)-1 : slen);

	bcopy(src,temp,len);
	temp[len] = 0;
	dprintf(DLEVEL,"chr2log: temp: %s",temp);
	*dest = (atoi(temp) == 0 ? 0 : 1);
	return;
}

void chr2list(list *dest,int dlen,char *src,int slen) {
	if (!*dest) *dest = list_create();
	if (*dest) {
		int x,len;
		len = (slen ? slen : 1);
		for(x=0; x < len; x++) list_add(*dest,(void *)&src[x],sizeof(*src));
	}
	return;
}

#define FMT "%f"

/*
 * DATA_TYPE_DOUBLE conversion functions
*/

void dbl2chr(char *dest,int dlen,double *src,int slen) {
	register int x;

	snprintf(dest,dlen,FMT,*src);
	x = strlen(dest);
	while(x < dlen) dest[x++] = ' ';
	return;
}

void dbl2str(char *dest,int dlen,double *src,int slen) {
	dprintf(DLEVEL,"dbl2str: src: %lf",*src);
	snprintf(dest,dlen-1,FMT,*src);
	dprintf(DLEVEL,"dbl2str: dest: %s",dest);
	return;
}

void dbl2byte(byte *dest,int dlen,double *src,int slen) {
	*dest = *src;
	return;
}

void dbl2short(short *dest,int dlen,double *src,int slen) {
	*dest = *src;
	return;
}

void dbl2int(int *dest,int dlen,double *src,int slen) {
	*dest = *src;
	return;
}

void dbl2long(long *dest,int dlen,double *src,int slen) {
	*dest = *src;
	return;
}

void dbl2quad(myquad_t *dest,int dlen,double *src,int slen) {
	*dest = *src;
	return;
}

void dbl2float(float *dest,int dlen,double *src,int slen) {
	*dest = *src;
	return;
}

void dbl2dbl(double *dest,int dlen,double *src,int slen) {
	*dest = *src;
	return;
}

void dbl2log(int *dest,int dlen,double *src,int slen) {
	*dest = (*src == 0.0 ? 0 : 1);
	return;
}

void dbl2date(char *dest,int dlen,double *src,int slen) {
	*dest = 0;
	return;
}

void dbl2list(list *dest,int dlen,double *src,int slen) {
	if (!*dest) *dest = list_create();
	if (*dest) {
		int x,len;
		len = (slen ? slen : 1);
		for(x=0; x < len; x++) list_add(*dest,(void *)&src[x],sizeof(*src));
	}
	return;
}

/*
 * DATA_TYPE_FLOAT conversion functions
*/

void float2chr(char *dest,int dlen,float *src,int slen) {
	register int x;

	sprintf(dest,"%f",*src);
	x = strlen(dest);
	while(x < dlen) dest[x++] = ' ';
	return;
}

void float2str(char *dest,int dlen,float *src,int slen) {
	sprintf(dest,"%f",*src);
	return;
}

void float2byte(byte *dest,int dlen,float *src,int slen) {
	*dest = *src;
	return;
}

void float2short(short *dest,int dlen,float *src,int slen) {
	*dest = *src;
	return;
}

void float2int(int *dest,int dlen,float *src,int slen) {
	*dest = *src;
	return;
}

void float2long(long *dest,int dlen,float *src,int slen) {
	*dest = *src;
	return;
}

void float2quad(myquad_t *dest,int dlen,float *src,int slen) {
	*dest = *src;
	return;
}

void float2float(float *dest,int dlen,float *src,int slen) {
	*dest = *src;
	return;
}

void float2dbl(double *dest,int dlen,float *src,int slen) {
	*dest = *src;
	return;
}

void float2log(int *dest,int dlen,float *src,int slen) {
	*dest = (*src == 0.0 ? 0 : 1);
	return;
}

void float2date(char *dest,int dlen,float *src,int slen) {
	*dest = 0;
	return;
}

void float2list(list *dest,int dlen,float *src,int slen) {
	if (!*dest) *dest = list_create();
	if (*dest) {
		int x,len;
		len = (slen ? slen : 1);
		for(x=0; x < len; x++) list_add(*dest,(void *)&src[x],sizeof(*src));
	}
	return;
}

/*
 * DATA_TYPE_INT conversion functions
*/

void int2chr(char *dest,int dlen,int *src,int slen) {
	register int x;

	sprintf(dest,"%d",*src);
	x = strlen(dest);
	while(x < dlen) dest[x++] = ' ';
	return;
}

void int2str(char *dest,int dlen,int *src,int slen) {
	sprintf(dest,"%d",*src);
	return;
}

void int2byte(byte *dest,int dlen,int *src,int slen) {
	*dest = *src;
	return;
}

void int2short(short *dest,int dlen,int *src,int slen) {
	*dest = *src;
	return;
}
void int2int(int *dest,int dlen,int *src,int slen) {
	*dest = *src;
	return;
}

void int2long(long *dest,int dlen,int *src,int slen) {
	*dest = *src;
	return;
}

void int2quad(myquad_t *dest,int dlen,int *src,int slen) {
	*dest = *src;
	return;
}

void int2float(float *dest,int dlen,int *src,int slen) {
	*dest = *src;
	return;
}

void int2dbl(double *dest,int dlen,int *src,int slen) {
	*dest = *src;
	return;
}

void int2log(int *dest,int dlen,int *src,int slen) {
	*dest = (*src == 0 ? 0 : 1);
	return;
}

void int2date(char *dest,int dlen,int *src,int slen) {
	*dest = 0;
	return;
}

void int2list(list *dest,int dlen,int *src,int slen) {
	if (!*dest) *dest = list_create();
	if (*dest) {
		int x,len;
		len = (slen ? slen : 1);
		for(x=0; x < len; x++) list_add(*dest,(void *)&src[x],sizeof(*src));
	}
	return;
}

void list2str(char *dest,int dlen,list *src,int slen) {
	int first,dl,sl;
	char *ptr;

	dprintf(DLEVEL,"dest: %p, dlen: %d, list: %p, slen: %d", dest, dlen, src, slen);
	dl = *dest = 0;
	if (!src) return;
	first = 1;
	list_reset(*src);
	while( (ptr = list_get_next(*src)) != 0) {
		sl = strlen(ptr);
		dprintf(DLEVEL,"list2str: ptr: %s, len: %d",ptr,sl);
		dprintf(DLEVEL,"dl: %d, sl: %d, dlen: %d",dl,sl,dlen);
		if (dl + sl >= dlen) break;
		if (first == 0) strcat(dest,",");
		else first = 0;
		strcat(dest,ptr);
	}
	dprintf(DLEVEL,"dest: %s",dest);
	return;
}

void list2chr(char *dest,int dlen,list *src,int slen) {
	register int x;

	list2str(dest,dlen,src,slen);
	x = strlen(dest);
	while(x < dlen) dest[x++] = ' ';
	return;
}

void list2byte(byte *dest,int dlen,list *src,int slen) {
	*dest = 0;
}

void list2short(short *dest,int dlen,list *src,int slen) {
	*dest = 0;
}

void list2int(int *dest,int dlen,list *src,int slen) {
	*dest = 0;
}

void list2long(long *dest,int dlen,list *src,int slen) {
	*dest = 0;
}

void list2quad(myquad_t *dest,int dlen,list *src,int slen) {
	*dest = 0;
}

void list2float(float *dest,int dlen,list *src,int slen) {
	*dest = 0;
}

void list2dbl(double *dest,int dlen,list *src,int slen) {
	*dest = 0;
}

void list2log(int *dest,int dlen,list *src,int slen) {
	*dest = 0;
}

void list2date(char *dest,int dlen,list *src,int slen) {
	*dest = 0;
}

void list2list(list *dest,int dlen,list *src,int slen) {
	if (!*dest) *dest = list_create();
	if (*dest) list_add_list(*dest,*src);
	return;
}

void log2chr(char *dest,int dlen,int *src,int slen) {
	register int x;

	sprintf(dest,"%s",(*src == 0 ? "False" : "True"));
	x = strlen(dest);
	while(x < dlen) dest[x++] = ' ';
	return;
}

void log2str(char *dest,int dlen,int *src,int slen) {
	char *p;
	switch(*src) {
	case -1:
		p = "null";
		break;
	case 0:
		p = "False";
		break;
	default:
		p = "True";
		break;
	}
	sprintf(dest,p);
	return;
}

void log2list(list *dest,int dlen,int *src,int slen) {
	if (!*dest) *dest = list_create();
	if (*dest) {
		int x,len;
		len = (slen ? slen : 1);
		for(x=0; x < len; x++) list_add(*dest,(void *)&src[x],sizeof(*src));
	}
	return;
}

/*
 * DATA_TYPE_LONG conversion functions
*/

void long2chr(char *dest,int dlen,long *src,int slen) {
	register int x;

	sprintf(dest,"%ld",*src);
	x = strlen(dest);
	while(x < dlen) dest[x++] = ' ';
	return;
}

void long2str(char *dest,int dlen,long *src,int slen) {
	sprintf(dest,"%ld",*src);
	return;
}

void long2byte(byte *dest,int dlen,long *src,int slen) {
	*dest = *src;
	return;
}

void long2short(short *dest,int dlen,long *src,int slen) {
	*dest = *src;
	return;
}

void long2int(int *dest,int dlen,long *src,int slen) {
	*dest = *src;
	return;
}

void long2long(long *dest,int dlen,long *src,int slen) {
	*dest = *src;
	return;
}

void long2quad(myquad_t *dest,int dlen,long *src,int slen) {
	*dest = *src;
	return;
}

void long2float(float *dest,int dlen,long *src,int slen) {
	*dest = *src;
	return;
}

void long2dbl(double *dest,int dlen,long *src,int slen) {
	*dest = *src;
	return;
}

void long2log(int *dest,int dlen,long *src,int slen) {
	*dest = (*src == 0 ? 0 : 1);
	return;
}

void long2date(char *dest,int dlen,long *src,int slen) {
	*dest = 0;
	return;
}

void long2list(list *dest,int dlen,long *src,int slen) {
	if (!*dest) *dest = list_create();
	if (*dest) {
		int x,len;
		len = (slen ? slen : 1);
		for(x=0; x < len; x++) list_add(*dest,(void *)&src[x],sizeof(*src));
	}
	return;
}

/*
 * DATA_TYPE_QUAD conversion functions
*/

void quad2chr(char *dest,int dlen,myquad_t *src,int slen) {
	register int x;

#ifdef __WIN32
	sprintf(dest,"%"PRId64,*src);
#else
	sprintf(dest,"%lld",*src);
#endif
	x = strlen(dest);
	while(x < dlen) dest[x++] = ' ';
	return;
}

void quad2str(char *dest,int dlen,myquad_t *src,int slen) {
	snprintf(dest,dlen,"%lld",*src);
	return;
}

void quad2byte(byte *dest,int dlen,myquad_t *src,int slen) {
	*dest = *src;
	return;
}

void quad2short(short *dest,int dlen,myquad_t *src,int slen) {
	*dest = (short) *src;
	return;
}

void quad2int(int *dest,int dlen,myquad_t *src,int slen) {
	*dest = *src;
	return;
}

void quad2long(long *dest,int dlen,myquad_t *src,int slen) {
	*dest = *src;
	return;
}

void quad2quad(myquad_t *dest,int dlen,myquad_t *src,int slen) {
	*dest = *src;
	return;
}

void quad2float(float *dest,int dlen,myquad_t *src,int slen) {
	*dest = (float) *src;
	return;
}

void quad2dbl(double *dest,int dlen,myquad_t *src,int slen) {
	*dest = *src;
	return;
}

void quad2log(int *dest,int dlen,myquad_t *src,int slen) {
	*dest = (*src == 0 ? 0 : 1);
	return;
}

void quad2date(char *dest,int dlen,myquad_t *src,int slen) {
	*dest = 0;
	return;
}

void quad2list(list *dest,int dlen,myquad_t *src,int slen) {
	if (!*dest) *dest = list_create();
	if (*dest) {
		int x,len;
		len = (slen ? slen : 1);
		for(x=0; x < len; x++) list_add(*dest,(void *)&src[x],sizeof(*src));
	}
	return;
}

/*
 * DATA_TYPE_SHORT conversion functions
*/

void short2chr(char *dest,int dlen,short *src,int slen) {
	register int x;

	sprintf(dest,"%d",*src);
	x = strlen(dest);
	while(x < dlen) dest[x++] = ' ';
	return;
}

void short2str(char *dest,int dlen,short *src,int slen) {
	sprintf(dest,"%d",*src);
	return;
}

void short2byte(byte *dest,int dlen,short *src,int slen) {
	*dest = *src;
	return;
}

void short2short(short *dest,int dlen,short *src,int slen) {
	*dest = *src;
	return;
}

void short2int(int *dest,int dlen,short *src,int slen) {
	*dest = *src;
	return;
}

void short2long(long *dest,int dlen,short *src,int slen) {
	*dest = *src;
	return;
}

void short2quad(myquad_t *dest,int dlen,short *src,int slen) {
	*dest = *src;
	return;
}

void short2float(float *dest,int dlen,short *src,int slen) {
	*dest = *src;
	return;
}

void short2dbl(double *dest,int dlen,short *src,int slen) {
	*dest = *src;
	return;
}

void short2log(int *dest,int dlen,short *src,int slen) {
	*dest = (*src == 0 ? 0 : 1);
	return;
}

void short2date(char *dest,int dlen,short *src,int slen) {
	*dest = 0;
	return;
}

void short2list(list *dest,int dlen,short *src,int slen) {
	if (!*dest) *dest = list_create();
	if (*dest) {
		int x,len;
		len = (slen ? slen : 1);
		for(x=0; x < len; x++) list_add(*dest,(void *)&src[x],sizeof(*src));
	}
	return;
}

/*
 * DATA_TYPE_STRING conversion functions
*/

void str2chr(char *dest,int dlen,char *src,int slen) {
	register int x,y;

	dprintf(DLEVEL,"src: %s",src);
	x=y=0;
	while(x < slen && src[x] != 0 && y < dlen) dest[y++] = src[x++];
	while(y < dlen) dest[y++] = ' ';

	return;
}

void str2str(char *dest,int dlen,char *src,int slen) {

	dprintf(DLEVEL,"src: %s",src);

	/* Copy the fields */
	*dest = 0;
	dprintf(DLEVEL,"dlen: %d", dlen);
	strncat(dest,src,dlen);
	trim(dest);

	dprintf(DLEVEL,"dest: %s",dest);
	return;
}

void str2byte(byte *dest,int dlen,char *src,int slen) {
	dprintf(DLEVEL,"src: %s",src);
	*dest = atoi(src);
	dprintf(DLEVEL,"src: %s",src);
	return;
}

void str2short(short *dest,int dlen,char *src,int slen) {
	dprintf(DLEVEL,"src: %s",src);
	*dest = atoi(src);
	dprintf(DLEVEL,"dest: %d",*dest);
	return;
}

void str2int(int *dest,int dlen,char *src,int slen) {
	dprintf(DLEVEL,"src: %s",src);
	*dest = atoi(src);
	dprintf(DLEVEL,"dest: %d",*dest);
	return;
}

void str2long(long *dest,int dlen,char *src,int slen) {
	dprintf(DLEVEL,"src: %s",src);
	*dest = atol(src);
	dprintf(DLEVEL,"dest: %ld",*dest);
	return;
}

void str2quad(myquad_t *dest,int dlen,char *src,int slen) {
	dprintf(DLEVEL,"src: %s",src);
	*dest = mystrtoll(src, 0, 0);
	dprintf(DLEVEL,"dest: %lld",*dest);
	return;
}

void str2float(float *dest,int dlen,char *src,int slen) {
	dprintf(DLEVEL,"src: %s",src);
	*dest = (float) atof(src);
	dprintf(DLEVEL,"dest: %f",*dest);
	return;
}

void str2dbl(double *dest,int dlen,char *src,int slen) {
	*dest = atof(src);
	return;
}

void str2log(int *dest,int dlen,char *src,int slen) {
#if 0
	dprintf(DLEVEL,"src: %s",src);
	*dest = (strcasecmp(src,"true")==0 || strcasecmp(src,"yes")==0 || strcmp(src,"1")==0) ? 1 : 0;
	dprintf(DLEVEL,"dest: %d",*dest);
#endif
	register char *ptr, ch;

	dprintf(DLEVEL,"src: %s",src);
	if (strcmp(src,"-1")==0 || strcasecmp(src,"null")==0) {
		*dest = -1;
		dprintf(DLEVEL,"dest: %d",*dest);
		return;
	}
	for(ptr = src; *ptr && isspace(*ptr); ptr++);
	dprintf(DLEVEL,"ptr: %s",ptr);
	ch = toupper(*ptr);
	dprintf(DLEVEL,"ch: %c",ch);
	*dest = (ch == 'T' || ch == 'Y' || ch == '1' ? 1 : 0);
	dprintf(DLEVEL,"dest: %d",*dest);
	return;
}

void str2date(char *dest,int dlen,char *src,int slen) {
	int len = (slen > dlen ? dlen : slen);

	*dest = 0;
	strncat(dest,src,len);
	return;
}

void str2list(list *dest,int dlen,char *src,int slen) {
	char temp[2048];
	int tmax,tlen,q;
	register char *p;
	register int i;

	dprintf(DLEVEL,"dest: %p, dlen: %d, src: %s, slen: %d", dest, dlen, src, slen);
	*dest = list_create();
	dprintf(DLEVEL,"src: %s",src);
	if (!src) return;

	p = src;
	tmax = sizeof(temp)-1;
	i = q = 0;
	while(*p) {
		dprintf(DLEVEL,"p: %c, q: %d\n", *p, q);
		if (*p == '\"' && !q)
			q = 1;
		else if (*p == '\"' && q)
			q = 0;
		else if ((*p == ',' && !q) || i >= tmax) {
			temp[i] = 0;
			trim(temp);
			tlen = strlen(temp);
			dprintf(DLEVEL,"adding: %s (%d)\n", temp, tlen);
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
		dprintf(DLEVEL,"adding: %s (%d)\n", temp, tlen);
		if (tlen) list_add(*dest,temp,tlen+1);
	}
#if DEBUG
	dprintf(DLEVEL,"list:");
	list_reset(*dest);
	while( (p = list_get_next(*dest)) != 0) dprintf(DLEVEL,"\t%s",p);
#endif
	return;
}

#define chr2date chr2str
#define date2chr str2chr
#define date2str str2str
#define date2byte str2byte
#define date2short str2short
#define date2int str2int
#define date2long str2long
#define date2quad str2quad
#define date2float str2float
#define date2dbl str2dbl
#define date2log str2log
#define date2date str2str
#define date2list str2list
#define log2byte int2byte
#define log2short int2short
#define log2int int2int
#define log2long int2long
#define log2quad int2quad
#define log2float int2float
#define log2dbl int2dbl
#define log2log int2int
#define log2date int2date

static conv_func_t *conv_funcs[13][13] = {
	{ 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 },
	{	0,
		(conv_func_t *)chr2chr,
		(conv_func_t *)chr2str,
		(conv_func_t *)chr2byte,
		(conv_func_t *)chr2short,
		(conv_func_t *)chr2int,
		(conv_func_t *)chr2long,
		(conv_func_t *)chr2quad,
		(conv_func_t *)chr2float,
		(conv_func_t *)chr2dbl,
		(conv_func_t *)chr2log,
		(conv_func_t *)chr2date,
		(conv_func_t *)chr2list,
	},
	{	0,
		(conv_func_t *)str2chr,
		(conv_func_t *)str2str,
		(conv_func_t *)str2byte,
		(conv_func_t *)str2short,
		(conv_func_t *)str2int,
		(conv_func_t *)str2long,
		(conv_func_t *)str2quad,
		(conv_func_t *)str2float,
		(conv_func_t *)str2dbl,
		(conv_func_t *)str2log,
		(conv_func_t *)str2date,
		(conv_func_t *)str2list,
	},
	{	0,
		(conv_func_t *)byte2chr,
		(conv_func_t *)byte2str,
		(conv_func_t *)byte2byte,
		(conv_func_t *)byte2short,
		(conv_func_t *)byte2int,
		(conv_func_t *)byte2long,
		(conv_func_t *)byte2quad,
		(conv_func_t *)byte2float,
		(conv_func_t *)byte2dbl,
		(conv_func_t *)byte2log,
		(conv_func_t *)byte2date,
		(conv_func_t *)byte2list,
	},
	{	0,
		(conv_func_t *)short2chr,
		(conv_func_t *)short2str,
		(conv_func_t *)short2byte,
		(conv_func_t *)short2short,
		(conv_func_t *)short2int,
		(conv_func_t *)short2long,
		(conv_func_t *)short2quad,
		(conv_func_t *)short2float,
		(conv_func_t *)short2dbl,
		(conv_func_t *)short2log,
		(conv_func_t *)short2date,
		(conv_func_t *)short2list,
	},
	{	0,
		(conv_func_t *)int2chr,
		(conv_func_t *)int2str,
		(conv_func_t *)int2byte,
		(conv_func_t *)int2short,
		(conv_func_t *)int2int,
		(conv_func_t *)int2long,
		(conv_func_t *)int2quad,
		(conv_func_t *)int2float,
		(conv_func_t *)int2dbl,
		(conv_func_t *)int2log,
		(conv_func_t *)int2date,
		(conv_func_t *)int2list,
	},
	{	0,
		(conv_func_t *)long2chr,
		(conv_func_t *)long2str,
		(conv_func_t *)long2byte,
		(conv_func_t *)long2short,
		(conv_func_t *)long2int,
		(conv_func_t *)long2long,
		(conv_func_t *)long2quad,
		(conv_func_t *)long2float,
		(conv_func_t *)long2dbl,
		(conv_func_t *)long2log,
		(conv_func_t *)long2date,
		(conv_func_t *)long2list,
	},
	{	0,
		(conv_func_t *)quad2chr,
		(conv_func_t *)quad2str,
		(conv_func_t *)quad2byte,
		(conv_func_t *)quad2short,
		(conv_func_t *)quad2int,
		(conv_func_t *)quad2long,
		(conv_func_t *)quad2quad,
		(conv_func_t *)quad2float,
		(conv_func_t *)quad2dbl,
		(conv_func_t *)quad2log,
		(conv_func_t *)quad2date,
		(conv_func_t *)quad2list,
	},
	{	0,
		(conv_func_t *)float2chr,
		(conv_func_t *)float2str,
		(conv_func_t *)float2byte,
		(conv_func_t *)float2short,
		(conv_func_t *)float2int,
		(conv_func_t *)float2long,
		(conv_func_t *)float2quad,
		(conv_func_t *)float2float,
		(conv_func_t *)float2dbl,
		(conv_func_t *)float2log,
		(conv_func_t *)float2date,
		(conv_func_t *)float2list,
	},
	{	0,
		(conv_func_t *)dbl2chr,
		(conv_func_t *)dbl2str,
		(conv_func_t *)dbl2byte,
		(conv_func_t *)dbl2short,
		(conv_func_t *)dbl2int,
		(conv_func_t *)dbl2long,
		(conv_func_t *)dbl2quad,
		(conv_func_t *)dbl2float,
		(conv_func_t *)dbl2dbl,
		(conv_func_t *)dbl2log,
		(conv_func_t *)dbl2date,
		(conv_func_t *)dbl2list,
	},
	{	0,
		(conv_func_t *)log2chr,
		(conv_func_t *)log2str,
		(conv_func_t *)log2byte,
		(conv_func_t *)log2short,
		(conv_func_t *)log2int,
		(conv_func_t *)log2long,
		(conv_func_t *)log2quad,
		(conv_func_t *)log2float,
		(conv_func_t *)log2dbl,
		(conv_func_t *)log2log,
		(conv_func_t *)log2date,
		(conv_func_t *)log2list,
	},
	{	0,
		(conv_func_t *)date2chr,
		(conv_func_t *)date2str,
		(conv_func_t *)date2byte,
		(conv_func_t *)date2short,
		(conv_func_t *)date2int,
		(conv_func_t *)date2long,
		(conv_func_t *)date2quad,
		(conv_func_t *)date2float,
		(conv_func_t *)date2dbl,
		(conv_func_t *)date2log,
		(conv_func_t *)date2date,
		(conv_func_t *)date2list,
	},
	{	0,
		(conv_func_t *)list2chr,
		(conv_func_t *)list2str,
		(conv_func_t *)list2byte,
		(conv_func_t *)list2short,
		(conv_func_t *)list2int,
		(conv_func_t *)list2long,
		(conv_func_t *)list2quad,
		(conv_func_t *)list2float,
		(conv_func_t *)list2dbl,
		(conv_func_t *)list2log,
		(conv_func_t *)list2date,
		(conv_func_t *)list2list,
	},
};

void conv_type(int dt,void *d,int dl,int st,void *s,int sl) {
	conv_func_t *func;

//	printf("st: %d(%s), dt: %d(%s)\n", st, typestr(st), dt, typestr(dt));
	/* Allocate convert temp var */
	dprintf(DLEVEL,"dest type: %s",typestr(dt));
	dprintf(DLEVEL,"src type: %s",typestr(st));
	if ((st > DATA_TYPE_UNKNOWN && st < DATA_TYPE_MAX) &&
	    (dt > DATA_TYPE_UNKNOWN && dt < DATA_TYPE_MAX)) {
		func = conv_funcs[st][dt];
		if (func) func(d,dl,s,sl);
	}
	return;
}

#if 0
/*-
 * Copyright (c) 1992, 1993
 *	The Regents of the University of California.  All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. All advertising materials mentioning features or use of this software
 *    must display the following acknowledgement:
 *	This product includes software developed by the University of
 *	California, Berkeley and its contributors.
 * 4. Neither the name of the University nor the names of its contributors
 *    may be used to endorse or promote products derived from this software
 *    without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE REGENTS AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE REGENTS OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 */

#include <sys/types.h>

#include <limits.h>
#include <errno.h>
#include <ctype.h>
#include <stdlib.h>

#ifndef LLONG_MAX
#define ULLONG_MAX      0xffffffffffffffffULL
#define LLONG_MAX       0x7fffffffffffffffLL    /* max value for a long long */
#define LLONG_MIN       (-0x7fffffffffffffffLL - 1) /* min for a long long */
#endif

/*
 * Convert a string to a long long integer.
 *
 * Ignores `locale' stuff.  Assumes that the upper and lower case
 * alphabets and digits are each contiguous.
 */
static long long mystrtoll(const char *nptr, char *endptr, register int base) {
	register const char *s;
	register unsigned long long acc;
	register unsigned char c;
	register unsigned long long qbase, cutoff;
	register int neg, any, cutlim;

	/*
	 * Skip white space and pick up leading +/- sign if any.
	 * If base is 0, allow 0x for hex and 0 for octal, else
	 * assume decimal; if base is already 16, allow 0x.
	 */
	s = nptr;
	do {
		c = *s++;
	} while (isspace(c));
	if (c == '-') {
		neg = 1;
		c = *s++;
	} else {
		neg = 0;
		if (c == '+')
			c = *s++;
	}
	if ((base == 0 || base == 16) &&
	    c == '0' && (*s == 'x' || *s == 'X')) {
		c = s[1];
		s += 2;
		base = 16;
	}
	if (base == 0)
		base = c == '0' ? 8 : 10;

	/*
	 * Compute the cutoff value between legal numbers and illegal
	 * numbers.  That is the largest legal value, divided by the
	 * base.  An input number that is greater than this value, if
	 * followed by a legal input character, is too big.  One that
	 * is equal to this value may be valid or not; the limit
	 * between valid and invalid numbers is then based on the last
	 * digit.  For instance, if the range for quads is
	 * [-9223372036854775808..9223372036854775807] and the input base
	 * is 10, cutoff will be set to 922337203685477580 and cutlim to
	 * either 7 (neg==0) or 8 (neg==1), meaning that if we have
	 * accumulated a value > 922337203685477580, or equal but the
	 * next digit is > 7 (or 8), the number is too big, and we will
	 * return a range error.
	 *
	 * Set any if any `digits' consumed; make it negative to indicate
	 * overflow.
	 */
	qbase = (unsigned)base;
	cutoff = neg ? (unsigned long long)-(LLONG_MIN + LLONG_MAX) + LLONG_MAX : LLONG_MAX;
	/* When compiling into a DLL for win32, it always crashes here on the below line */
	cutlim = cutoff % qbase;
	cutoff /= qbase;
	for (acc = 0, any = 0;; c = *s++) {
		if (!isascii(c))
			break;
		if (isdigit(c))
			c -= '0';
		else if (isalpha(c))
			c -= isupper(c) ? 'A' - 10 : 'a' - 10;
		else
			break;
		if (c >= base)
			break;
		if (any < 0 || acc > cutoff || (acc == cutoff && c > cutlim))
			any = -1;
		else {
			any = 1;
			acc *= qbase;
			acc += c;
		}
	}
	if (any < 0) {
		acc = neg ? LLONG_MIN : LLONG_MAX;
		errno = ERANGE;
	} else if (neg)
		acc = -acc;
	if (endptr != 0)
		*endptr = (char *)(any ? s - 1 : nptr);
	return (acc);
}
#endif

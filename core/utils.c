
/*
Copyright (c) 2021, Stephen P. Shoecraft
All rights reserved.

This source code is licensed under the BSD-style license found in the
LICENSE file in the root directory of this source tree.
*/

#define DEBUG_UTILS 0
#define dlevel 6

#ifdef DEBUG
#undef DEBUG
#define DEBUG DEBUG_UTILS
#endif
#include "debug.h"

#include <stdio.h>
#include <string.h>
#include <ctype.h>
#include <errno.h>
#include <time.h>
#include <stdarg.h>
#include <stdlib.h>
#include <unistd.h>
#ifdef __WIN32
#include <windows.h>
#endif
#include <math.h>
#include "utils.h"

void _bindump(long offset,void *bptr,int len) {
	unsigned char *buf = bptr;
	char line[128];
	int end;
	register char *ptr;
	register int x,y;

//	dprintf(dlevel,"buf: %p, len: %d\n", buf, len);
#ifdef __WIN32__
	if (buf == (void *)0xBAADF00D) return;
#endif

	for(x=y=0; x < len; x += 16) {
		sprintf(line,"%04lX: ",offset);
		ptr = line + strlen(line);
		end=(x+16 >= len ? len : x+16);
		for(y=x; y < end; y++) {
			sprintf(ptr,"%02X ",buf[y]);
			ptr += 3;
		}
		for(y=end; y < x+17; y++) {
			sprintf(ptr,"   ");
			ptr += 3;
		}
		for(y=x; y < end; y++) {
			if (buf[y] > 31 && buf[y] < 127)
				*ptr++ = buf[y];
			else
				*ptr++ = '.';
		}
		for(y=end; y < x+16; y++) *ptr++ = ' ';
		*ptr = 0;
		log_info("%s\n", line);
		offset += 16;
	}
}

void bindump(char *label,void *bptr,int len) {
	log_info("%s(%d):\n",label,len);
	_bindump(0,bptr,len);
}

char *trim(char *string) {
	register char *src,*dest;

	/* If string is empty, just return it */
	if (*string == '\0') return string;

	/* Trim the front */
	src = string;
//	while(isspace((int)*src) && *src != '\t') src++;
	while(isspace((int)*src) || (*src > 0 && *src < 32)) src++;
	dest = string;
	while(*src != '\0') *dest++ = *src++;

	/* Trim the back */
	*dest-- = '\0';
	while((dest >= string) && isspace((int)*dest)) dest--;
	*(dest+1) = '\0';

	return string;
}

char *strele(int num,char *delimiter,char *string) {
	static char return_info[1024];
	register char *src,*dptr,*eptr,*cptr;
	register char *dest, qc;
	register int count;

	dprintf(dlevel,"Element: %d, delimiter: %s, string: %s\n",num,delimiter,string);

	eptr = string;
	dptr = delimiter;
	dest = return_info;
	count = 0;
	qc = 0;
	for(src = string; *src != '\0'; src++) {
		dprintf(dlevel,"src: %d, qc: %d\n", *src, qc);
		if (qc) {
			if (*src == qc) qc = 0;
			continue;
		} else {
			if (*src == '\"' || *src == '\'')  {
				qc = *src;
				cptr++;
			}
		}
		if (isspace(*src)) *src = 32;
#ifdef DEBUG_STRELE
		if (*src)
			dprintf(dlevel,"src: %c == ",*src);
		else
			dprintf(dlevel,"src: (null) == ");
		if (*dptr != 32)
			dprintf(dlevel,"dptr: %c\n",*dptr);
		else if (*dptr == 32)
			dprintf(dlevel,"dptr: (space)\n");
		else
			dprintf(dlevel,"dptr: (null)\n");
#endif
		if (*src == *dptr) {
			cptr = src+1;
			dptr++;
#ifdef DEBUG_STRELE
			if (*cptr != '\0')
				dprintf(dlevel,"cptr: %c == ",*cptr);
			else
				dprintf(dlevel,"cptr: (null) == ");
			if (*dptr != '\0')
				dprintf(dlevel,"dptr: %c\n",*dptr);
			else
				dprintf(dlevel,"dptr: (null)\n");
#endif
			while(*cptr == *dptr && *cptr != '\0' && *dptr != '\0') {
				cptr++;
				dptr++;
#ifdef DEBUG_STRELE
				if (*cptr != '\0')
					dprintf(dlevel,"cptr: %c == ",*cptr);
				else
					dprintf(dlevel,"cptr: (null) == ");
				if (*dptr != '\0')
					dprintf(dlevel,"dptr: %c\n",*dptr);
				else
					dprintf(dlevel,"dptr: (null)\n");
#endif
				if (*dptr == '\0' || *cptr == '\0') {
					dprintf(dlevel,"Breaking...\n");
					break;
				}
/*
				dptr++;
				if (*dptr == '\0') break;
				cptr++;
				if (*cptr == '\0') break;
*/
			}
#ifdef DEBUG_STRELE
			if (*cptr != '\0')
				dprintf(dlevel,"cptr: %c == ",*cptr);
			else
				dprintf(dlevel,"cptr: (null) == ");
			if (*dptr != '\0')
				dprintf(dlevel,"dptr: %c\n",*dptr);
			else
				dprintf(dlevel,"dptr: (null)\n");
#endif
			if (*dptr == '\0') {
				dprintf(dlevel,"Count: %d, num: %d\n",count,num);
				if (count == num) break;
				if (cptr > src+1) src = cptr-1;
				eptr = src+1;
				count++;
//				dprintf(dlevel,"eptr[0]: %c\n", eptr[0]);
				if (*eptr == '\"' || *eptr == '\'') eptr++;
				dprintf(dlevel,"eptr: %s, src: %s\n",eptr,src+1);
			}
			dptr = delimiter;
		}
	}
	dprintf(dlevel,"Count: %d, num: %d\n",count,num);
	if (count == num) {
		dprintf(dlevel,"eptr: %s\n",eptr);
		dprintf(dlevel,"src: %s\n",src);
		while(eptr < src) {
			if (*eptr == '\"' || *eptr == '\'') break;
			*dest++ = *eptr++;
		}
	}
	*dest = '\0';
	dprintf(dlevel,"Returning: %s\n",return_info);
	return(return_info);
}

int is_ip(char *string) {
	register char *ptr;
	int dots,digits;

	dprintf(7,"string: %s\n", string);

	digits = dots = 0;
	for(ptr=string; *ptr; ptr++) {
		if (*ptr == '.') {
			if (!digits) goto is_ip_error;
			if (++dots > 3) goto is_ip_error;
			digits = 0;
		} else if (isdigit((int)*ptr)) {
			if (++digits > 3) goto is_ip_error;
		} else {
			goto is_ip_error;
		}
	}
	dprintf(7,"dots: %d\n", dots);

	return (dots == 3 ? 1 : 0);
is_ip_error:
	return 0;
}
int get_timestamp(char *ts, int tslen, int local) {
	struct tm *tptr;
	time_t t;

	if (!ts || !tslen) return 1;

	/* Fill the tm struct */
	t = time(NULL);
	tptr = 0;
//	dprintf(2,"local: %d\n", local);
	if (local) tptr = localtime(&t);
	else tptr = gmtime(&t);
	if (!tptr) {
		dprintf(2,"unable to get %s!\n",local ? "localtime" : "gmtime");
		return 1;
	}

	/* Set month to 1 if month is out of range */
	if (tptr->tm_mon < 0 || tptr->tm_mon > 11) tptr->tm_mon = 0;

	/* Fill string with yyyymmddhhmmss */
	snprintf(ts,tslen,"%04d-%02d-%02d %02d:%02d:%02d",
		1900+tptr->tm_year,tptr->tm_mon+1,tptr->tm_mday,
		tptr->tm_hour,tptr->tm_min,tptr->tm_sec);

	dprintf(2,"returning: %s\n", ts);
	return 0;
}

char *os_getenv(const char *name) {
#ifdef __WIN32
	{
		static char value[4096];
		DWORD len = GetEnvironmentVariable(name, value, sizeof(value)-1);
		if (!len) return 0;
		if (len > sizeof(value)) return 0;
		return value;
	}
#else
	return getenv(name);
#endif
}

int os_setenv(const char *name, const char *value, int overwrite) {
#ifdef __WIN32
	return (SetEnvironmentVariable(name,value) ? 0 : 1);
#else
	return setenv(name,value,overwrite);
#endif
}

int os_exists(char *path, char *name) {

	if (name) {
		char temp[1024];
		snprintf(temp,sizeof(temp)-1,"%s/%s",path,name);
		return (access(temp,0) == 0);
	} else {
		return (access(path,0) == 0);
	}
}

bool fequal(float a, float b) {
	return fabs(a-b) < 0.00001f;
}

#if 0
int double_equals(double a, double b) {
	double r;
	int v;

//	printf("a: %lf, b: %lf\n", a, b);
	if (a > b) r = a - b;
	else r = b - a;
//	printf("a: %f, b: %f, r: %f\n", a, b, r);
	v= (r < 0.0000001f);
//	printf("v: %d\n", v);
	return v;
}
#endif

int double_equals(double a, double b) {
        double r;

        if (a > b) r = a - b;
        else r = b - a;
        dprintf(1,"a: %f, b: %f, r: %f\n", a, b, r);
        return (r > 0.00001);
}

#if 1
int float_equals(float f1, float f2) {
	int r,r1,r2;
//	float precision = 0.00001;
//	return (((f1 - precision) < f2) && (f1 + precision) > f2) ? 1 : 0;
	r1 = ((f1 - 0.00001) < f2);
	r2 = ((f1 + 0.00001) > f2);
	r = r1 && r2;
//	printf("r: %d, r1: %d, r2: %d\n", r, r1, r2);
	return r;
}
#else
int float_equals(float a, float b) {
	float r;

	if (a > b) r = a - b;
	else r = b - a;
	dprintf(0,"a: %f, b: %f, r: %f\n", a, b, r);
	dprintf(0,"returning: %d\n", r < 0.00001f);
	return (r < 0.00001f);
}
#endif

int double_isint(double z) {
	return (fabsf(roundf(z) - z) <= 0.00001f ? 1 : 0);
}

const char *getBuild() { //Get current architecture, detectx nearly every architecture. Coded by Freak
        #if defined(__x86_64__) || defined(_M_X64)
        return "x86_64";
        #elif defined(i386) || defined(__i386__) || defined(__i386) || defined(_M_IX86)
        return "x86_32";
        #elif defined(__ARM_ARCH_2__)
        return "ARM2";
        #elif defined(__ARM_ARCH_3__) || defined(__ARM_ARCH_3M__)
        return "ARM3";
        #elif defined(__ARM_ARCH_4T__) || defined(__TARGET_ARM_4T)
        return "ARM4T";
        #elif defined(__ARM_ARCH_5_) || defined(__ARM_ARCH_5E_)
        return "ARM5"
        #elif defined(__ARM_ARCH_6T2_) || defined(__ARM_ARCH_6T2_)
        return "ARM6T2";
        #elif defined(__ARM_ARCH_6__) || defined(__ARM_ARCH_6J__) || defined(__ARM_ARCH_6K__) || defined(__ARM_ARCH_6Z__) || defined(__ARM_ARCH_6ZK__)
        return "ARM6";
        #elif defined(__ARM_ARCH_7__) || defined(__ARM_ARCH_7A__) || defined(__ARM_ARCH_7R__) || defined(__ARM_ARCH_7M__) || defined(__ARM_ARCH_7S__)
        return "ARM7";
        #elif defined(__ARM_ARCH_7A__) || defined(__ARM_ARCH_7R__) || defined(__ARM_ARCH_7M__) || defined(__ARM_ARCH_7S__)
        return "ARM7A";
        #elif defined(__ARM_ARCH_7R__) || defined(__ARM_ARCH_7M__) || defined(__ARM_ARCH_7S__)
        return "ARM7R";
        #elif defined(__ARM_ARCH_7M__)
        return "ARM7M";
        #elif defined(__ARM_ARCH_7S__)
        return "ARM7S";
        #elif defined(__aarch64__) || defined(_M_ARM64)
        return "ARM64";
        #elif defined(mips) || defined(__mips__) || defined(__mips)
        return "MIPS";
        #elif defined(__sh__)
        return "SUPERH";
        #elif defined(__powerpc) || defined(__powerpc__) || defined(__powerpc64__) || defined(__POWERPC__) || defined(__ppc__) || defined(__PPC__) || defined(_ARCH_PPC)
        return "POWERPC";
        #elif defined(__PPC64__) || defined(__ppc64__) || defined(_ARCH_PPC64)
        return "POWERPC64";
        #elif defined(__sparc__) || defined(__sparc)
        return "SPARC";
        #elif defined(__m68k__)
        return "M68K";
        #else
        return "UNKNOWN";
        #endif
    }

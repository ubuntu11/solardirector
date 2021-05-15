
/*
Copyright (c) 2021, Stephen P. Shoecraft
All rights reserved.

This source code is licensed under the BSD-style license found in the
LICENSE file in the root directory of this source tree.
*/

#include <stdio.h>
#include <string.h>
#include <ctype.h>
#include <errno.h>
#include <time.h>
#include <stdarg.h>
#ifdef DEBUG
#undef DEBUG
#endif
//#define DEBUG 1
#include "utils.h"
#include "debug.h"
#ifdef __WIN32
#include <windows.h>
#endif

void _bindump(long offset,void *bptr,int len) {
	unsigned char *buf = bptr;
	char line[128];
	int end;
	register char *ptr;
	register int x,y;

//	printf("buf: %p, len: %d\n", buf, len);
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
		printf("%s\n",line);
		offset += 16;
	}
}

void bindump(char *label,void *bptr,int len) {
	printf("%s(%d):\n",label,len);
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

#ifdef DEBUG_STRELE
	printf("Element: %d, delimiter: %s, string: %s\n",num,delimiter,string);
#endif

	eptr = string;
	dptr = delimiter;
	dest = return_info;
	count = 0;
	qc = 0;
	for(src = string; *src != '\0'; src++) {
#ifdef DEBUG_STRELE
		printf("src: %d, qc: %d\n", *src, qc);
#endif
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
			printf("src: %c == ",*src);
		else
			printf("src: (null) == ");
		if (*dptr != 32)
			printf("dptr: %c\n",*dptr);
		else if (*dptr == 32)
			printf("dptr: (space)\n");
		else
			printf("dptr: (null)\n");
#endif
		if (*src == *dptr) {
			cptr = src+1;
			dptr++;
#ifdef DEBUG_STRELE
			if (*cptr != '\0')
				printf("cptr: %c == ",*cptr);
			else
				printf("cptr: (null) == ");
			if (*dptr != '\0')
				printf("dptr: %c\n",*dptr);
			else
				printf("dptr: (null)\n");
#endif
			while(*cptr == *dptr && *cptr != '\0' && *dptr != '\0') {
				cptr++;
				dptr++;
#ifdef DEBUG_STRELE
				if (*cptr != '\0')
					printf("cptr: %c == ",*cptr);
				else
					printf("cptr: (null) == ");
				if (*dptr != '\0')
					printf("dptr: %c\n",*dptr);
				else
					printf("dptr: (null)\n");
#endif
				if (*dptr == '\0' || *cptr == '\0') {
#ifdef DEBUG_STRELE
					printf("Breaking...\n");
#endif
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
				printf("cptr: %c == ",*cptr);
			else
				printf("cptr: (null) == ");
			if (*dptr != '\0')
				printf("dptr: %c\n",*dptr);
			else
				printf("dptr: (null)\n");
#endif
			if (*dptr == '\0') {
#ifdef DEBUG_STRELE
				printf("Count: %d, num: %d\n",count,num);
#endif
				if (count == num) break;
				if (cptr > src+1) src = cptr-1;
				eptr = src+1;
				count++;
//				printf("eptr[0]: %c\n", eptr[0]);
				if (*eptr == '\"' || *eptr == '\'') eptr++;
#ifdef DEBUG_STRELE
				printf("eptr: %s, src: %s\n",eptr,src+1);
#endif
			}
			dptr = delimiter;
		}
	}
#ifdef DEBUG_STRELE
	printf("Count: %d, num: %d\n",count,num);
#endif
	if (count == num) {
#ifdef DEBUG_STRELE
		printf("eptr: %s\n",eptr);
		printf("src: %s\n",src);
#endif
		while(eptr < src) {
			if (*eptr == '\"' || *eptr == '\'') break;
			*dest++ = *eptr++;
		}
	}
	*dest = '\0';
#ifdef DEBUG_STRELE
	printf("Returning: %s\n",return_info);
#endif
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
			if (++dots > 4) goto is_ip_error;
			digits = 0;
		} else if (isdigit((int)*ptr)) {
			if (++digits > 3) goto is_ip_error;
		}
	}
	dprintf(7,"dots: %d\n", dots);

	return (dots == 4 ? 1 : 0);
is_ip_error:
	return 0;
}
int get_timestamp(char *ts, int tslen, int local) {
	struct tm *tptr;
	time_t t;
	char temp[32];

	if (!ts || !tslen) return 1;

	/* Fill the tm struct */
	t = time(NULL);
	tptr = 0;
	DPRINTF("local: %d\n", local);
	if (local) tptr = localtime(&t);
	else tptr = gmtime(&t);
	if (!tptr) {
		DPRINTF("unable to get %s!\n",local ? "localtime" : "gmtime");
		return 1;
	}

	/* Set month to 1 if month is out of range */
	if (tptr->tm_mon < 0 || tptr->tm_mon > 11) tptr->tm_mon = 0;

	/* Fill string with yyyymmddhhmmss */
	sprintf(temp,"%04d-%02d-%02d %02d:%02d:%02d",
		1900+tptr->tm_year,tptr->tm_mon+1,tptr->tm_mday,
		tptr->tm_hour,tptr->tm_min,tptr->tm_sec);
	DPRINTF("temp: %s\n", temp);

	ts[0] = 0;
	strncat(ts,temp,tslen);
	DPRINTF("returning: %s\n", ts);
	return 0;
}

char *os_getenv(const char *name) {
	static char value[4096];

#ifdef __WIN32
	DWORD len = GetEnvironmentVariable(name, value, sizeof(value)-1);
	if (!len) return 0;
	if (len > sizeof(value)) return 0;
	return value;
#else
	return getenv(name);
#endif
#if 0
	char *value;
#ifndef __WIN32
	char *p;

	dprintf(1,"name: %s\n", name);

	p = getenv(name);
	if (!p) return 0;
	value = malloc(strlen(p)+1);
	if (!value) return 0;
	strcpy(value,p);
#else
	static char temp[32];
	DWORD len;

	printf("name: %s\n", name);

	len = GetEnvironmentVariable(name, temp, sizeof(temp)-1);
	dprintf(1,"len: %d\n", len);
	if (!len) return 0;
	if (len > sizeof(temp)) {
		value = malloc(len);
		dprintf(1,"value: %p\n", value);
		if (!value) return 0;
		GetEnvironmentVariable(name, value, len-1);
		return value;
	}
	return temp;
#endif
#endif
}

int os_setenv(const char *name, const char *value, int overwrite) {
#ifdef __WIN32
	return (SetEnvironmentVariable(name,value) ? 0 : 1);
#else
	return setenv(name,value,overwrite);
#endif
}

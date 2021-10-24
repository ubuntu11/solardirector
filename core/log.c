
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

FILE *logfp = (FILE *) 0;
int logopts;

int log_open(char *ident,char *filename,int opts) {
	DPRINTF("filename: %s\n",filename);
	if (filename) {
		char *op;

		/* Open the file */
		op = (opts & LOG_CREATE ? "w+" : "a+");
		logfp = fopen(filename,op);
		if (!logfp) {
			perror("log_open: unable to create logfile");
			return 1;
		}
	} else if (opts & LOG_STDERR) {
		DPRINTF("logging to stderr\n");
		logfp = stderr;
	} else {
		DPRINTF("logging to stdout\n");
		logfp = stdout;
	}
	logopts = opts;

	DPRINTF("log is opened.\n");
	return 0;
}
int log_write(int type,char *format,...) {
	char message[16384];
	va_list ap;
	char dt[32],error[128];
	register char *ptr;
	int len;

	/* Make sure log_open was called */
	if (!logfp) log_open("",0,LOG_INFO|LOG_WARNING|LOG_ERROR|LOG_SYSERR|LOG_DEBUG);

	/* Do we even log this type? */
	DPRINTF("logopts: %0x, type: %0x\n",logopts,type);
	if ( (logopts | type) != logopts) return 0;

	/* get the error text asap before it's gone */
	if (type & LOG_SYSERR) {
		error[0] = 0;
		strncat(error,strerror(errno),sizeof(error));
	}

	/* Prepend the time? */
	ptr = message;
	if (logopts & LOG_TIME || type & LOG_TIME) {
//		struct tm *tptr;
//		time_t t;

		DPRINTF("prepending time...\n");
		get_timestamp(dt,sizeof(dt),1);
#if 0
		/* Fill the tm struct */
		t = time(NULL);
		tptr = 0;
		DPRINTF("getting localtime\n");
		tptr = localtime(&t);
		if (!tptr) {
			DPRINTF("unable to get localtime!\n");
			return 1;
		}

		/* Set month to 1 if month is out of range */
		if (tptr->tm_mon < 0 || tptr->tm_mon > 11) tptr->tm_mon = 0;

		/* Fill string with yyyymmddhhmmss */
		sprintf(dt,"%04d-%02d-%02d %02d:%02d:%02d",
			1900+tptr->tm_year,tptr->tm_mon+1,tptr->tm_mday,
			tptr->tm_hour,tptr->tm_min,tptr->tm_sec);
#endif

		strcat(dt,"  ");
		ptr += sprintf(ptr,"%s",dt);
	}

	/* If it's a warning, prepend warning: */
	if (type & LOG_WARNING) {
		DPRINTF("prepending warning...\n");
		sprintf(ptr,"warning: ");
		ptr += strlen(ptr);
	}

	/* If it's an error, prepend error: */
	else if ((type & LOG_ERROR) || (type & LOG_SYSERR)) {
		DPRINTF("prepending error...\n");
		sprintf(ptr,"error: ");
		ptr += strlen(ptr);
	}

	len = (sizeof(message) - strlen(message)) - 1;

	/* Build the rest of the message */
	DPRINTF("adding message...\n");
	DPRINTF("format: %p\n", format);
	va_start(ap,format);
	vsnprintf(ptr,len,format,ap);
	va_end(ap);

	/* Trim */
	trim(message);

	/* If it's a system error, concat the system message */
	if (type & LOG_SYSERR) {
		DPRINTF("adding error text...\n");
		strcat(message,": ");
		strcat(message, error);
	}

	/* Strip all CRs and LFs */
	DPRINTF("stripping newlines...\n");
	for(ptr = message; *ptr; ptr++) {
		if (*ptr == '\n' || *ptr == '\r')
			strcpy(ptr,ptr+1);
	}

	/* Write the message */
	DPRINTF("message: %s\n",message);
	fprintf(logfp,"%s\n",message);
	fflush(logfp);
	return 0;
}

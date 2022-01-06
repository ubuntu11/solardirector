
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

#if DEBUG
static struct _opt_info {
	int type;
	char *name;
} optlist[] = {
	{ LOG_CREATE, "CREATE" },
	{ LOG_TIME, "TIME" },
	{ LOG_STDERR, "STDERR" },
	{ LOG_INFO, "INFO" },
	{ LOG_VERBOSE, "VERBOSE" },
	{ LOG_WARNING, "WARNING" },
	{ LOG_ERROR, "ERROR" },
	{ LOG_SYSERR, "SYSERR" },
	{ LOG_DEBUG, "DEBUG" },
	{ LOG_DEBUG2, "DEBUG2" },
	{ LOG_DEBUG3, "DEBUG3" },
	{ LOG_DEBUG4, "DEBUG4" },
	{ 0,0 }
};

static __inline void _dispopts(int type) {
	register struct _opt_info *opt;
	char message[128];
	register char *ptr;

	message[0] = 0;
	sprintf(message,"log options:");
	ptr = message + strlen(message);
	for(opt = optlist; opt->name; opt++) {
		if (type & opt->type) {
			sprintf(ptr," %s",opt->name);
			ptr += strlen(ptr);
		}
	}
	if (logfp) fprintf(logfp,"%s\n",message);
	return;
}
#endif

int log_open(char *ident,char *filename,int opts) {

#if DEBUG
	DPRINTF("ident: %p, filename: %p,opts: %x\n",ident,filename,opts);
	_dispopts(opts);
#endif

	if (filename) {
		char *op;

		/* Open the file */
		op = (opts & LOG_CREATE ? "w+" : "a+");
		DPRINTF("op: %s\n", op);
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

//FILE *log_getfp(void) { return(logfp); }

static int _log_write(int type,char *format,va_list ap) {
	char message[32768];
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
	vsnprintf(ptr,len,format,ap);

	/* Trim */
	trim(message);

	/* If it's a system error, concat the system message */
	if (type & LOG_SYSERR) {
		DPRINTF("adding error text...\n");
		strcat(message,": ");
		strcat(message, error);
	}

	/* Strip all CRs and LFs */
#if 0
	DPRINTF("stripping newlines...\n");
	for(ptr = message; *ptr; ptr++) {
		if (*ptr == '\n' || *ptr == '\r')
			strcpy(ptr,ptr+1);
	}
#endif

	/* Write the message */
	DPRINTF("message: %s\n",message);
	fprintf(logfp,"%s\n",message);
	fflush(logfp);
	va_end(ap);
	return 0;
}

int log_write(int type, char *format, ...) {
	va_list ap;

	va_start(ap,format);
	return _log_write(type,format,ap);
}

#define LOGDEF(n,t) int n(char *format,...) { va_list ap; va_start(ap,format); return _log_write(t,format,ap); }

LOGDEF(log_info,LOG_INFO);
LOGDEF(log_warning,LOG_WARNING);
LOGDEF(log_error,LOG_ERROR);
LOGDEF(log_syserr,LOG_SYSERR);
LOGDEF(log_syserror,LOG_SYSERR);
LOGDEF(log_debug,LOG_DEBUG);


/*
Copyright (c) 2021, Stephen P. Shoecraft
All rights reserved.

This source code is licensed under the BSD-style license found in the
LICENSE file in the root directory of this source tree.
*/

#ifndef __UTILS_H
#define __UTILS_H

#include <stdlib.h>
#include "types.h"

#if 0
/* Define the data types */
/* XXX ORDER IS IMPORTANT HERE - DO NOT CHANGE */
enum DATA_TYPE {
	DATA_TYPE_UNKNOWN = 0,		/* Unknown */
	DATA_TYPE_CHAR,			/* Array of chars w/no null */
	DATA_TYPE_STRING,		/* Array of chars w/null */
	DATA_TYPE_BYTE,			/* char -- 8 bit */
	DATA_TYPE_SHORT,		/* Short -- 16 bit */
	DATA_TYPE_INT,			/* Integer -- 16 | 32 bit */
	DATA_TYPE_LONG,			/* Long -- 32 bit */
	DATA_TYPE_QUAD,			/* Quadword - 64 bit */
	DATA_TYPE_FLOAT,		/* Float -- 32 bit */
	DATA_TYPE_DOUBLE,		/* Double -- 64 bit */
	DATA_TYPE_LOGICAL,		/* (int) Yes/No,True/False,on/off */
	DATA_TYPE_DATE,			/* (char 23) DD-MMM-YYYY HH:MM:SS.HH */
	DATA_TYPE_LIST,			/* Itemlist */
	DATA_TYPE_MAX			/* Max data type number */
};
#endif

/* Special data types */
#ifndef WIN32
typedef char byte;
#else
#define bcopy(s,d,l) memcpy(d,s,l)
typedef unsigned char byte;
#endif
typedef long long myquad_t;

#include "cfg.h"

/* util funcs */
void bindump(char *label,void *bptr,int len);
void _bindump(long offset,void *bptr,int len);
char *trim(char *);
char *strele(int num,char *delimiter,char *string);
int is_ip(char *);
int get_timestamp(char *ts, int tslen, int local);
int become_daemon(void);
void conv_type(int dt,void *d,int dl,int st,void *s,int sl);
char *typestr(int);
int find_config_file(char *,char *,int);

/* Define the log options */
#define LOG_CREATE		0x0001	/* Create a new logfile */
#define LOG_TIME		0x0002	/* Prepend the time */
#define LOG_STDERR		0x0004	/* Log to stderr */
#define LOG_INFO		0x0008	/* Informational messages */
#define LOG_VERBOSE		0x0010	/* Full info messages */
#define LOG_WARNING		0x0020	/* Program warnings */
#define LOG_ERROR		0x0040	/* Program errors */
#define LOG_SYSERR		0x0080	/* System errors */
#define LOG_DEBUG		0x0100	/* Misc debug messages */
#define LOG_DEBUG2 		0x0200	/* func entry/exit */
#define LOG_DEBUG3		0x0400	/* inner loops! */
#define LOG_DEBUG4		0x0800	/* The dolly-lamma */
#define LOG_WX			0x8000	/* Log to wxMessage */
#define LOG_ALL			0x7FFF	/* Whoa, Nellie */
#define LOG_DEFAULT		(LOG_INFO|LOG_WARNING|LOG_ERROR|LOG_SYSERR)

#define LOG_SYSERROR LOG_SYSERR

/* Log funcs */
int log_open(char *,char *,int);
int log_read(char *,int);
int log_write(int,char *,...);
#define log_info(args...) log_write(LOG_INFO,args)
#define log_warning(args...) log_write(LOG_WARNING,args)
#define log_error(args...) log_write(LOG_ERROR,args)
#define log_syserror(args...) log_write(LOG_SYSERROR,args)
//int log_debug(char *,...);
void log_close(void);
void log_writeopts(void);
char *log_nextname(void);

#define lprintf(mask, format, args...) log_write(mask,format,## args)

int solard_exec(char *, char *[], char *,int);
int solard_checkpid(int pid, int *status);
int solard_kill(int);
void fixpath(char *,int);
int solard_get_path(char *prog, char *dest, int dest_len);
void tmpdir(char *, int);
int gethomedir(char *dest, int dest_len);
char *os_getenv(const char *name);
int os_setenv(const char *name, const char *value, int overwrite);
#ifdef __WIN32
int fork(void);
#endif

#endif

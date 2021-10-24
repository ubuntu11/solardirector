
/*
Copyright (c) 2021, Stephen P. Shoecraft
All rights reserved.

This source code is licensed under the BSD-style license found in the
LICENSE file in the root directory of this source tree.
*/

#include <string.h>
#include <stdlib.h>
#include "common.h"
#ifdef __WIN32
#include <winsock2.h>
#endif

#define DIRPATH_LEN 256
char SOLARD_BINDIR[DIRPATH_LEN];
char SOLARD_ETCDIR[DIRPATH_LEN];
char SOLARD_LIBDIR[DIRPATH_LEN];
char SOLARD_LOGDIR[DIRPATH_LEN];

#define CFG 1

#if defined(__WIN32) && !defined(__WIN64)
#include <windows.h>
BOOL InitOnceExecuteOnce(
	PINIT_ONCE    InitOnce,
	PINIT_ONCE_FN InitFn,
	PVOID         Parameter,
	LPVOID        *Context
) {
	InitFn(InitOnce,Parameter,Context);
	return TRUE;
}
#endif

static int _getcfg(cfg_info_t *cfg, char *dest, char *what) {
	char *p;
	int r;

	dprintf(7,"cfg: %p, what: %s\n", cfg, what);

	r = 1;
#if CFG
	p = cfg_get_item(cfg,"",what);
	if (!p) {
		p = cfg_get_item(cfg,"","prefix");
		if (!p) goto _getcfg_err;
		dprintf(7,"prefix: %s\n", p);
		sprintf(dest,"%s/%s",p,what);
	} else {
		strncat(dest,p,DIRPATH_LEN-1);
	}
	dprintf(7,"dest: %s\n", dest);
	r = 0;
_getcfg_err:
#endif
	dprintf(7,"returning: %d\n", r);
	return r;
}

static void _getpath(cfg_info_t *cfg, char *home, char *dest, char *what) {

	dprintf(7,"cfg: %p, home: %s, what: %s\n", cfg, home, what);

	if (cfg && _getcfg(cfg,dest,what) == 0) return;

#ifdef __WIN32
	strcpy(dest,"%ProgramFiles%\\SolarDirector");
	return;
#else
	/* Am I root? */
	if (getuid() == 0) {
		if (strcmp(what,"bin") == 0)
			strcpy(dest,"/usr/local/bin");
		else if (strcmp(what,"etc") == 0)
			strcpy(dest,"/usr/local/etc");
		else if (strcmp(what,"lib") == 0)
			strcpy(dest,"/usr/local/lib");
		else if (strcmp(what,"log") == 0)
			strcpy(dest,"/var/log");
		else {
			printf("error: init _getpath: no path set for: %s\n", what);
			dest = ".";
		}
		return;

	/* Not root */
	} else {
		sprintf(dest,"%s/%s",home,what);
		return;
	}
	printf("error: init: _getpath: no path set for: %s, using /tmp\n", what);
	dest = "/tmp";
#endif
}

static void _setp(char *name,char *value) {
	int len=strlen(value);
	if (len > 1 && value[len-1] == '/') value[len-1] = 0;
	fixpath(value,DIRPATH_LEN-1);
	os_setenv(name,value,1);
}

int solard_common_init(int argc,char **argv,opt_proctab_t *add_opts,int start_opts) {
	/* std command-line opts */
	static char logfile[256];
	char home[246],configfile[256];
	cfg_info_t *cfg;
	static int append_flag;
	static int back_flag,verb_flag;
	static int help_flag,err_flag;
	static opt_proctab_t std_opts[] = {
		{ "-b|run in background",&back_flag,DATA_TYPE_LOGICAL,0,0,"no" },
#ifdef DEBUG
		{ "-d:#|set debugging level",&debug,DATA_TYPE_INT,0,0,"0" },
#endif
		{ "-e|redirect output to stderr",&err_flag,DATA_TYPE_LOGICAL,0,0,"no" },
		{ "-h|display program options",&help_flag,DATA_TYPE_LOGICAL,0,0,"no" },
		{ "-l:%|redirect output to logfile",&logfile,DATA_TYPE_STRING,sizeof(logfile)-1,0,"" },
		{ "-a|append to logfile",&append_flag,DATA_TYPE_LOGICAL,0,0,"no" },
		{ "-v|enable verbose output ",&verb_flag,DATA_TYPE_LOGICAL,0,0,"no" },
		{ 0,0,0,0,0 }
	};
	opt_proctab_t *opts;
	char *file;
	volatile int error,log_opts;
	char *ident;
#ifdef __WIN32
	WSADATA wsaData;
	int iResult;
#endif

//	printf("initializng...\n");

	append_flag = back_flag = verb_flag = help_flag = err_flag = 0;

        /* Open the startup log */
	ident = "startup";
	log_opts = start_opts;
	if (log_opts & LOG_WX) {
		if (argv) ident = argv[argc+1];
		else log_opts &= ~LOG_WX;
	}
	log_open(ident,0,log_opts);

	dprintf(1,"common_init: argc: %d, argv: %p, add_opts: %p, log_opts: %x", argc, argv, add_opts, log_opts);

#ifdef __WIN32
	dprintf(1,"initializng winsock...\n");
	/* Initialize Winsock */
	iResult = WSAStartup(MAKEWORD(2,2), &wsaData);
	if (iResult != 0) {
		log_write(LOG_SYSERR,"WSAStartup");
		return 1;
	}
#endif

	/* If add_opts, add to std */
	opts = (add_opts ? opt_addopts(std_opts,add_opts) : std_opts);

	/* Init opts */
	opt_init(opts);

	/* Process the opts */
	dprintf(1,"common_init: processing opts...");
	error = opt_process(argc,argv,opts);

	/* If help flag, display usage and exit */
	dprintf(1,"common_init: error: %d, help_flag: %d",error,help_flag);
	if (!error && help_flag) {
		opt_usage(argv[0],opts);
		error = 1;
	}

	/* If add_opts, free malloc'd opts */
	if (add_opts) mem_free(opts);
	if (error) return 1;

	/* Set the requested flags */
	log_opts &= ~(LOG_DEBUG|LOG_DEBUG2|LOG_DEBUG3|LOG_DEBUG4);
#ifdef DEBUG
	dprintf(1,"debug: %d\n", debug);
	switch(debug) {
		case 4:
		default:
			log_opts |= LOG_DEBUG4;
			/* Fall-through */
		case 3:
			log_opts |= LOG_DEBUG3;
			/* Fall-through */
		case 2:
			log_opts |= LOG_DEBUG2;
			/* Fall-through */
		case 1:
			log_opts |= LOG_DEBUG;
			break;
	}
#endif
	dprintf(1,"common_init: verb_flag: %d",verb_flag);
	if (verb_flag) log_opts |= LOG_VERBOSE;
	if (err_flag) log_opts |= LOG_STDERR;
	if (append_flag) log_opts &= ~LOG_CREATE;
	else log_opts |= LOG_CREATE;

	/* If back_flag is set, become a daemon */
	if (back_flag) become_daemon();


	/* Re-open the startup log */
//	log_open(ident, 0 ,log_opts);

	/* Log to a file? */
	dprintf(1,"logfile: %s\n", logfile);
	if (strlen(logfile)) {
//		strcpy(logfile,os_fnparse(logfile,".log",0));
//		dprintf(1,"logfile: %s\n", logfile);
//		log_lock_id = os_lock_file(logfile,0);
//		if (!log_lock_id) return 1;
		log_opts |= LOG_TIME;
		file = logfile;
		log_opts &= ~LOG_WX;
		ident = "logfile";
	} else {
		file = 0;
	}

	/* Re-open log */
	log_open(ident,file,log_opts);

	/* Get paths */
	*SOLARD_BINDIR = *SOLARD_ETCDIR = *SOLARD_LIBDIR = *SOLARD_LOGDIR = 0;

	/* Look for configfile in ~/.sd.conf, and /etc/sd.conf, /usr/local/etc/sd.conf */
	*configfile = 0;
	if (gethomedir(home,sizeof(home)-1) == 0) {
		char path[256];

		sprintf(path,"%s/.sd.conf",home);
		fixpath(path,sizeof(path)-1);
		if (access(path,0) == 0) strcpy(configfile,path);
	}
#ifndef __WIN32
	if (access("/etc/sd.conf",R_OK) == 0) strcpy(configfile,"/etc/sd.conf");
	if (access("/usr/local/etc/sd.conf",R_OK) == 0) strcpy(configfile,"/usr/local/etc/sd.conf");
#endif

	cfg = 0;
	dprintf(6,"configfile: %s\n", configfile);
#if CFG
	if (strlen(configfile)) {
		cfg = cfg_read(configfile);
		if (!cfg) log_write(LOG_SYSERR,"cfg_read %s",configfile);
	}
#endif

	_getpath(cfg,home,SOLARD_BINDIR,"bin");
	_getpath(cfg,home,SOLARD_ETCDIR,"etc");
	_getpath(cfg,home,SOLARD_LIBDIR,"lib");
	_getpath(cfg,home,SOLARD_LOGDIR,"log");
#if CFG
	if (cfg) cfg_destroy(cfg);
#endif

	_setp("SOLARD_BINDIR",SOLARD_BINDIR);
	_setp("SOLARD_ETCDIR",SOLARD_ETCDIR);
	_setp("SOLARD_LIBDIR",SOLARD_LIBDIR);
	_setp("SOLARD_LOGDIR",SOLARD_LOGDIR);
	dprintf(6,"BINDIR: %s, ETCDIR: %s, LIBDIR: %s, LOGDIR: %s\n", SOLARD_BINDIR, SOLARD_ETCDIR, SOLARD_LIBDIR, SOLARD_LOGDIR);

	return 0;
}

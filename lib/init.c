
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

static int _getcfg(char *cf, char *dest, char *what) {
	cfg_info_t *cfg;
	char *p;
	int r;

//	dprintf(1,"cf: %s\n", cf);
	r = 1;
	cfg = cfg_read(cf);
	if (!cfg) {
		log_write(LOG_SYSERR,"cfg_read");
		goto _getcfg_err;
	}
	p = cfg_get_item(cfg,"",what);
	if (!p) {
		p = cfg_get_item(cfg,"","prefix");
		if (!p) goto _getcfg_err;
//		dprintf(1,"prefix: %s\n", p);
		sprintf(dest,"%s/%s",p,what);
	} else {
		strncat(dest,p,DIRPATH_LEN-1);
	}
//	dprintf(1,"dest: %s\n", dest);
	r = 0;
_getcfg_err:
	cfg_destroy(cfg);
//	dprintf(1,"returning: %d\n", r);
	return r;
}

static void _getpath(char *dest, char *what) {
	char home[246],temp[256];

	if (gethomedir(home,sizeof(home)-1) == 0) {
		sprintf(temp,"%s/.sd.conf",home);
		if (access(temp,0) == 0) {
			if (_getcfg(temp,dest,what) == 0)
				return;
		}
	}

	if (access("/etc/sd.conf",0) == 0) {
		if (_getcfg("/etc/sd.conf",dest,what) == 0)
			return;
	}

#ifdef SOLARD_PREFIX
	sprintf(dest,"%s/%s",SOLARD_PREFIX,what);
	return;
#endif

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
			printf("error: _getpath: no path set for: %s\n", what);
			dest = ".";
		}
		return;

	/* Not root */
	} else {
		sprintf(dest,"%s/%s",home,what);
		return;
	}
	printf("error: _getpath: no path set for: %s\n", what);
	dest = ".";
}

int solard_common_init(int argc,char **argv,opt_proctab_t *add_opts,int start_opts) {
	/* std command-line opts */
	static char logfile[256];
	static int append_flag;
	static int back_flag,verb_flag;
	static int help_flag,err_flag;
	static opt_proctab_t std_opts[] = {
		{ "-b|run in background",&back_flag,DATA_TYPE_LOGICAL,0,0,"no" },
		{ "-d:#|set debugging level",&debug,DATA_TYPE_INT,0,0,"0" },
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
	_getpath(SOLARD_BINDIR,"bin");
	_getpath(SOLARD_ETCDIR,"etc");
	_getpath(SOLARD_LIBDIR,"lib");
	_getpath(SOLARD_LOGDIR,"log");

	dprintf(1,"BINDIR: %s, ETCDIR: %s, LIBDIR: %s, LOGDIR: %s\n", SOLARD_BINDIR, SOLARD_ETCDIR, SOLARD_LIBDIR, SOLARD_LOGDIR);

	return 0;
}

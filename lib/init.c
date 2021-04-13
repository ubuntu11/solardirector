
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
	if (add_opts) {
		dprintf(1,"common_init: freeing opts...");
		mem_free(opts);
	}
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


	return 0;
}

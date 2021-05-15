
/*
Copyright (c) 2021, Stephen P. Shoecraft
All rights reserved.

This source code is licensed under the BSD-style license found in the
LICENSE file in the root directory of this source tree.
*/

#include "rdevserver.h"
#ifndef __WIN32
#include <sys/signal.h>
#include <sys/socket.h>
#include <netinet/in.h>
#endif

int server(rdev_config_t *conf, int port) {
	struct sockaddr_in sin;
	socklen_t sin_size;
	int c,pid,status;
	socket_t s;
#ifndef __WIN32
	sigset_t set;

	/* Ignore SIGPIPE */
	sigemptyset(&set);
	sigaddset(&set, SIGPIPE);
	sigprocmask(SIG_BLOCK, &set, NULL);
#endif

	dprintf(1,"opening socket...\n");
	s = socket(AF_INET, SOCK_STREAM, 0);
	if (s < 0)  {
		log_write(LOG_SYSERR,"socket");
		goto rdev_server_error;
	}
	status = 1;
	if (setsockopt(s, SOL_SOCKET, SO_REUSEADDR, (const void *)&status, sizeof(status)) < 0) {
		log_write(LOG_SYSERR,"setsockopt");
		goto rdev_server_error;
	}
	sin_size = sizeof(sin);
	memset(&sin,0,sin_size);

	sin.sin_family = AF_INET;
	sin.sin_addr.s_addr = htonl(INADDR_ANY);
	sin.sin_port = htons((unsigned short)port);

	dprintf(1,"binding...\n");
	if (bind(s, (struct sockaddr *) &sin, sizeof(sin)) < 0)  {
		log_write(LOG_SYSERR,"bind");
		goto rdev_server_error;
	}
	dprintf(1,"listening...\n");
	if (listen(s, 5) < 0) {
		log_write(LOG_SYSERR,"listen");
		goto rdev_server_error;
	}
	while(solard_check_state(conf,RDEV_STATE_RUNNING)) {
		dprintf(1,"accepting...\n");
		c = accept(s,(struct sockaddr *)&sin,&sin_size);
		if (c < 0) {
			log_write(LOG_SYSERR,"accept");
			goto rdev_server_error;
		}
		pid = fork();
		if (pid < 0) {
			log_write(LOG_SYSERR,"fork");
			goto rdev_server_error;
		} else if (pid == 0) {
			_exit(devserver(&conf->ds,c));
#if 0
		} else {
			dprintf(1,"waiting on pid...\n");
			waitpid(pid,&status,0);
			dprintf(1,"WIFEXITED: %d\n", WIFEXITED(status));
			if (WIFEXITED(status)) dprintf(1,"WEXITSTATUS: %d\n", WEXITSTATUS(status));
			status = (WIFEXITED(status) ? WEXITSTATUS(status) : 1);
			dprintf(1,"status: %d\n", status);
#endif
		}
	}
	if (conf->can) {
		/* Close the IF */
		if (conf->can->close(conf->can_handle)) {
			log_write(LOG_SYSERR,"can close");
			goto rdev_server_error;
		}
		solard_clear_state(conf,RDEV_STATE_OPEN);
	}
	return 0;
rdev_server_error:
	return 1;
}

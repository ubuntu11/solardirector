
/*
Copyright (c) 2021, Stephen P. Shoecraft
All rights reserved.

This source code is licensed under the BSD-style license found in the
LICENSE file in the root directory of this source tree.
*/

#include "rdevserver.h"
#include <sys/socket.h>
#include <netinet/in.h>
#include <sys/signal.h>

int server(siproxy_config_t *conf, int port) {
	struct sockaddr_in sin;
	socklen_t sin_size;
	sigset_t set;
	int s,c,pid,status;

	/* Ignore SIGPIPE */
	sigemptyset(&set);
	sigaddset(&set, SIGPIPE);
	sigprocmask(SIG_BLOCK, &set, NULL);

	dprintf(1,"opening socket...\n");
	s = socket(AF_INET, SOCK_STREAM, 0);
	if (s < 0)  {
		log_write(LOG_SYSERR,"socket");
		goto siproxy_server_error;
	}
	status = 1;
	if (setsockopt(s, SOL_SOCKET, SO_REUSEADDR, (const void *)&status, sizeof(status)) < 0) {
		log_write(LOG_SYSERR,"setsockopt");
		goto siproxy_server_error;
	}
	sin_size = sizeof(sin);
	bzero((char *) &sin, sin_size);

	sin.sin_family = AF_INET;
	sin.sin_addr.s_addr = htonl(INADDR_ANY);
	sin.sin_port = htons((unsigned short)port);

	dprintf(1,"binding...\n");
	if (bind(s, (struct sockaddr *) &sin, sizeof(sin)) < 0)  {
		log_write(LOG_SYSERR,"bind");
		goto siproxy_server_error;
	}
	dprintf(1,"listening...\n");
	if (listen(s, 5) < 0) {
		log_write(LOG_SYSERR,"listen");
		goto siproxy_server_error;
	}
	while(solard_check_state(conf,SI_STATE_RUNNING)) {
		dprintf(1,"accepting...\n");
		c = accept(s,(struct sockaddr *)&sin,&sin_size);
		if (c < 0) {
			log_write(LOG_SYSERR,"accept");
			goto siproxy_server_error;
		}
		pid = fork();
		if (pid < 0) {
			log_write(LOG_SYSERR,"fork");
			goto siproxy_server_error;
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
			goto siproxy_server_error;
		}
		solard_clear_state(conf,SI_STATE_OPEN);
	}
	return 0;
siproxy_server_error:
	return 1;
}

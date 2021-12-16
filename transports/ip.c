
/*
Copyright (c) 2021, Stephen P. Shoecraft
All rights reserved.

This source code is licensed under the BSD-style license found in the
LICENSE file in the root directory of this source tree.
*/

#include <sys/types.h>
#ifdef __WIN32
#include <winsock2.h>
#include <windows.h>
#include <ws2tcpip.h>
#else
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#endif
#include <fcntl.h>
#include <ctype.h>
#include "transports.h"

#define DEFAULT_PORT 23

#ifdef __WIN32
typedef SOCKET socket_t;
#define SOCKET_CLOSE(s) closesocket(s);
#else
typedef int socket_t;
#define SOCKET_CLOSE(s) close(s)
#define INVALID_SOCKET -1
#endif

#define dlevel 7

struct ip_session {
	socket_t sock;
	char target[SOLARD_TARGET_LEN];
	int port;
};
typedef struct ip_session ip_session_t;

static void *ip_new(void *conf, void *target, void *topts) {
	ip_session_t *s;
	char *p;

	s = calloc(1,sizeof(*s));
	if (!s) {
		perror("ip_new: malloc");
		return 0;
	}
	s->sock = INVALID_SOCKET;
	p = strchr((char *)target,':');
	if (p) *p = 0;
	strncat(s->target,(char *)target,SOLARD_TARGET_LEN-1);
	if (p) {
		p++;
		s->port = atoi(p);
	}
	if (!s->port) s->port = DEFAULT_PORT;
	dprintf(dlevel,"target: %s, port: %d\n", s->target, s->port);
	return s;
}

static int ip_open(void *handle) {
	ip_session_t *s = handle;
	struct sockaddr_in addr;
	socklen_t sin_size;
	struct hostent *he;
	char temp[SOLARD_TARGET_LEN];
	uint8_t *ptr;

	if (s->sock != INVALID_SOCKET) return 0;
	s->sock = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
	if (s->sock == INVALID_SOCKET) {
		log_write(LOG_SYSERR,"socket");
		return 1;
	}

	/* Try to resolve the target */
	he = (struct hostent *) 0;
	if (!is_ip(s->target)) {
		he = gethostbyname(s->target);
		dprintf(6,"he: %p\n", he);
		if (he) {
			ptr = (unsigned char *) he->h_addr;
			sprintf(temp,"%d.%d.%d.%d",ptr[0],ptr[1],ptr[2],ptr[3]);
		}
	}
	if (!he) strcpy(temp,s->target);
	dprintf(dlevel,"temp: %s\n",temp);

	memset(&addr,0,sizeof(addr));
	sin_size = sizeof(addr);
	addr.sin_family = AF_INET;
	addr.sin_addr.s_addr = inet_addr(temp);
	addr.sin_port = htons(s->port);
	dprintf(dlevel,"connecting...\n");
	if (connect(s->sock,(struct sockaddr *)&addr,sin_size) < 0) {
		lprintf(LOG_SYSERR,"connect to %s", s->target);
		return 1;
	}
	return 0;
}

static int ip_read(void *handle, void *buf, int buflen) {
	ip_session_t *s = handle;
	int bytes, bidx, num;
	struct timeval tv;
	fd_set rdset;

	dprintf(dlevel,"buf: %p, buflen: %d\n", buf, buflen);

	tv.tv_usec = 0;
	tv.tv_sec = 1;

	FD_ZERO(&rdset);

	if (s->sock == INVALID_SOCKET) return -1;

	dprintf(dlevel,"reading...\n");
	bidx=0;
	while(1) {
		FD_SET(s->sock,&rdset);
		num = select(s->sock+1,&rdset,0,0,&tv);
		dprintf(dlevel,"num: %d\n", num);
		if (!num) break;
		dprintf(dlevel,"buf: %p, bufen: %d\n", buf, buflen);
		bytes = recv(s->sock, buf, buflen, 0);
		dprintf(dlevel,"bytes: %d\n", bytes);
		if (bytes < 0) {
			if (errno == EAGAIN) continue;
			bidx = -1;
			break;
		} else if (bytes == 0) {
			break;
		} else if (bytes > 0) {
			buf += bytes;
			bidx += bytes;
			buflen -= bytes;
			if (buflen < 1) break;
		}
	}
#ifdef DEBUG
	if (debug >= dlevel+1) bindump("ip_read",buf,bidx);
#endif
	dprintf(dlevel,"returning: %d\n", bidx);
	return bidx;
}

static int ip_write(void *handle, void *buf, int buflen) {
	ip_session_t *s = handle;
	int bytes;

	dprintf(dlevel,"s->sock: %p\n", s->sock);

	if (s->sock == INVALID_SOCKET) return -1;
#ifdef DEBUG
	if (debug >= dlevel+1) bindump("ip_write",buf,buflen);
#endif
	bytes = send(s->sock, buf, buflen, 0);
	dprintf(dlevel,"bytes: %d\n", bytes);
	return bytes;
}


static int ip_close(void *handle) {
	ip_session_t *s = handle;

	if (s->sock != INVALID_SOCKET) {
		dprintf(dlevel,"closing...\n");
		SOCKET_CLOSE(s->sock);
		s->sock = INVALID_SOCKET;
	}
	return 0;
}

static int ip_config(void *h, int func, ...) {
	va_list ap;
	int r;

	r = 1;
	va_start(ap,func);
	switch(func) {
	default:
		dprintf(dlevel,"error: unhandled func: %d\n", func);
		break;
	}
	return r;
}

solard_driver_t ip_driver = {
	SOLARD_DRIVER_TRANSPORT,
	"ip",
	ip_new,
	0,
	ip_open,
	ip_close,
	ip_read,
	ip_write,
	ip_config
};


/*
Copyright (c) 2021, Stephen P. Shoecraft
All rights reserved.

This source code is licensed under the BSD-style license found in the
LICENSE file in the root directory of this source tree.
*/

#ifdef __WIN32
#include <winsock2.h>
#include <windows.h>
#include <ws2tcpip.h>
#else
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#endif
#include <fcntl.h>
#include <ctype.h>
#include "devserver.h"
#include "transports.h"

#define DEFAULT_PORT 3930

struct rdev_session {
	int fd;
	char target[SOLARD_TARGET_LEN];
	int port;
	char name[DEVSERVER_NAME_LEN];
	char type[DEVSERVER_TYPE_LEN];
	uint8_t unit;
};
typedef struct rdev_session rdev_session_t;

static void *rdev_new(void *conf, void *target, void *topts) {
	rdev_session_t *s;
	char temp[128];
	char *p;

	s = calloc(1,sizeof(*s));
	if (!s) {
		perror("rdev_new: malloc");
		return 0;
	}
	s->fd = -1;

	/* Format is addr:port,name */
	dprintf(1,"target: %s\n", target);
	temp[0] = 0;
	strncat(temp,strele(0,",",(char *)target),sizeof(temp)-1);
	strncat(s->target,strele(0,":",temp),sizeof(s->target)-1);
	dprintf(1,"s->target: %s\n", s->target);
	p = strele(1,":",temp);
	if (strlen(p)) s->port = atoi(p);
	if (!s->port) s->port = DEFAULT_PORT;

	if (!topts) {
		log_write(LOG_ERROR,"rdev requires name in topts\n");
		return 0;
	}
	strncat(s->name,strele(0,",",topts),sizeof(s->name)-1);
	if (!strlen(s->name)) {
		log_write(LOG_ERROR,"rdev requires name in topts\n");
		return 0;
	}

	dprintf(1,"target: %s, port: %d, name: %s\n", s->target, s->port, s->name);
	return s;
}

static int rdev_open(void *handle) {
	rdev_session_t *s = handle;
	struct sockaddr_in addr;
	socklen_t sin_size;
	struct hostent *he;
	uint8_t status;
	int bytes;
	char temp[SOLARD_TARGET_LEN];
	uint8_t *ptr;
	uint16_t control;

	if (s->fd >= 0) return 0;

	dprintf(1,"Creating socket...\n");
	s->fd = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
	if (s->fd < 0) {
		perror("rdev_open: socket");
		return 1;
	}

	/* Try to resolve the target */
	dprintf(1,"checking..\n");
	he = (struct hostent *) 0;
	if (!is_ip(s->target)) {
		he = gethostbyname(s->target);
		dprintf(1,"he: %p\n", he);
		if (he) {
			ptr = (unsigned char *) he->h_addr;
			sprintf(temp,"%d.%d.%d.%d",ptr[0],ptr[1],ptr[2],ptr[3]);
		}
	}
	if (!he) strcpy(temp,s->target);
	dprintf(1,"temp: %s\n",temp);

	dprintf(1,"Connecting to: %s\n",temp);
	memset(&addr,0,sizeof(addr));
	sin_size = sizeof(addr);
	addr.sin_family = AF_INET;
	addr.sin_addr.s_addr = inet_addr(temp);
	addr.sin_port = htons(s->port);
	if (connect(s->fd,(struct sockaddr *)&addr,sin_size) < 0) {
		perror("rdev_open: connect");
		return 1;
	}

	dprintf(1,"sending open\n");
	bytes = devserver_send(s->fd,DEVSERVER_OPEN,0,0,s->name,strlen(s->name)+1);
	dprintf(1,"bytes: %d\n", bytes);

	/* Read the reply */
	bytes = devserver_recv(s->fd,&status,&s->unit,&control,s->type,sizeof(s->type)-1,0);
	if (bytes < 0) return -1;
	dprintf(1,"bytes: %d, status: %d, unit: %d, control: %d\n", bytes, status, s->unit, control);
	dprintf(1,"type: %s\n", s->type);
	if (status != 0) {
		close(s->fd);
		s->fd = -1;
		return 1;
	}

	return 0;
}

static int rdev_read(void *handle, void *buf, int buflen) {
	rdev_session_t *s = handle;
	uint8_t status,runit;
	uint16_t control;
	int bytes;

	dprintf(5,"buf: %p, buflen: %d\n", buf, buflen);

	bytes = devserver_send(s->fd,DEVSERVER_READ,s->unit,buflen,0,0);
	dprintf(5,"bytes: %d\n", bytes);
	if (bytes < 0) return -1;

	/* Read the response */
	status = runit = control = 0;
	bytes = devserver_recv(s->fd,&status,&runit,&control,buf,buflen,0);
	dprintf(5,"bytes: %d, status: %d, runit: %d, control: %d\n", bytes, status, runit, control);
	/* control has bytes read */
	if (bytes > 0 && debug >= 7) bindump("FROM SERVER",buf,control);
	dprintf(5,"returning: %d\n", control);
	return control;
}

static int rdev_write(void *handle, void *buf, int buflen) {
	rdev_session_t *s = handle;
	uint8_t status,runit;
	uint16_t control;
	int bytes;

	dprintf(5,"buf: %p, buflen: %d\n", buf, buflen);

	if (debug >= 7) bindump("TO SERVER",buf,buflen);
	bytes = devserver_send(s->fd,DEVSERVER_WRITE,s->unit,buflen,buf,buflen);
	dprintf(5,"bytes: %d\n", bytes);
	if (bytes < 0) return -1;

	/* Read the response */
	status = runit = control = 0;
	bytes = devserver_recv(s->fd,&status,&runit,&control,0,0,0);
	/* control has bytes written */
	dprintf(5,"bytes: %d, status: %d, runit: %d\n", bytes, status, runit);
	dprintf(5,"returning: %d\n", control);
	return control;
}


static int rdev_close(void *handle) {
	rdev_session_t *s = handle;

	if (s->fd >= 0) {
		devserver_request(s->fd,DEVSERVER_CLOSE,s->unit,0,0,0);
		close(s->fd);
		s->fd = -1;
	}
	return 0;
}

static int rdev_config(void *h, int func, ...) {
	va_list ap;
	int r;

	r = 1;
	va_start(ap,func);
	switch(func) {
	default:
		dprintf(1,"error: unhandled func: %d\n", func);
		break;
	}
	return r;
}

solard_driver_t rdev_driver = {
	SOLARD_DRIVER_TRANSPORT,
	"rdev",
	rdev_new,
	rdev_open,
	rdev_close,
	rdev_read,
	rdev_write,
	rdev_config
};


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
#include "transports.h"
#include "rdev.h"

#define DEFAULT_PORT 3930
#define dlevel 7

#define RDEV_TARGET_LEN 128

struct rdev_session {
	int fd;
	char target[RDEV_TARGET_LEN];
	int port;
	char name[RDEV_NAME_LEN];
	char type[RDEV_TYPE_LEN];
	uint8_t unit;
};
typedef struct rdev_session rdev_session_t;

/*************************************** Global funcs ***************************************/

int rdev_send(socket_t fd, uint8_t opcode, uint8_t unit, uint16_t control, void *data, uint16_t data_len) {
	uint8_t header[8],*hptr;
	int bytes,sent;

	dprintf(dlevel,"fd: %d, opcode: %d, unit: %d, control: %d, data: %p, datasz: %d\n",
		fd, opcode, unit, control, data, data_len);
	if (fd < 0) return -1;

	sent = 0;

	/* Send the header */
	hptr = header;
	*hptr++ = opcode;
	*hptr++ = unit;
	*(uint16_t *)hptr = control;
	hptr += 2;
	*(uint16_t *)hptr = data_len;
	hptr += 2;
	if (debug >= dlevel+1) bindump("SENDING",header,hptr - header);
	bytes = send(fd,(char *)header,hptr - header, 0);
	dprintf(dlevel,"bytes: %d\n", bytes);
	if (bytes < 0) return -1;
	sent += bytes;

	/* Send the data */
	if (data_len) {
		if (debug >= dlevel+1) bindump("SENDING",data,data_len);
		bytes = send(fd,data,data_len,0);
		dprintf(dlevel,"bytes: %d\n", bytes);
		if (bytes < 0) return -1;
		sent += bytes;
	}

	dprintf(dlevel,"returning: %d\n", sent);
	return sent;
}

int rdev_recv(socket_t fd, uint8_t *opcode, uint8_t *unit, uint16_t *control, void *data, int datasz, int timeout) {
	int bytes,len,bytes_left;
	uint8_t ch;
	rdev_header_t h;
	register int i;

	dprintf(dlevel,"opcode: %d, unit: %d, control: %d, data: %p, datasz: %d, timeout: %d\n",
		*opcode, *unit, *control, data, datasz, timeout);

	if (timeout > 0) {
		struct timeval tv;
		fd_set fds;
		int num;

		FD_ZERO(&fds);
		FD_SET(fd,&fds);
		tv.tv_usec = 0;
		tv.tv_sec = 5;
		dprintf(dlevel,"waiting...\n");
		num = select(fd+1,&fds,0,0,&tv);
		dprintf(dlevel,"num: %d\n", num);
		if (num < 1) return -1;
	}

	/* Read the header */
	bytes = recv(fd, (char *)&h, sizeof(h), 0);
	dprintf(dlevel,"bytes: %d\n", bytes);
	if (bytes < 0) return -1;
	dprintf(dlevel,"header: opcode: %02x, unit: %d, control: %04x, len: %d\n", h.opcode, h.unit, h.control, h.len);
	if (debug >= dlevel+1) bindump("RECEIVED",&h,sizeof(h));

	/* Read the data */
	len = h.len;
	if (len > datasz) len = datasz;
	dprintf(dlevel,"len: %d\n",len);
	bytes_left = h.len - len;
	dprintf(dlevel,"bytes_left: %d\n", bytes_left);
	if (data && len) {
		bytes = recv(fd, data, len, 0);
		dprintf(dlevel,"bytes: %d\n", bytes);
		if (bytes < 0) return -1;
		if (debug >= dlevel+1) bindump("RECEIVED",data,bytes);
		/* XXX */
		if (bytes != len) return 0;
	}
	for(i=0; i < bytes_left; i++) recv(fd, (char *)&ch, 1, 0);
	*opcode = h.opcode;
	*unit = h.unit;
	*control = h.control;
	dprintf(dlevel,"returning: %d\n", bytes);
	return bytes;
}

int rdev_request(socket_t fd, uint8_t opcode, uint8_t unit, uint16_t control, void *data, int len) {
	uint8_t status,runit;
	int bytes;

	/* Send the req */
	dprintf(dlevel,"sending req\n");
	bytes = rdev_send(fd,opcode,unit,control,0,0);
	dprintf(dlevel,"bytes: %d\n", bytes);
	if (bytes < 0) return -1;

	/* Read the response */
	bytes = rdev_recv(fd,&status,&runit,&control,data,len,10);
	dprintf(dlevel,"bytes: %d, status: %d, runit: %d, control: %d\n", bytes, status, runit, control);
	if (bytes < 0) return -1;
	if (status != 0) return 0;
	return bytes;
}

/*************************************** Driver funcs ***************************************/

static void *rdev_new(void *conf, void *target, void *topts) {
	rdev_session_t *s;
	char temp[128];
	char *p;

	s = calloc(sizeof(*s),1);
	if (!s) {
		log_syserror("rdev_new: malloc");
		return 0;
	}
	s->fd = -1;

	/* Format is addr:port,name */
	dprintf(dlevel,"target: %s\n", target);
	temp[0] = 0;
	strncat(temp,strele(0,",",(char *)target),sizeof(temp)-1);
	strncat(s->target,strele(0,":",temp),sizeof(s->target)-1);
	dprintf(dlevel,"s->target: %s\n", s->target);
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

	dprintf(dlevel,"target: %s, port: %d, name: %s\n", s->target, s->port, s->name);
	return s;
}

#if 0
static int rdev_getip(char *ip, char *name) {
	int rc, err;
	void *tmp;
	struct hostent hbuf;
	struct hostent *result;
	char *buf;
	unsigned char *ptr;
	int len;

	dprintf(dlevel,"name: %s\n", name);

	len = 1024;
	buf = malloc(len);

	while ((rc = gethostbyname_r(name, &hbuf, buf, len, &result, &err)) == ERANGE) {
		len *= 2;
		tmp = realloc(buf, len);
		if (!tmp) {
			log_syserror("rdev_getip: realloc(buf,%d)",len);
			free(buf);
			return 1;
		} else {
			buf = tmp;
		}
	}
	free(buf);

	if (rc || !result) {
		log_syserror("rdev_getip: gethostbyname_r");
		return 1;
	}

	/* not found */
	dprintf(dlevel,"result->h_addr: %p\n", result->h_addr);
	if (!result->h_addr) return 1;

	ptr = (unsigned char *) result->h_addr;
	sprintf(ip,"%d.%d.%d.%d",ptr[0],ptr[1],ptr[2],ptr[3]);
	return 0;
}
#endif

static int rdev_open(void *handle) {
	rdev_session_t *s = handle;
	struct sockaddr_in addr;
	socklen_t sin_size;
//	struct hostent *he;
	uint8_t status;
	int bytes;
	char temp[SOLARD_TARGET_LEN];
//	uint8_t *ptr;
	uint16_t control;

	if (s->fd >= 0) return 0;

	dprintf(dlevel,"Creating socket...\n");
	s->fd = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
	if (s->fd < 0) {
		dprintf(dlevel,"rdev_open: socket");
		return 1;
	}

	/* Try to resolve the target */
	dprintf(dlevel,"target: %s\n",s->target);
	*temp = 0;
	if (os_gethostbyname(temp,sizeof(temp),s->target)) strcpy(temp,s->target);
	if (!*temp) strcpy(temp,s->target);
	dprintf(dlevel,"temp: %s\n",temp);

	dprintf(dlevel,"Connecting to: %s\n",temp);
	memset(&addr,0,sizeof(addr));
	sin_size = sizeof(addr);
	addr.sin_family = AF_INET;
	addr.sin_addr.s_addr = inet_addr(temp);
	addr.sin_port = htons(s->port);
	if (connect(s->fd,(struct sockaddr *)&addr,sin_size) < 0) {
		dprintf(dlevel,"rdev_open: connect");
		close(s->fd);
		s->fd = -1;
		return 1;
	}

	dprintf(dlevel,"sending open\n");
	bytes = rdev_send(s->fd,RDEV_OPCODE_OPEN,0,0,s->name,strlen(s->name)+1);
	dprintf(dlevel,"sent bytes: %d\n", bytes);

	/* Read the reply */
	bytes = rdev_recv(s->fd,&status,&s->unit,&control,s->type,sizeof(s->type)-1,10);
	if (bytes < 0) return -1;
	dprintf(dlevel,"recvd bytes: %d, status: %d, unit: %d, control: %d\n", bytes, status, s->unit, control);
	dprintf(dlevel,"type: %s\n", s->type);
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

	dprintf(dlevel,"buf: %p, buflen: %d\n", buf, buflen);

	bytes = rdev_send(s->fd,RDEV_OPCODE_READ,s->unit,buflen,0,0);
	dprintf(dlevel,"bytes sent: %d\n", bytes);
	if (bytes < 0) return -1;

	/* Read the response */
	status = runit = control = 0;
	bytes = rdev_recv(s->fd,&status,&runit,&control,buf,buflen,10);
	dprintf(dlevel,"bytes recvd: %d, status: %d, runit: %d, control: %d\n", bytes, status, runit, control);
	if (bytes < 0) return -1;
	/* control has bytes read */
	if (bytes > 0 && debug >= 7) bindump("FROM SERVER",buf,control);
	dprintf(dlevel,"returning: %d\n", control);
	return control;
}

static int rdev_write(void *handle, void *buf, int buflen) {
	rdev_session_t *s = handle;
	uint8_t status,runit;
	uint16_t control;
	int bytes;

	dprintf(dlevel,"buf: %p, buflen: %d\n", buf, buflen);

	if (debug >= 7) bindump("TO SERVER",buf,buflen);
	bytes = rdev_send(s->fd,RDEV_OPCODE_WRITE,s->unit,buflen,buf,buflen);
	dprintf(dlevel,"bytes: %d\n", bytes);
	if (bytes < 0) return -1;

	/* Read the response */
	status = runit = control = 0;
	bytes = rdev_recv(s->fd,&status,&runit,&control,0,0,10);
	dprintf(dlevel,"bytes recvd: %d, status: %d, runit: %d\n", bytes, status, runit);
	if (bytes < 0) return -1;
	/* control has bytes written */
	dprintf(dlevel,"returning: %d\n", control);
	return control;
}


static int rdev_close(void *handle) {
	rdev_session_t *s = handle;

	if (s->fd >= 0) {
		rdev_request(s->fd,RDEV_OPCODE_CLOSE,s->unit,0,0,0);
		close(s->fd);
		s->fd = -1;
	}
	return 0;
}

static int rdev_destroy(void *handle) {
        rdev_session_t *s = handle;

        if (s->fd >= 0) rdev_close(s);
	free(s);
	return 0;
}

static int rdev_config(void *h, int func, ...) {
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

solard_driver_t rdev_driver = {
	SOLARD_DRIVER_TRANSPORT,
	"rdev",
	rdev_new,
	rdev_destroy,
	rdev_open,
	rdev_close,
	rdev_read,
	rdev_write,
	rdev_config
};

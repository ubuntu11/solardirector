
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <fcntl.h>
#include <linux/can.h>
#include <ctype.h>
#include <netdb.h>
#include "devserver.h"
#include "module.h"
#include "utils.h"
#include "debug.h"

#define DEFAULT_PORT 3930

struct rdev_session {
	int fd;
	char target[SOLARD_AGENT_TARGET_LEN+1];
	int port;
	char interface[16];
	uint8_t unit;
};
typedef struct rdev_session rdev_session_t;

static void *rdev_new(void *conf, void *target, void *topts) {
	rdev_session_t *s;
	char *p;

	s = calloc(1,sizeof(*s));
	if (!s) {
		perror("rdev_new: malloc");
		return 0;
	}
	s->fd = -1;

	/* Format is addr:port:interface:speed */
	s->target[0] = 0;
	strncat(s->target,strele(0,",",(char *)target),sizeof(s->target)-1);
	p = strele(1,",",(char *)target);
	if (strlen(p)) s->port = atoi(p);
	else s->port = DEFAULT_PORT;
	p =strele(2,",",(char *)target);
	if (!p) {
		printf("error: rdev requires ip,port,interface,speed\n");
		return 0;
	}
	s->interface[0] = 0;
	strncat(s->interface,p,sizeof(s->interface)-1);

	dprintf(1,"target: %s, port: %d, interface: %s\n", s->target, s->port, s->interface);
	return s;
}

static int rdev_open(void *handle) {
	rdev_session_t *s = handle;
	struct sockaddr_in addr;
	socklen_t sin_size;
	struct hostent *he;
	struct timeval tv;
	uint8_t status;
	int bytes;
	char temp[SOLARD_AGENT_TARGET_LEN];
	uint8_t *ptr;

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

	dprintf(1,"setting timeout\n");
	tv.tv_sec = 1;
	tv.tv_usec = 0;
	if (setsockopt(s->fd, SOL_SOCKET, SO_RCVTIMEO, (const char *)&tv, sizeof(tv)) < 0) {
		perror("socket_open: setsockopt SO_SNDTIMEO");
		return 1;
	}

	dprintf(1,"sending open\n");
	bytes = devserver_send(s->fd,DEVSERVER_OPEN,0,s->interface,strlen(s->interface)+1);
	dprintf(1,"bytes: %d\n", bytes);

//	usleep (2500);

	/* Read the reply */
	bytes = devserver_recv(s->fd,&status,&s->unit,0,0,0);
	if (bytes < 0) return -1;
	dprintf(1,"bytes: %d, status: %d, unit: %d\n", bytes, status, s->unit);

	return 0;
}

static int rdev_read(void *handle, void *buf, int buflen) {
	rdev_session_t *s = handle;
	uint8_t status,runit;
	int bytes;

	dprintf(4,"buf: %p, buflen: %d\n", buf, buflen);

	bytes = devserver_send(s->fd,DEVSERVER_READ,s->unit,buf,buflen);
	dprintf(5,"bytes: %d\n", bytes);
	if (bytes < 0) return -1;

	/* Read the response */
	bytes = devserver_recv(s->fd,&status,&runit,buf,buflen,8000);
	dprintf(4,"bytes: %d, status: %d, runit: %d\n", bytes, status, runit);
	if (bytes > 0 && debug >= 8) bindump("FROM SERVER",buf,buflen);
	return bytes;
}

static int rdev_write(void *handle, void *buf, int buflen) {
	rdev_session_t *s = handle;
	uint8_t status,runit;
	int bytes;

	dprintf(5,"buf: %p, buflen: %d\n", buf, buflen);

	bytes = devserver_send(s->fd,DEVSERVER_WRITE,s->unit,buf,buflen);
	dprintf(5,"bytes: %d\n", bytes);
	if (bytes < 0) return -1;

	/* Read the response */
	bytes = devserver_recv(s->fd,&status,&runit,0,0,0);
	dprintf(5,"bytes: %d, status: %d, runit: %d\n", bytes, status, runit);
	return bytes;
}


static int rdev_close(void *handle) {
	rdev_session_t *s = handle;

	if (s->fd >= 0) {
		devserver_request(s->fd,DEVSERVER_CLOSE,s->unit,0,0);
		close(s->fd);
		s->fd = -1;
	}
	return 0;
}

solard_module_t rdev_module = {
	SOLARD_MODTYPE_TRANSPORT,
	"rdev",
	0,
	rdev_new,
	0,
	rdev_open,
	rdev_read,
	rdev_write,
	rdev_close
};

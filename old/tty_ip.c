
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <fcntl.h>
#include <linux/can.h>
#include <ctype.h>
#include <netdb.h>
#include "module.h"
#include "utils.h"
#include "debug.h"
#include "dsfuncs.h"

#define DEFAULT_PORT 3930

struct tty_ip_session {
	int fd;
	char target[SOLARD_AGENT_TARGET_LEN+1];
	int port;
	char interface[DEVSERVER_DEVICE_LEN+1];
	uint8_t unit;
	int speed,data,stop,parity;
};
typedef struct tty_ip_session tty_ip_session_t;

static void *tty_ip_new(void *conf, void *target, void *topts) {
	tty_ip_session_t *s;
	char temp[128];
	char *p;

	dprintf(1,"target: %p, topts: %p\n", target, topts);

	s = calloc(1,sizeof(*s));
	if (!s) {
		perror("tty_ip_new: malloc");
		return 0;
	}
	s->fd = -1;

	/* Format is addr:port,interface */
	temp[0] = 0;
	strncat(temp,strele(0,",",(char *)target),sizeof(temp)-1);
	strncat(s->target,strele(0,":",temp),sizeof(s->target)-1);
	p = strele(1,":",temp);
	if (!strlen(p)) s->port = 3900;
	else s->port = atoi(p);
	strncat(s->interface,strele(1,",",(char *)target),sizeof(s->interface)-1);

	/* Topts format is: speed,data,stop,parity */
	if (!topts) topts = "";
	p = strele(0,",",(char *)topts);
	if (strlen(p)) s->speed = atoi(p);
	else s->speed = 9600;

	p = strele(1,",",(char *)topts);
	if (strlen(p)) s->data = atoi(p);
	else s->data = 8;

	p = strele(2,",",(char *)topts);
	if (strlen(p)) {
		if (tolower(*p) == 'n') s->stop = 0;
		else s->stop = atoi(p);
	} else
		s->stop = 0;

	p = strele(3,",",(char *)topts);
	if (strlen(p)) s->parity = atoi(p);
	else s->parity = 1;
	return s;
}

static int tty_ip_open(void *handle) {
	tty_ip_session_t *s = handle;
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
		perror("tty_ip_open: socket");
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
		perror("tty_ip_open: connect");
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

	usleep (2500);

	/* Read the reply */
	bytes = devserver_recv(s->fd,&status,&s->unit,0,0,0);
	if (bytes < 0) return -1;
	dprintf(1,"bytes: %d, status: %d, unit: %d\n", bytes, status, s->unit);

	return 0;
}

static int tty_ip_read(void *handle, void *buf, int buflen) {
	tty_ip_session_t *s = handle;
	uint8_t status,runit;
	int bytes;

	dprintf(5,"s->fd: %d\n", s->fd);
	if (s->fd < 0) return -1;

	dprintf(5,"buf: %p, buflen: %d\n", buf, buflen);

	bytes = devserver_send(s->fd,DEVSERVER_READ,s->unit,buf,buflen);
	dprintf(5,"bytes: %d\n", bytes);
	if (bytes < 0) return -1;

	/* Read the response */
	bytes = devserver_recv(s->fd,&status,&runit,buf,buflen,1000);
	dprintf(1,"bytes: %d, status: %d, runit: %d\n", bytes, status, runit);
//	if (bytes > 0 && debug >= 8) bindump("FROM SERVER",buf,buflen);
	return bytes;
}

static int tty_ip_write(void *handle, void *buf, int buflen) {
	tty_ip_session_t *s = handle;
	uint8_t status,runit;
	int bytes;

	dprintf(1,"s->fd: %d\n", s->fd);
	if (s->fd < 0) return -1;

	dprintf(1,"buf: %p, buflen: %d\n", buf, buflen);

	bytes = devserver_send(s->fd,DEVSERVER_WRITE,s->unit,buf,buflen);
	dprintf(5,"bytes: %d\n", bytes);
	if (bytes < 0) return -1;

	/* Read the response */
	bytes = devserver_recv(s->fd,&status,&runit,0,0,0);
	dprintf(1,"bytes: %d, status: %d, runit: %d\n", bytes, status, runit);
	return bytes;
}


static int tty_ip_close(void *handle) {
	tty_ip_session_t *s = handle;

	if (s->fd >= 0) {
		devserver_request(s->fd,DEVSERVER_CLOSE,s->unit,0,0);
		close(s->fd);
		s->fd = -1;
	}
	return 0;
}

solard_module_t tty_ip_module = {
	SOLARD_MODTYPE_TRANSPORT,
	"tty_ip",
	0,
	tty_ip_new,
	0,
	tty_ip_open,
	tty_ip_read,
	tty_ip_write,
	tty_ip_close
};

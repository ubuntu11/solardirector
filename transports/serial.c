
/*
Copyright (c) 2021, Stephen P. Shoecraft
All rights reserved.

This source code is licensed under the BSD-style license found in the
LICENSE file in the root directory of this source tree.
*/

#include <fcntl.h> 
#include <termios.h>
#include <sys/ioctl.h>
#include <time.h>
#include <sys/select.h>
#include "module.h"
#include "utils.h"
#include "debug.h"

#define DEFAULT_SPEED 9600

struct serial_session {
	int fd;
	char target[SOLARD_AGENT_TARGET_LEN+1];
	int speed,data,stop,parity;
};
typedef struct serial_session serial_session_t;

static void *serial_new(void *conf, void *target, void *topts) {
	serial_session_t *s;
	char *p;

	s = calloc(1,sizeof(*s));
	if (!s) {
		perror("serial_new: malloc");
		return 0;
	}
	s->fd = -1;
	strncat(s->target,strele(0,",",(char *)target),sizeof(s->target)-1);

	/* Baud rate */
	p = strele(0,",",topts);
	s->speed = atoi(p);
	if (!s->speed) s->speed = DEFAULT_SPEED;

	/* Data bits */
	p = strele(1,",",topts);
	s->data = atoi(p);
	if (!s->data) s->data = 8;

	/* Parity */
	p = strele(2,",",topts);
	if (strlen(p)) {
		switch(p[0]) {
		case 'N':
		default:
			s->parity = 0;
			break;
		case 'E':
			s->parity = 1;
			break;
		case 'O':
			s->parity = 2;
			break;
		}
	}

	/* Stop */
	p = strele(3,",",topts);
	s->stop = atoi(p);
	if (!s->stop) s->stop = 1;

	dprintf(1,"target: %s, speed: %d, data: %d, parity: %c, stop: %d\n", s->target, s->speed, s->data, s->parity == 0 ? 'N' : s->parity == 2 ? 'O' : 'E', s->stop);

	return s;
}

static int set_interface_attribs (int fd, int speed, int data, int parity, int stop, int vmin, int vtime) {
	struct termios tty;
	int rate;

	dprintf(1,"fd: %d, speed: %d, data: %d, parity: %d, stop: %d, vmin: %d, vtime: %d\n",
		fd, speed, data, parity, stop, vmin, vtime);

	// Don't block
	fcntl(fd, F_SETFL, FNDELAY);

	// Get current device tty
	tcgetattr(fd, &tty);

#if 0
	// Set the transport port in RAW Mode (non cooked)
	tty.c_iflag &= ~(IGNBRK|BRKINT|PARMRK|ISTRIP|INLCR|IGNCR|ICRNL|IXON);
	tty.c_cflag |= (CLOCAL | CREAD);
	tty.c_lflag &= ~(ICANON | ECHO | ECHOE | ISIG);
	tty.c_oflag &= ~OPOST;
#else
	/* Or we can just use this :) */
	cfmakeraw(&tty);
#endif

	tty.c_cc[VMIN]  = vmin;
	tty.c_cc[VTIME] = vtime;

	/* Baud */
	switch(speed) {
	case 57600:     rate = B57600;   break;
	case 38400:     rate = B38400;   break;
	case 19200:     rate = B19200;   break;
	case 9600:      rate = B9600;    break;
	case 4800:      rate = B4800;    break;
	case 2400:      rate = B2400;    break;
	case 1200:      rate = B1200;    break;
	case 600:       rate = B600;      break;
	case 300:       rate = B300;      break;
	case 150:       rate = B150;      break;
	case 110:       rate = B110;      break;
	default:
		tty.c_cflag &= ~CBAUD;
		tty.c_cflag |= CBAUDEX;
		rate = speed;
		break;
	}
	cfsetispeed(&tty, rate);
	cfsetospeed(&tty, rate);

	/* Data bits */
	switch(data) {
		case 5: tty.c_cflag = (tty.c_cflag & ~CSIZE) | CS5; break;
		case 6: tty.c_cflag = (tty.c_cflag & ~CSIZE) | CS6; break;
		case 7: tty.c_cflag = (tty.c_cflag & ~CSIZE) | CS7; break;
		case 8: default: tty.c_cflag = (tty.c_cflag & ~CSIZE) | CS8; break;
	}

	/* Stop bits (0 = 1, non-zero = 2 */
	if (stop) tty.c_cflag |= CSTOPB;
	else tty.c_cflag &= ~CSTOPB;

	/* Parity (0=none, 1=even, 2=odd */
        tty.c_cflag &= ~(PARENB | PARODD);
	switch(parity) {
		case 1: tty.c_cflag |= PARENB; break;
		case 2: tty.c_cflag |= PARODD; break;
		default: break;
	}

	// No flow control
//	tty.c_cflag &= ~CRTSCTS;

	// do it
	tcsetattr(fd, TCSANOW, &tty);
	return 0;
}

static int serial_open(void *handle) {
	serial_session_t *s = handle;
	char path[256];

	if (s->fd >= 0) return 0;

	dprintf(1,"target: %s\n", s->target);
	strcpy(path,s->target);
	if ((access(path,0)) == 0) {
		if (strncmp(path,"/dev",4) != 0) {
			sprintf(path,"/dev/%s",s->target);
			dprintf(1,"path: %s\n", path);
		}
	}
	dprintf(1,"path: %s\n", path);
//	s->fd = open(path, O_RDWR | O_NOCTTY | O_NDELAY );
	s->fd = open(path, O_RDWR | O_NOCTTY);
	if (s->fd < 0) {
		log_write(LOG_SYSERR,"open %s\n", path);
		return 1;
	}
	set_interface_attribs(s->fd, s->speed, s->data, s->parity, s->stop, 0, 1);

	return 0;
}

static int serial_read(void *handle, void *buf, int buflen) {
	serial_session_t *s = handle;
	register uint8_t *p = buf;
	register int i;
//	uint8_t ch;
	int bytes, count;
//	int num, i;
//	struct timeval tv;
//	fd_set rfds;
	time_t start,cur;

	if (s->fd < 0) return -1;

	dprintf(8,"buf: %p, buflen: %d\n", buf, buflen);
	if (!buf || !buflen) return 0;

	time(&start);
	for(i=0; i < buflen; i++) {
		bytes = read(s->fd,p,1);
		dprintf(8,"bytes: %d\n", bytes);
		if (bytes < 0) {
//			dprintf(1,"errno: %d\n", errno);
//			if (errno == EAGAIN) continue;
			if (errno == EAGAIN) break;
			else return -1;
		}
		if (bytes == 0) break;
		time(&cur);
		if (cur - start >= 1) break;
		dprintf(8,"ch: %02X\n", *p);
		if (*p == 0x11 || *p == 0x13) {
			printf("***** p: %02X ******\n",*p);
			exit(0);
		}
		p++;
	}
	count = i;
	dprintf(8,"count: %d\n", count);

#if 0
	/* See if there's anything to read */
	count = 0;
	FD_ZERO(&rfds);
	FD_SET(s->fd,&rfds);
//	dprintf(8,"waiting...\n");
	tv.tv_usec = 500000;
	tv.tv_sec = 0;
	num = select(s->fd+1,&rfds,0,0,&tv);
//	dprintf(8,"num: %d\n", num);
	if (num < 1) goto serial_read_done;

	/* See how many bytes are waiting to be read */
	bytes = ioctl(s->fd, FIONREAD, &count);
//	dprintf(8,"bytes: %d\n", bytes);
	if (bytes < 0) {
		log_write(LOG_SYSERR|LOG_DEBUG,"serial_read: iotcl");
		return -1;
	}
//	dprintf(8,"count: %d\n", count);
	/* select said there was data yet there is none? */
	if (count == 0) goto serial_read_done;

	/* Read */
//	dprintf(8,"count: %d, buflen: %d\n",count,buflen);
	num = count > buflen ? buflen : count;
//	dprintf(8,"num: %d\n", num);
	bytes = read(s->fd, buf, num);
	if (bytes < 0) {
		log_write(LOG_SYSERR|LOG_DEBUG,"serial_read: read");
		return -1;
	}
	count = bytes;
#endif

//serial_read_done:
	if (debug >= 8 && count > 0) bindump("FROM DEVICE",buf,count);
	dprintf(8,"returning: %d\n", count);
	return count;
}

static int serial_write(void *handle, void *buf, int buflen) {
	serial_session_t *s = handle;
	int bytes;

	if (s->fd < 0) return -1;

	dprintf(8,"buf: %p, buflen: %d\n", buf, buflen);

	bytes = write(s->fd,buf,buflen);
	dprintf(8,"bytes: %d\n", bytes);
	if (debug >= 8 && bytes > 0) bindump("TO DEVICE",buf,buflen);
#if 0
	{
		int x = (buflen + 25) * 1000;
		printf("x: %d\n", x);
		usleep(x);
	}
//	usleep ((buflen + 25) * 10000);
#endif
	return bytes;
}

static int serial_close(void *handle) {
	serial_session_t *s = handle;

	dprintf(1,"fd: %d\n",s->fd);
	if (s->fd >= 0) {
		close(s->fd);
		s->fd = -1;
	}
	return 0;
}

solard_module_t serial_module = {
	SOLARD_MODTYPE_TRANSPORT,
	"serial",
	0,
	serial_new,
	0,
	serial_open,
	serial_read,
	serial_write,
	serial_close
};

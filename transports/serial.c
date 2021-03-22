
#include <fcntl.h> 
#include <termios.h>
#include <sys/ioctl.h>
#include "module.h"
#include "utils.h"
#include "debug.h"

#define DEFAULT_SPEED 19200

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
	s->stop = 8;
	s->parity = 1;

	p = strchr((char *)target,',');
	if (p) {
		*p++ = 0;
		s->speed = atoi(p);
	} else {
		s->speed = DEFAULT_SPEED;
	}
	dprintf(1,"target: %s, speed: %d, stop: %d, parity: %d\n", s->target, s->speed, s->stop, s->parity);

	return s;
}

#if 0
static int set_interface_attribs (int fd, int speed, int data, int parity, int stop, int vmin, int vtime) {
        struct termios tty;
	int rate;

	fcntl(fd, F_SETFL, FNDELAY);

        if (tcgetattr (fd, &tty) != 0) {
                printf("error %d from tcgetattr\n", errno);
                return -1;
        }

#if 0
	tty.c_cflag &= ~CBAUD;
	tty.c_cflag |= CBAUDEX;
//	tty.c_ispeed = speed;
//	tty.c_ospeed = speed;
	cfsetspeed (&tty, speed);
#endif

      //set the right baud rate
	dprintf(1,"speed: %d\n", speed);
      switch(speed)
           {
         case 57600:   rate = B57600;   break;
         case 38400:   rate = B38400;   break;
         case 19200:   rate = B19200;   break;
         case 9600:    rate = B9600;    break;
         case 4800:    rate = B4800;    break;
         case 2400:       rate = B2400;    break;
         case 1200:    rate = B1200;    break;
         case 600:        rate = B600;      break;
         case 300:        rate = B300;      break;
         case 150:        rate = B150;      break;
         case 110:        rate = B110;      break;
         default:      rate = B1200;
           }
      cfsetispeed(&tty, rate);
      cfsetospeed(&tty, rate);

	switch(data) {
	case 5:
        	tty.c_cflag = (tty.c_cflag & ~CSIZE) | CS5;
		break;
	case 6:
        	tty.c_cflag = (tty.c_cflag & ~CSIZE) | CS6;
		break;
	case 7:
        	tty.c_cflag = (tty.c_cflag & ~CSIZE) | CS7;
		break;
	case 8:
	default:
        	tty.c_cflag = (tty.c_cflag & ~CSIZE) | CS8;
		break;
	}
	/* set RAW Mode (non cooked) */
	tty.c_iflag &= ~(IGNBRK|BRKINT|PARMRK|ISTRIP|INLCR|IGNCR|ICRNL|IXON);
	tty.c_lflag &= ~(ICANON | ECHO | ECHOE | ISIG);
	tty.c_oflag &= ~OPOST;
        tty.c_oflag = 0;                // no remapping, no delays
        tty.c_cc[VMIN]  = vmin;		// min num of chars before returning
        tty.c_cc[VTIME] = vtime;	// useconds to wait for a char

        tty.c_iflag &= ~(IXON | IXOFF | IXANY); // shut off xon/xoff ctrl

        tty.c_cflag |= (CLOCAL | CREAD);// ignore modem controls,
                                        // enable reading
        tty.c_cflag &= ~(PARENB | PARODD);      // shut off parity
        tty.c_cflag |= parity;
        tty.c_cflag &= ~CSTOPB;		/* No stop bits */
        tty.c_cflag |= stop;
        tty.c_cflag &= ~CRTSCTS; /* No CTS */

        if (tcsetattr (fd, TCSANOW, &tty) != 0) {
		printf("error %d from tcsetattr\n", errno);
                return -1;
        }
        return 0;
}
#else
static int set_interface_attribs (int fd, int speed, int data, int parity, int stop, int vmin, int vtime) {
	struct termios options;
	int rate;

	// don't block
	fcntl(fd, F_SETFL, FNDELAY);

	// get current device options
	tcgetattr(fd, &options);

	// set the transport port in RAW Mode (non cooked)
	options.c_iflag &= ~(IGNBRK|BRKINT|PARMRK|ISTRIP|INLCR|IGNCR|ICRNL|IXON);
	options.c_cflag |= (CLOCAL | CREAD);
	options.c_lflag &= ~(ICANON | ECHO | ECHOE | ISIG);
	options.c_oflag &= ~OPOST;
	options.c_cc[VMIN]  = 0;
	options.c_cc[VTIME] = 5;

	//set the right baud rate
	dprintf(1,"speed: %d\n", speed);
	switch(speed)
	   {
	 case 57600:   rate = B57600;   break;
	 case 38400:   rate = B38400;   break;
	 case 19200:   rate = B19200;   break;
	 case 9600:    rate = B9600;    break;
	 case 4800:    rate = B4800;    break;
	 case 2400:       rate = B2400;    break;
	 case 1200:    rate = B1200;    break;
	 case 600:        rate = B600;      break;
	 case 300:        rate = B300;      break;
	 case 150:        rate = B150;      break;
	 case 110:        rate = B110;      break;
	 default:      rate = B1200;
	   }
	cfsetispeed(&options, rate);
	cfsetospeed(&options, rate);

	// 8 bits, no paraty, sone stop bit
	options.c_cflag &= ~PARENB;
	options.c_cflag &= ~CSTOPB;
	options.c_cflag &= ~CSIZE;
	options.c_cflag |= CS8;

	// no handshake
	options.c_cflag &= ~CRTSCTS;

	// set new parameter
	tcsetattr(fd, TCSANOW, &options);

	return 0;
}
#endif

static int serial_open(void *handle) {
	serial_session_t *s = handle;
	char path[256];

//	if (s->fd >= 0) return 0;

	dprintf(1,"target: %s\n", s->target);
	strcpy(path,s->target);
	if ((access(path,0)) == 0) {
		if (strncmp(path,"/dev",4) != 0) {
			sprintf(path,"/dev/%s",s->target);
			dprintf(1,"path: %s\n", path);
		}
	}
	dprintf(1,"path: %s\n", path);
	s->fd = open(path, O_RDWR | O_NOCTTY | O_NDELAY );
	if (s->fd < 0) {
		log_write(LOG_SYSERR,"open %s\n", path);
		return 1;
	}
	set_interface_attribs(s->fd, s->speed, s->data, s->parity, s->stop, 1, 5);
//	setport(s->fd, s->speed);

	return 0;
}

static int serial_read(void *handle, void *buf, int buflen) {
	serial_session_t *s = handle;
	int bytes, bidx, num;
//	uint8_t *p;
	struct timeval tv;
	fd_set rdset;

	if (s->fd < 0) return -1;

	dprintf(7,"buf: %p, buflen: %d\n", buf, buflen);

	tv.tv_usec = 2500;
	tv.tv_sec = 0;

#if 1
	bidx = 0;
	FD_ZERO(&rdset);
	FD_SET(s->fd,&rdset);
	dprintf(5,"waiting...\n");
	num = select(s->fd+1,&rdset,0,0,&tv);
	dprintf(5,"num: %d\n", num);
	if (num < 1) goto serial_read_done;

	bidx = ioctl(s->fd, FIONREAD, &bytes);
	dprintf(5,"bidx: %d\n", bidx);
	if (bidx < 0) goto serial_read_done;
	dprintf(5,"bytes: %d\n", bytes);
	/* select said there was data yet there is none? */
	if (bytes == 0) goto serial_read_done;

	//limit the read for the maximum destination buffer size
	dprintf(5,"bytes: %d, buflen: %d\n",bytes,buflen);
	num = buflen > bytes ? bytes : buflen;
	dprintf(7,"num: %d\n", num);
	bidx = read(s->fd, buf, num);
#else
	FD_ZERO(&rdset);
	dprintf(3,"reading...\n");
	bidx=0;
	p = buf;
	while(1) {
		FD_SET(s->fd,&rdset);
		num = select(s->fd+1,&rdset,0,0,&tv);
		dprintf(3,"num: %d\n", num);
		if (!num) break;
		dprintf(3,"p: %p, bufen: %d\n", p, buflen);
		bytes = read(s->fd, p, buflen);
		dprintf(3,"bytes: %d\n", bytes);
		if (bytes < 0) {
			bidx = -1;
			break;
		} else if (bytes == 0) {
			break;
		} else if (bytes > 0) {
			p += bytes;
			bidx += bytes;
			buflen -= bytes;
			if (buflen < 1) break;
		}
	}
#endif

serial_read_done:
	if (debug >= 7 && bidx > 0) bindump("FROM DEVICE",buf,bidx);
	dprintf(7,"returning: %d\n", bidx);
	return bidx;
}

static int serial_write(void *handle, void *buf, int buflen) {
	serial_session_t *s = handle;
	int bytes;

	if (s->fd < 0) return -1;

	dprintf(7,"buf: %p, buflen: %d\n", buf, buflen);

	bytes = write(s->fd,buf,buflen);
	dprintf(7,"bytes: %d\n", bytes);
	if (debug >= 7 && bytes > 0) bindump("TO DEVICE",buf,buflen);
	usleep ((buflen + 25) * 100);
	return bytes;
}

static int serial_close(void *handle) {
	serial_session_t *s = handle;

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

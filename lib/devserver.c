
#include "common.h"
#include "devserver.h"

#if 0
#include <string.h>
#include <stdint.h>
#include <unistd.h>
#include "utils.h"
#include "dsfuncs.h"
#include "debug.h"
#include "devserver.h"
#include <sys/types.h> 
#include <sys/wait.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <linux/can.h>
#include <linux/can/raw.h>
#include <sys/signal.h>
#include <pthread.h>
#include <unistd.h>
#include <errno.h>
#include "dsfuncs.h"
#include "utils.h"
#include "debug.h"
#endif

struct devserver_funcs {
	uint8_t opcode;
	char *label;
	int (*func)(devserver_config_t *,int,uint8_t,uint8_t *,int);
};

#define PACKET_START	0xD5
#define PACKET_END	0xEB
#ifdef CHKSUM
#define PACKET_MIN	7
#else
#define PACKET_MIN	5
#endif

#ifdef CHKSUM
#define _getshort(p) (uint16_t)(*(p) | (*((p)+1) << 8))
static uint16_t chksum(uint8_t *data, uint16_t datasz) {
	uint16_t sum;
	register int i;

	sum = 0;
	for(i=0; i < datasz/2; i += 2) sum += (data[i] | data[i] << 8);
	if (i < datasz) sum += data[i];
	return sum;
}
#endif

int devserver_send(int fd, uint8_t opcode, uint8_t unit, void *data, int datasz) {
	uint8_t pkt[256];
	int i,bytes;

	dprintf(5,"fd: %d, opcode: %d, unit: %d, data: %p, datasz: %d\n", fd, opcode, unit, data, datasz);
	if (fd < 0) return -1;

	i = 0;
	pkt[i++] = PACKET_START;
	pkt[i++] = opcode;
	pkt[i++] = unit;
	pkt[i++] = datasz;
//	devserver_put32(&pkt[i],datasz);
//	i += 2;
	if (data && datasz) {
		memcpy(&pkt[i],data,datasz);
		i += datasz;
	}
#ifdef CHKSUM
	devserver_putshort(&pkt[i],chksum(data,datasz));
	i += 2;
#endif
	pkt[i++] = PACKET_END;

	if (debug >= 5) bindump("<<<",pkt,i);
	bytes = write(fd, pkt, i);
	dprintf(5,"bytes: %d\n", bytes);
	return bytes;
}

int devserver_recv(int fd, uint8_t *opcode, uint8_t *unit, void *data, int datasz, int timeout) {
	uint8_t ch,len;
	int i,bytes,bytes_left;
	uint8_t pkt[256];

	/* Read the start */
	do {
		bytes = read(fd,&ch,1);
		dprintf(5,"bytes: %d, start: %02x\n", bytes, ch);
		if (bytes < 0) {
			return -1;
		} else if (bytes == 0)
			return 0;
	} while(ch != PACKET_START);
	i=0;
	pkt[i++] = ch;

	/* Read opcode */
	bytes = read(fd,opcode,1);
	dprintf(5,"bytes: %d, opcode: %d\n", bytes, *opcode);
	if (bytes < 1) return -1;
	pkt[i++] = *opcode;

	/* Read unit */
	bytes = read(fd,unit,1);
	dprintf(5,"bytes: %d, unit: %d\n", bytes, *unit);
	if (bytes < 1) return -1;
	pkt[i++] = *unit;

	/* Read length */
	bytes = read(fd,&len,1);
	dprintf(5,"bytes: %d, len: %d\n", bytes, len);
	if (bytes < 1) return -1;
	pkt[i++] = len;

	/* Read the data */
	dprintf(5,"data: %p, datasz: %d\n", data, datasz);
	bytes_left = len;
	if (data && datasz && len) {
		if (len > datasz) len = datasz;
		bytes = read(fd,data,len);
		dprintf(5,"bytes: %d\n", bytes);
		if (bytes < 1) return -1;
		bytes_left -= len;
		memcpy(&pkt[i],data,len);
		i += len;
	}
	/* Drain any remaining data */
	dprintf(5,"bytes_left: %d\n", bytes_left);
	while(bytes_left--) {
		read(fd,&ch,1);
		pkt[i++] = ch;
	}

#ifdef CHKSUM
	{
		uint16_t sum,psum;
		char temp[2];

		/* Read the checksum */
		bytes = read(fd,&temp,2);
		dprintf(5,"bytes: %d\n", bytes);
		if (bytes < 1) return -1;
		psum = _getshort(temp);
		sum = chksum(data,len);
		dprintf(5,"sum: %02x, psum: %02x\n", sum, psum);
		pkt[i++] = temp[0];
		pkt[i++] = temp[1];
	}
#endif

	/* Read the end mark */
	bytes = read(fd,&ch,1);
	dprintf(5,"bytes: %d, ch: %02x\n", bytes, ch);
	if (bytes < 1) return -1;
	if (ch != PACKET_END) return -1;
	pkt[i++] = ch;

	if (debug >= 5) bindump(">>>",pkt,i);
	dprintf(5,"returning: %d\n", len);
	return len;
}

int devserver_request(int fd, uint8_t opcode, uint8_t unit, void *data, int len) {
	uint8_t status,runit;
	int bytes;

	/* Send the req */
	dprintf(7,"sending req\n");
	bytes = devserver_send(fd,opcode,unit,0,0);
	dprintf(7,"bytes: %d\n", bytes);
	if (bytes < 0) return -1;

	/* Read the response */
	bytes = devserver_recv(fd,&status,&runit,data,len,1000);
	dprintf(7,"bytes: %d, status: %d, runit: %d\n", bytes, status, runit);
	if (bytes < 0) return -1;
	if (status != 0) return 0;
	return bytes;
}

int devserver_reply(int fd, uint8_t status, uint8_t unit, uint8_t *data, int len) {
	int bytes;

	bytes = devserver_send(fd,status,unit,data,len);
	dprintf(7,"bytes: %d\n", bytes);
	return (bytes < 0 ? 1 : 0);
}

int devserver_error(int fd, uint8_t code) {
	int bytes;

	bytes = devserver_send(fd,code,0,0,0);
	dprintf(7,"bytes: %d\n", bytes);
	return (bytes < 0 ? 1 : 0);
}
int devserver_open(devserver_config_t *conf, int c, uint8_t unit, uint8_t *data, int len) {
	devserver_io_t *io;
	int i;

	dprintf(1,"data: %s\n", (char *)data);
	io = 0;
	for(i=0; i < conf->count; i++) {
		if (strcmp(conf->units[i].name,(char *)data) == 0) {
			io = &conf->units[i];
			unit = i;
			break;
		}
	}
	if (!io) goto devserver_open_error;
	if (!io->open) return devserver_reply(c,DEVSERVER_SUCCESS,unit,0,0);
	if (io->open(io->handle)) goto devserver_open_error;
	return devserver_reply(c,DEVSERVER_SUCCESS,unit,0,0);
devserver_open_error:
	return devserver_error(c,errno);
}

int devserver_close(devserver_config_t *conf, int c, uint8_t unit, uint8_t *data, int len) {
	devserver_io_t *io;

	if (unit < 0 || unit > conf->count) return devserver_error(c,EBADF);

	io = &conf->units[unit];
	if (!io->close) return devserver_reply(c,DEVSERVER_SUCCESS,unit,0,0);
	if (io->close(io->handle)) return devserver_error(c,errno);
	return devserver_reply(c,DEVSERVER_SUCCESS,unit,0,0);
}


int devserver_read(devserver_config_t *conf, int c, uint8_t unit, uint8_t *data, int len) {
	devserver_io_t *io;
	int bytes;

	if (unit < 0 || unit > conf->count) return devserver_error(c,EBADF);
	io = &conf->units[unit];
	bytes = io->read(io->handle, data, len);
	return bytes < 0 ? devserver_error(c,errno) : devserver_reply(c,DEVSERVER_SUCCESS,unit,data,bytes);
}

int devserver_write(devserver_config_t *conf, int c, uint8_t unit, uint8_t *data, int data_len) {
	devserver_io_t *io;
	int bytes;

	dprintf(1,"unit: %d, data: %p, len: %d\n", unit, data, data_len);

	if (unit < 0 || unit > conf->count) return devserver_error(c,EBADF);
	io = &conf->units[unit];
	bytes = io->write(io->handle, data, data_len);
	dprintf(1,"bytes: %d\n", bytes);
	return bytes < 0 ? devserver_error(c,errno) : devserver_reply(c,DEVSERVER_SUCCESS,unit,data,bytes);
}

static struct devserver_funcs devserver_funcs[] = {
	{ DEVSERVER_OPEN, "open", devserver_open },
	{ DEVSERVER_CLOSE, "close", devserver_close },
	{ DEVSERVER_READ, "read", devserver_read },
	{ DEVSERVER_WRITE, "write", devserver_write },
};
#define DEVSERVER_FUNCS_COUNT (sizeof(devserver_funcs)/sizeof(struct devserver_funcs))

int devserver_add_unit(devserver_config_t *conf, devserver_io_t *io) {
	dprintf(1,"unit[%d]: name: %s, device: %p\n", conf->count, io->name, io->device);
	conf->units[conf->count] = *io;
	conf->count++;
	return 0;
}

int devserver(devserver_config_t *conf, int c) {
	uint8_t data[32],opcode,unit;
	int i,bytes,found;

	while(1) {
		dprintf(5,"waiting for command...\n");
		bytes = devserver_recv(c,&opcode,&unit,data,sizeof(data),-1);
		dprintf(5,"bytes: %d\n", bytes);
		if (bytes < 1) break;
		dprintf(5,"opcode: %d, unit: %d\n", opcode, unit);
		found = 0;
		for(i=0; i < DEVSERVER_FUNCS_COUNT; i++) {
			dprintf(5,"funcs[%d].opcode: %d\n", i, devserver_funcs[i].opcode);
			if (devserver_funcs[i].opcode == opcode) {
				found = 1;
				dprintf(5,"calling %s\n", devserver_funcs[i].label);
				if (devserver_funcs[i].func(conf,c,unit,data,bytes) != 0) return 1;
				break;
			}
		}
		if (!found) {
			dprintf(2,"not found, sending error\n");
			devserver_error(c,ENOENT);
		}
	}
	/* XXX since we're exiting when we return we dont need to close anything */
#if 0
	/* Close any open units */
	for(i=0; i < conf->count; i++) {
		if (conf->units[i].close && conf->units[i].handle) {
			dprintf(1,"closing unit %s...\n",conf->units[i].name);
			conf->units[i].close(conf->units[i].handle);
		}
	}
#endif
	dprintf(1,"closing socket\n");
	close(c);
	dprintf(1,"done!\n");
	return 0;
}

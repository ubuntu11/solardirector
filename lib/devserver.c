
/*
Copyright (c) 2021, Stephen P. Shoecraft
All rights reserved.

This source code is licensed under the BSD-style license found in the
LICENSE file in the root directory of this source tree.
*/

#include "common.h"
#include "devserver.h"
#include <linux/can.h>

//#define devserver_get16(p) (uint16_t)(*(p) | (*((p)+1) << 8))
#define devserver_get16(p) (uint16_t)(*(p) | (*((p)+1) << 8))
#define devserver_put16(p,v) { float tmp; *(p) = ((uint16_t)(tmp = (v))); *((p)+1) = ((uint16_t)(tmp = (v)) >> 8); }
#define devserver_get32(p) (uint16_t)(*(p) | (*((p)+1) << 8) | (*((p)+2) << 16) | (*((p)+3) << 24))
#define devserver_put32(p,v) *(p) = ((int)(v) & 0xFF); *((p)+1) = ((int)(v) >> 8) & 0xFF; *((p)+2) = ((int)(v) >> 16) & 0xFF; *((p)+3) = ((int)(v) >> 24) & 0xFF

struct devserver_funcs {
	uint8_t opcode;
	char *label;
	int (*func)(devserver_config_t *,int,uint8_t,uint16_t,uint8_t *,int);
};

#define PACKET_START	0xD5
#define PACKET_END	0xEB
#ifdef CHKSUM
#define PACKET_MIN	7
#else
#define PACKET_MIN	5
#endif

#ifdef CHKSUM
static uint16_t chksum(uint8_t *data, uint16_t datasz) {
	uint16_t sum;
	register int i;

	sum = 0;
	for(i=0; i < datasz/2; i += 2) sum += (data[i] | data[i] << 8);
	if (i < datasz) sum += data[i];
	return sum;
}
#endif

int devserver_send(int fd, uint8_t opcode, uint8_t unit, uint16_t control, void *data, int datasz) {
	uint8_t pkt[256];
	int i,bytes,len;

	dprintf(5,"fd: %d, opcode: %d, unit: %d, control: %d, data: %p, datasz: %d\n",
		fd, opcode, unit, control, data, datasz);
	if (fd < 0) return -1;

	i = 0;
	/* byte 0: start */
	pkt[i++] = PACKET_START;
	/* byte 1: opcode */
	pkt[i++] = opcode;
	/* byte 2: unit # */
	pkt[i++] = unit;
	/* byte 3+4: control code */
	devserver_put16(&pkt[i],control);
	i += 2;
	/* byte 5: data length */
	len = datasz > 255 ? 255 : datasz;
	pkt[i++] = len;
//	devserver_put16(&pkt[i],datasz);
//	i += 2;
//	devserver_put32(&pkt[i],datasz);
//	i += 4;
	/* byte 6+: data */
	if (data && datasz) {
		memcpy(&pkt[i],data,datasz);
		i += datasz;
	}
#ifdef CHKSUM
	/* byte n: checksum */
	devserver_putshort(&pkt[i],chksum(data,datasz));
	i += 2;
#endif
	/* byte n: end */
	pkt[i++] = PACKET_END;

	if (debug >= 5) bindump("SENDING",pkt,i);
	bytes = write(fd, pkt, i);
	dprintf(5,"bytes: %d\n", bytes);
	return bytes;
}

int devserver_recv(int fd, uint8_t *opcode, uint8_t *unit, uint16_t *control, void *data, int datasz, int timeout) {
	int i,bytes,len,bytes_left;
	uint8_t ch,pkt[256];
//	struct timeval tv,*tp;
//	fd_set rdset;

	dprintf(5,"opcode: %d, unit: %d, control: %d, data: %p, datasz: %d, timeout: %d\n",
		*opcode, *unit, *control, data, datasz, timeout);

#if 0
//	tv.tv_usec = 0;
//	tv.tv_sec = 2;
	tv.tv_usec = timeout;
	tv.tv_sec = 0;
	tp = timeout >= 0 ? &tv : 0;

	FD_ZERO(&rdset);
	FD_SET(fd,&rdset);

	bytes = select(fd+1,&rdset,0,0,tp);
	dprintf(5,"bytes: %d\n", bytes);
	if (bytes < 1) return bytes;

#if 0
	i = ioctl(s->fd, FIONREAD, &bytes);
	dprintf(5,"ioctl status: %d\n", i);
	if (i < 0) return -1;
	dprintf(5,"bytes: %d\n", bytes);
	/* select said there was data yet there is none? */
	if (bytes == 0) return 0;
#endif

	bytes = read(fd,&ch,1);
	dprintf(5,"bytes: %d, ch: %02x\n", bytes, ch);
	if (bytes < 0) perror("read");
#if 0
	/* Read the start */
	do {
		bytes = select(fd+1,&rdset,0,0,tp);
		dprintf(5,"bytes: %d\n", bytes);
		if (bytes < 1) return bytes;

		dprintf(1,"reading...\n");
		bytes = read(fd,&ch,1);
		dprintf(5,"bytes: %d, ch: %02x\n", bytes, ch);
		if (bytes < 1) return bytes;
	} while(ch != PACKET_START);
#endif
#else
	bytes = read(fd,&ch,1);
	dprintf(5,"bytes: %d, ch: %02x\n", bytes, ch);
	if (bytes < 0) perror("read");
#endif
	if (ch != PACKET_START) return -1;
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

	/* Read control */
	bytes = read(fd,&pkt[i],2);
	dprintf(5,"bytes: %d\n", bytes);
	if (bytes < 1) return -1;
	*control = devserver_get16(&pkt[i]);
	i += 2;
	dprintf(5,"control: %d\n", *control);

	/* Read length */
	bytes = read(fd,&pkt[i],1);
	dprintf(5,"bytes: %d\n", bytes);
	if (bytes < 1) return -1;
	len = pkt[i++];
//	len = devserver_get16(&pkt[i]);
//	i += 2;
//	len = devserver_get32(&pkt[i]);
//	i += 4;
	dprintf(5,"len: %d\n", len);

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
	dprintf(5,"good packet!\n");

	if (debug >= 5) bindump("RECEIVED",pkt,i);
	dprintf(5,"opcode: %d, unit: %d, control: %d, data: %p, dataaz: %d, timeout: %d\n",
		*opcode, *unit, *control, data, datasz, timeout);
	/* Return total bytes written into buffer */
	dprintf(5,"returning: %d\n", 1);
	return 1;
}

int devserver_request(int fd, uint8_t opcode, uint8_t unit, uint16_t control, void *data, int len) {
	uint8_t status,runit;
	int bytes;

	/* Send the req */
	dprintf(7,"sending req\n");
	bytes = devserver_send(fd,opcode,unit,control,0,0);
	dprintf(7,"bytes: %d\n", bytes);
	if (bytes < 0) return -1;

	/* Read the response */
	bytes = devserver_recv(fd,&status,&runit,&control,data,len,1000);
	dprintf(7,"bytes: %d, status: %d, runit: %d, control: %d\n", bytes, status, runit, control);
	if (bytes < 0) return -1;
	if (status != 0) return 0;
	return bytes;
}

/***************************************************************************************/

/* Send a reply */
static int devserver_reply(int fd, uint8_t status, uint8_t unit, uint16_t control, uint8_t *data, int len) {
	int bytes;

	bytes = devserver_send(fd,status,unit,control,data,len);
	dprintf(7,"bytes: %d\n", bytes);
	return (bytes < 0 ? 1 : 0);
}

/* Send error reply */
int devserver_error(int fd, uint8_t code) {
	int bytes;

	bytes = devserver_send(fd,code,0,0,0,0);
	dprintf(7,"bytes: %d\n", bytes);
	return (bytes < 0 ? 1 : 0);
}

int devserver_open(devserver_config_t *conf, int c, uint8_t unit, uint16_t control, uint8_t *data, int len) {
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
	if (!io->open) return devserver_reply(c,DEVSERVER_SUCCESS,unit,control,0,0);
	if (io->open(io->handle)) goto devserver_open_error;
	return devserver_reply(c,DEVSERVER_SUCCESS,unit,control,(uint8_t *)io->type,strlen(io->type)+1);
devserver_open_error:
	return devserver_error(c,errno);
}

int devserver_close(devserver_config_t *conf, int c, uint8_t unit, uint16_t control, uint8_t *data, int len) {
	devserver_io_t *io;

	if (unit < 0 || unit > conf->count) return devserver_error(c,EBADF);

	io = &conf->units[unit];
	if (!io->close) return devserver_reply(c,DEVSERVER_SUCCESS,unit,control,0,0);
	if (io->close(io->handle)) return devserver_error(c,errno);
	return devserver_reply(c,DEVSERVER_SUCCESS,unit,control,0,0);
}


int devserver_read(devserver_config_t *conf, int c, uint8_t unit, uint16_t control, uint8_t *data, int datasz) {
	devserver_io_t *io;
	int bytes;

	dprintf(5,"unit: %d, control: %d, data: %p, len: %d\n", unit, control, data, datasz);
	if (unit < 0 || unit > conf->count) return devserver_error(c,EBADF);

	io = &conf->units[unit];
	/* Control has bytes to read or can id */
	if (strcmp(io->type,"can")==0)
		bytes = io->read(io->handle, data, control);
	else
		bytes = io->read(io->handle, data, control > 255 ? 255 : 1);
	/* control has bytes read */
	return bytes < 0 ? devserver_error(c,errno) : devserver_reply(c,DEVSERVER_SUCCESS,unit,bytes,data,bytes);
}

int devserver_write(devserver_config_t *conf, int c, uint8_t unit, uint16_t control, uint8_t *data, int datasz) {
	devserver_io_t *io;
	int bytes;

	dprintf(5,"unit: %d, control: %d, data: %p, len: %d\n", unit, control, data, datasz);
	if (unit < 0 || unit > conf->count) return devserver_error(c,EBADF);

	io = &conf->units[unit];
	/* control has bytes to write */
	if (strcmp(io->type,"can")==0)
		bytes = io->write(io->handle, data, sizeof(struct can_frame));
	else
		bytes = io->write(io->handle, data, control > 255 ? 255 : control);
	dprintf(5,"bytes: %d\n", bytes);
	/* control has bytes written */
	return bytes < 0 ? devserver_error(c,errno) : devserver_reply(c,DEVSERVER_SUCCESS,unit,bytes,data,bytes);
}

static struct devserver_funcs devserver_funcs[] = {
	{ DEVSERVER_OPEN, "open", devserver_open },
	{ DEVSERVER_CLOSE, "close", devserver_close },
	{ DEVSERVER_READ, "read", devserver_read },
	{ DEVSERVER_WRITE, "write", devserver_write },
};
#define DEVSERVER_FUNCS_COUNT (sizeof(devserver_funcs)/sizeof(struct devserver_funcs))

int devserver_add_unit(devserver_config_t *conf, devserver_io_t *io) {
	dprintf(1,"unit[%d]: name: %s\n", conf->count, io->name);
	conf->units[conf->count] = *io;
	dprintf(1,"new unit: %d\n", conf->count);
	conf->count++;
	return 0;
}

int devserver(devserver_config_t *conf, int c) {
	/* our max data size is 255 */
	uint8_t data[255],opcode,unit;
	uint16_t control;
	int i,bytes,found;

	while(1) {
		dprintf(5,"waiting for command...\n");
		/* recv returns the number of bytes actually read from server, not size of data */
		opcode = unit = control = 0;
		bytes = devserver_recv(c,&opcode,&unit,&control,data,sizeof(data),-1);
		dprintf(5,"bytes: %d\n", bytes);
		if (bytes < 1) break;
		dprintf(5,"opcode: %d, unit: %d\n", opcode, unit);
		found = 0;
		for(i=0; i < DEVSERVER_FUNCS_COUNT; i++) {
			dprintf(5,"funcs[%d].opcode: %d\n", i, devserver_funcs[i].opcode);
			if (devserver_funcs[i].opcode == opcode) {
				found = 1;
				dprintf(5,"calling %s\n", devserver_funcs[i].label);
				if (devserver_funcs[i].func(conf,c,unit,control,data,sizeof(data)) != 0) return 1;
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

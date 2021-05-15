
/*
Copyright (c) 2021, Stephen P. Shoecraft
All rights reserved.

This source code is licensed under the BSD-style license found in the
LICENSE file in the root directory of this source tree.
*/

#include "common.h"
#include "devserver.h"

#include <sys/types.h>
#ifndef __WIN32
#include <sys/socket.h>
#endif

struct __attribute__((packed, aligned(1))) devserver_header {
	union {
		uint8_t opcode;
		uint8_t status;
	};
	uint8_t unit;
	uint16_t control;
	uint16_t len;
};

struct devserver_funcs {
	uint8_t opcode;
	char *label;
	int (*func)(devserver_config_t *,int,uint8_t,uint16_t,uint8_t *,int);
};

#define dlevel 7

int devserver_send(socket_t fd, uint8_t opcode, uint8_t unit, uint16_t control, void *data, uint16_t data_len) {
	uint8_t header[8],*hptr;
	int bytes,sent;

	dprintf(dlevel,"fd: %d, opcode: %d, unit: %d, control: %d, data: %p, datasz: %d\n",
		fd, opcode, unit, control, data, data_len);
	if (fd < 0) return -1;

	sent = 0;
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
	if (debug >= 5) bindump("SENDING",data,data_len);
	bytes = send(fd,data,data_len,0);
	dprintf(dlevel,"bytes: %d\n", bytes);
	if (bytes < 0) return -1;
	sent += bytes;

	dprintf(dlevel,"returning: %d\n", sent);
	return sent;
}

int devserver_recv(socket_t fd, uint8_t *opcode, uint8_t *unit, uint16_t *control, void *data, int datasz, int timeout) {
	uint8_t *frame,*fptr;
	int bytes,len,bytes_left;
	uint8_t ch,pkt[256],*dptr;
	struct devserver_header h;
	register int i,j;

//	dprintf(dlevel,"opcode: %d, unit: %d, control: %d, data: %p, datasz: %d, timeout: %d\n",
//		*opcode, *unit, *control, data, datasz, timeout);

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
	dprintf(dlevel,"returning: %d\n", bytes);
	*opcode = h.opcode;
	*unit = h.unit;
	*control = h.control;
	dprintf(5,"returning: %d\n", bytes);
	return bytes;
}

int devserver_request(socket_t fd, uint8_t opcode, uint8_t unit, uint16_t control, void *data, int len) {
	uint8_t status,runit;
	int bytes;

	/* Send the req */
	dprintf(dlevel,"sending req\n");
	bytes = devserver_send(fd,opcode,unit,control,0,0);
	dprintf(dlevel,"bytes: %d\n", bytes);
	if (bytes < 0) return -1;

	/* Read the response */
	bytes = devserver_recv(fd,&status,&runit,&control,data,len,1000);
	dprintf(dlevel,"bytes: %d, status: %d, runit: %d, control: %d\n", bytes, status, runit, control);
	if (bytes < 0) return -1;
	if (status != 0) return 0;
	return bytes;
}

/***************************************************************************************/

/* Send a reply */
static int devserver_reply(socket_t fd, uint8_t status, uint8_t unit, uint16_t control, uint8_t *data, int len) {
	int bytes;

	bytes = devserver_send(fd,status,unit,control,data,len);
	dprintf(dlevel,"bytes: %d\n", bytes);
	return (bytes < 0 ? 1 : 0);
}

/* Send error reply */
int devserver_error(socket_t fd, uint8_t code) {
	int bytes;

	bytes = devserver_send(fd,code,0,0,0,0);
	dprintf(dlevel,"bytes: %d\n", bytes);
	return (bytes < 0 ? 1 : 0);
}

int devserver_open(devserver_config_t *conf, int c, uint8_t unit, uint16_t control, uint8_t *data, int len) {
	devserver_io_t *io;
	int i;

	dprintf(dlevel,"data: %s\n", (char *)data);

	io = 0;
	dprintf(dlevel,"conf->count: %d\n", conf->count);
	for(i=0; i < conf->count; i++) {
		dprintf(dlevel,"units[%d].name: %s\n", i, conf->units[i].name);
		if (strcmp(conf->units[i].name,(char *)data) == 0) {
			dprintf(dlevel,"found!\n");
			io = &conf->units[i];
			unit = i;
			break;
		}
	}
	errno = ENOENT;
	if (!io) {
		dprintf(dlevel,"NOT found!\n");
		return devserver_error(c,ENOENT);
	}
	dprintf(dlevel,"io->open: %p\n", io->open);
	if (!io->open) return devserver_reply(c,DEVSERVER_SUCCESS,unit,control,0,0);
	dprintf(dlevel,"io->handle: %p\n", io->handle);
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

	dprintf(dlevel,"unit: %d, control: %d, data: %p, len: %d\n", unit, control, data, datasz);
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
	int bytes,len;

	dprintf(dlevel,"unit: %d, control: %d, data: %p, len: %d\n", unit, control, data, datasz);
	if (unit < 0 || unit > conf->count) return devserver_error(c,EBADF);

	io = &conf->units[unit];
	/* control has bytes to write */
	if (strcmp(io->type,"can")==0)
		bytes = io->write(io->handle, data, sizeof(struct can_frame));
	else {
		len = control > datasz ? datasz : control;
		dprintf(dlevel,"len: %d\n",len);
		bytes = io->write(io->handle, data, control > datasz ? datasz : control);
	}
	dprintf(dlevel,"bytes: %d\n", bytes);
	/* control has bytes written */
	return bytes < 0 ? devserver_error(c,errno) : devserver_reply(c,DEVSERVER_SUCCESS,unit,bytes,0,0);
}

static struct devserver_funcs devserver_funcs[] = {
	{ DEVSERVER_OPEN, "open", devserver_open },
	{ DEVSERVER_CLOSE, "close", devserver_close },
	{ DEVSERVER_READ, "read", devserver_read },
	{ DEVSERVER_WRITE, "write", devserver_write },
};
#define DEVSERVER_FUNCS_COUNT (sizeof(devserver_funcs)/sizeof(struct devserver_funcs))

int devserver_add_unit(devserver_config_t *conf, devserver_io_t *io) {
	dprintf(dlevel,"unit[%d]: name: %s\n", conf->count, io->name);
	conf->units[conf->count] = *io;
	dprintf(dlevel,"new unit: %d\n", conf->count);
	conf->count++;
	return 0;
}

int devserver(devserver_config_t *conf, int c) {
	/* our max data size is 255 */
	uint8_t data[255],opcode,unit;
	uint16_t control;
	int i,bytes,found;

	while(1) {
		dprintf(dlevel,"waiting for command...\n");
		/* recv returns the number of bytes actually read from server, not size of data */
		opcode = unit = control = 0;
		bytes = devserver_recv(c,&opcode,&unit,&control,data,sizeof(data),-1);
		dprintf(dlevel,"bytes: %d\n", bytes);
		if (bytes < 1) break;
		dprintf(dlevel,"opcode: %d, unit: %d\n", opcode, unit);
		found = 0;
		for(i=0; i < DEVSERVER_FUNCS_COUNT; i++) {
			dprintf(dlevel,"funcs[%d].opcode: %d\n", i, devserver_funcs[i].opcode);
			if (devserver_funcs[i].opcode == opcode) {
				found = 1;
				dprintf(dlevel,"calling %s\n", devserver_funcs[i].label);
				if (devserver_funcs[i].func(conf,c,unit,control,data,sizeof(data)) != 0) return 1;
				break;
			}
		}
		if (!found) {
			dprintf(dlevel,"not found, sending error\n");
			devserver_error(c,ENOENT);
		}
	}
	/* XXX since we're exiting when we return we dont need to close anything */
#if 0
	/* Close any open units */
	for(i=0; i < conf->count; i++) {
		if (conf->units[i].close && conf->units[i].handle) {
			dprintf(dlevel,"closing unit %s...\n",conf->units[i].name);
			conf->units[i].close(conf->units[i].handle);
		}
	}
#endif
	dprintf(dlevel,"closing socket\n");
	close(c);
	dprintf(dlevel,"done!\n");
	return 0;
}

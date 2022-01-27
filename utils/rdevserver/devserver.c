
/*
Copyright (c) 2021, Stephen P. Shoecraft
All rights reserved.

This source code is licensed under the BSD-style license found in the
LICENSE file in the root directory of this source tree.
*/

#include "common.h"
#include "rdevserver.h"

/* needed for threaded server */
struct devserver_context {
	/* shared */
	pthread_mutex_t lock;
	devserver_config_t conf[DEVSERVER_MAX_UNITS];
	/* thread local */
	devserver_io_t *opened[DEVSERVER_MAX_UNITS];
};
typedef struct devserver_context devserver_context_t;

struct devserver_funcs {
	uint8_t opcode;
	char *label;
	int (*func)(devserver_context_t *,int,uint8_t,uint16_t,uint8_t *,int);
};

#define dlevel 1

/* Send a reply */
static int devserver_reply(socket_t fd, uint8_t status, uint8_t unit, uint16_t control, uint8_t *data, int len) {
	int bytes;

	bytes = rdev_send(fd,status,unit,control,data,len);
	dprintf(dlevel,"bytes sent: %d\n", bytes);
	return (bytes < 0 ? 1 : 0);
}

/* Send error reply */
int devserver_error(socket_t fd, uint8_t code) {
	int bytes;

	bytes = rdev_send(fd,code,0,0,0,0);
	dprintf(dlevel,"bytes sent: %d\n", bytes);
	return (bytes < 0 ? 1 : 0);
}

static int devserver_open(devserver_context_t *ctx, int c, uint8_t unit, uint16_t control, uint8_t *data, int len) {
	devserver_io_t *io;
	int i,r;

	dprintf(dlevel,"data: %s\n", (char *)data);

	io = 0;
	pthread_mutex_lock(&ctx->lock);
	dprintf(dlevel,"ctx->conf->count: %d\n", ctx->conf->count);
	for(i=0; i < ctx->conf->count; i++) {
		dprintf(dlevel,"units[%d].name: %s\n", i, ctx->conf->units[i].name);
		if (strcmp(ctx->conf->units[i].name,(char *)data) == 0) {
			dprintf(dlevel,"found!\n");
			io = &ctx->conf->units[i];
			unit = i;
			break;
		}
	}
	if (!io) {
		dprintf(dlevel,"NOT found!\n");
		pthread_mutex_unlock(&ctx->lock);
		return devserver_error(c,ENOENT);
	}
	dprintf(dlevel,"opened: %p\n", ctx->opened[unit]);
	if (ctx->opened[unit]) {
		pthread_mutex_unlock(&ctx->lock);
		return devserver_error(c,EBUSY);
	}
	dprintf(dlevel,"io->open: %p\n", io->open);
	if (!io->open) {
		pthread_mutex_unlock(&ctx->lock);
		return devserver_reply(c,RDEV_STATUS_SUCCESS,unit,control,0,0);
	}
	dprintf(dlevel,"io->handle: %p\n", io->handle);
	r = io->open(io->handle);
	dprintf(dlevel,"r: %d\n", r);
	if (!r) {
		ctx->opened[unit] = io;
		dprintf(0,"opened unit %s\n", io->name);
	}
	pthread_mutex_unlock(&ctx->lock);
	if (r) return devserver_error(c,errno);
	else return devserver_reply(c,RDEV_STATUS_SUCCESS,unit,control,(uint8_t *)io->type,strlen(io->type)+1);
}

static int devserver_close(devserver_context_t *ctx, int c, uint8_t unit, uint16_t control, uint8_t *data, int len) {
	devserver_io_t *io;
	int r;

	if (unit < 0 || unit > ctx->conf->count) return devserver_error(c,EBADF);

	pthread_mutex_lock(&ctx->lock);
	io = ctx->opened[unit];
	dprintf(dlevel,"io: %p\n", io);
	if (!io) {
		pthread_mutex_unlock(&ctx->lock);
		return devserver_error(c,EBADF);
	}
	if (!io->close) {
		pthread_mutex_unlock(&ctx->lock);
		return devserver_reply(c,RDEV_STATUS_SUCCESS,unit,control,0,0);
	}
	r = io->close(io->handle);
	pthread_mutex_unlock(&ctx->lock);
	if (!r) ctx->opened[unit] = 0;
	if (r) return devserver_error(c,errno);
	else return devserver_reply(c,RDEV_STATUS_SUCCESS,unit,control,0,0);
}


static int devserver_read(devserver_context_t *ctx, int c, uint8_t unit, uint16_t control, uint8_t *data, int datasz) {
	devserver_io_t *io;
	int bytes;

	dprintf(dlevel,"unit: %d, control: %d, data: %p, len: %d\n", unit, control, data, datasz);
	dprintf(dlevel,"unit: %d, count: %d\n", unit, ctx->conf->count);
	if (unit < 0 || unit > ctx->conf->count) return devserver_error(c,EBADF);

	io = ctx->opened[unit];
	dprintf(dlevel,"io: %p\n", io);
	if (!io) {
		pthread_mutex_unlock(&ctx->lock);
		return devserver_error(c,EBADF);
	}
	/* Control has bytes to read or can id */
	dprintf(1,"type: %s\n", io->type);
	if (strcmp(io->type,"can")==0)
		bytes = io->read(io->handle, data, control);
	else
		bytes = io->read(io->handle, data, control > 255 ? 255 : 1);
	dprintf(1,"bytes read: %d\n", bytes);
	/* control has bytes read */
	return bytes < 0 ? devserver_error(c,errno) : devserver_reply(c,RDEV_STATUS_SUCCESS,unit,bytes,data,bytes);
}

static int devserver_write(devserver_context_t *ctx, int c, uint8_t unit, uint16_t control, uint8_t *data, int datasz) {
	devserver_io_t *io;
	int bytes,len;

	dprintf(dlevel,"unit: %d, control: %d, data: %p, len: %d\n", unit, control, data, datasz);
	if (unit < 0 || unit > ctx->conf->count) return devserver_error(c,EBADF);

	io = ctx->opened[unit];
	dprintf(dlevel,"io: %p\n", io);
	if (!io) {
		pthread_mutex_unlock(&ctx->lock);
		return devserver_error(c,EBADF);
	}
	/* control has bytes to write */
	if (strcmp(io->type,"can")==0)
		bytes = io->write(io->handle, data, sizeof(struct can_frame));
	else {
		len = control > datasz ? datasz : control;
		dprintf(dlevel,"len: %d\n",len);
		bytes = io->write(io->handle, data, control > datasz ? datasz : control);
	}
	dprintf(dlevel,"bytes written: %d\n", bytes);
	/* control has bytes written */
	return bytes < 0 ? devserver_error(c,errno) : devserver_reply(c,RDEV_STATUS_SUCCESS,unit,bytes,0,0);
}

static struct devserver_funcs devserver_funcs[] = {
	{ RDEV_OPCODE_OPEN, "open", devserver_open },
	{ RDEV_OPCODE_CLOSE, "close", devserver_close },
	{ RDEV_OPCODE_READ, "read", devserver_read },
	{ RDEV_OPCODE_WRITE, "write", devserver_write },
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
	devserver_context_t ctx;
	/* our max data size is 255 */
	uint8_t data[255],opcode,unit;
	uint16_t control;
	int i,bytes,found,error;

	dprintf(dlevel,"************************************ DEVSERVER START *************************************\n");

	dprintf(dlevel,"c: %d, count: %d\n", c, conf->count);

	memset(&ctx,0,sizeof(ctx));
	memcpy(ctx.conf,conf,sizeof(*conf));

	error = 0;
	while(!error) {
		dprintf(dlevel,"waiting for command...\n");
		/* recv returns the number of bytes actually read from server, not size of data */
		opcode = unit = control = 0;
		bytes = rdev_recv(c,&opcode,&unit,&control,data,sizeof(data),-1);
		dprintf(dlevel,"bytes: %d\n", bytes);
		if (bytes < 1) break;
		dprintf(dlevel,"opcode: %d, unit: %d\n", opcode, unit);
		found = 0;
		for(i=0; i < DEVSERVER_FUNCS_COUNT; i++) {
			dprintf(dlevel,"funcs[%d].opcode: %d, name: %s\n", i, devserver_funcs[i].opcode,
				devserver_funcs[i].label);
			if (devserver_funcs[i].opcode == opcode) {
				found = 1;
				dprintf(dlevel,"calling %s\n", devserver_funcs[i].label);
				if (devserver_funcs[i].func(&ctx,c,unit,control,data,sizeof(data)) != 0) error = 1;
				break;
			}
		}
		if (!found) {
			dprintf(dlevel,"not found, sending error\n");
			devserver_error(c,ENOENT);
		}
	}
	/* Only close units we've opened */
	/* Close any open units */
	for(i=0; i < DEVSERVER_MAX_UNITS; i++) {
		dprintf(dlevel,"opened[%d]: %d\n", i, ctx.opened[i]);
		if (ctx.opened[i] && conf->units[i].close && conf->units[i].handle) {
			dprintf(0,"closing unit %s...\n",conf->units[i].name);
			conf->units[i].close(conf->units[i].handle);
		}
	}
	dprintf(dlevel,"closing socket\n");
	close(c);
	dprintf(dlevel,"done!\n");
	dprintf(dlevel,"************************************ DEVSERVER END *************************************\n");
	return 0;
}

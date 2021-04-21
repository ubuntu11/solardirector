
/*
Copyright (c) 2021, Stephen P. Shoecraft
All rights reserved.

This source code is licensed under the BSD-style license found in the
LICENSE file in the root directory of this source tree.
*/

#include "common.h"
#include "devserver.h"

#include <sys/types.h>
#ifdef __WIN32
#include <winsock2.h>
#include <windows.h>
#include <ws2tcpip.h>
#else
#include <sys/socket.h>
#endif


#ifdef __WIN32
typedef SOCKET socket_t;
#define SOCKET_CLOSE(s) closesocket(s);
#else
typedef int socket_t;
#define SOCKET_CLOSE(s) close(s)
#define INVALID_SOCKET -1
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

#if 0
//#define devserver_get16(p) (uint16_t)(*(p) | (*((p)+1) << 8))
#define devserver_get16(p) (uint16_t)(*(p) | (*((p)+1) << 8))
#define devserver_put16(p,v) { float tmp; *(p) = ((uint16_t)(tmp = (v))); *((p)+1) = ((uint16_t)(tmp = (v)) >> 8); }
#define devserver_get32(p) (uint16_t)(*(p) | (*((p)+1) << 8) | (*((p)+2) << 16) | (*((p)+3) << 24))
#define devserver_put32(p,v) *(p) = ((int)(v) & 0xFF); *((p)+1) = ((int)(v) >> 8) & 0xFF; *((p)+2) = ((int)(v) >> 16) & 0xFF; *((p)+3) = ((int)(v) >> 24) & 0xFF
#endif

struct devserver_funcs {
	uint8_t opcode;
	char *label;
	int (*func)(devserver_config_t *,int,uint8_t,uint16_t,uint8_t *,int);
};

#if 0
#define PACKET_START		0xD5
#define PACKET_END		0xEB
#define PACKET_ESC		0xAA
#define PACKET_ESC_START 	0x01
#define PACKET_ESC_END		0x02
#define PACKET_ESC_ESC		0x03
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

#if 0
static uint8_t *_frame_add(uint8_t *frame, uint8_t *data, int len) {
	register int i;

	for(i=0; i < len; i++) {
		if (data[i] == PACKET_START || data[i] == PACKET_END || data[i] == PACKET_ESC) {
			dprintf(1,"****** ESCAPING *****\n");
			*frame++ = PACKET_ESC;
			switch(data[i]) {
			case PACKET_START:
				*frame++ = PACKET_ESC_START;
				break;
			case PACKET_END:
				*frame++ = PACKET_ESC_END; 
				break;
			case PACKET_ESC:
				*frame++ = PACKET_ESC_ESC; 
				break;
			}
		} else {
			*frame++ = data[i];
		}
	}
	return frame;
}
#endif

int devserver_send(socket_t fd, uint8_t opcode, uint8_t unit, uint16_t control, void *data, uint16_t data_len) {
	uint8_t header[8],*hptr;
	int bytes,sent;

	dprintf(5,"fd: %d, opcode: %d, unit: %d, control: %d, data: %p, datasz: %d\n",
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
	if (debug >= 5) bindump("SENDING",header,hptr - header);
	bytes = send(fd,header,hptr - header, 0);
	dprintf(5,"bytes: %d\n", bytes);
	if (bytes < 0) return -1;
	sent += bytes;
	if (debug >= 5) bindump("SENDING",data,data_len);
	bytes = send(fd,data,data_len,0);
	dprintf(5,"bytes: %d\n", bytes);
	if (bytes < 0) return -1;
	sent += bytes;

	dprintf(1,"returning: %d\n", sent);
	return sent;
#if 0
	/* Assume every byte needs escaped */
	fptr = frame = malloc((data_len*2)+2);
	if (!frame) {
		log_write(LOG_SYSERR,"malloc(%d)\n",(data_len*2)+2);
		return -1;
	}

//	/* Start */
//	*fptr++ = PACKET_START;
	/* byte 1: opcode */
//	fptr =_frame_add(fptr,&opcode,1);
	/* byte 2: unit # */
//	fptr = _frame_add(fptr,&unit,1);
	/* byte 3+4: control code */
//	fptr = _frame_add(fptr,(uint8_t *)&control,2);
	/* byte 5+6: data length */
//	fptr = _frame_add(fptr,(uint8_t *)&data_len,2);
	*(uint16_t *)fptr = data_len;
	fptr += 2;
	/* byte 7+: data */
	if (data && data_len) fptr = _frame_add(fptr,data,data_len);
#ifdef CHKSUM
	/* byte n: checksum */
	fptr = _frame_add(fptr,(uint16_t *)&sum,2);
#endif
	/* byte n: end */
	*fptr++ = PACKET_END;

	if (debug >= 5) bindump("SENDING",frame,fptr - frame);
	bytes = send(fd, frame, fptr - frame, 0);
	dprintf(5,"bytes: %d\n", bytes);
	free(frame);
	return bytes;
#endif
}

#if 0
static int _read_frame(socket_t fd, uint8_t *data, int len) {
	uint8_t header[8], ch, *frame;
	register int i,j;
	int bytes;

	ch = PACKET_START;
	send(fd, &ch, 1, 0);
	for(i=j=0; i < len; i++) {
		if (data[i] == PACKET_START || data[i] == PACKET_END || data[i] == PACKET_ESC) {
			dprintf(1,"****** ESCAPING *****\n");
			frame[j++] = PACKET_ESC;
			switch(data[i]) {
			case PACKET_START:
				frame[j++] = PACKET_ESC_START;
				break;
			case PACKET_END:
				frame[j++] = PACKET_ESC_END; 
				break;
			case PACKET_ESC:
				frame[j++] = PACKET_ESC_ESC; 
				break;
			}
		} else {
			data[j++] = data[i];
		}
	}

	if (debug >= 5) bindump("SENDING",frame,j);
	bytes = send(fd, frame, j, 0);
	dprintf(5,"bytes: %d\n", bytes);
	return bytes;
}
#endif

int devserver_recv(socket_t fd, uint8_t *opcode, uint8_t *unit, uint16_t *control, void *data, int datasz, int timeout) {
	uint8_t *frame,*fptr;
	int bytes,len,bytes_left;
	uint8_t ch,pkt[256],*dptr;
	struct devserver_header h;
	register int i,j;

//	dprintf(5,"opcode: %d, unit: %d, control: %d, data: %p, datasz: %d, timeout: %d\n",
//		*opcode, *unit, *control, data, datasz, timeout);

	/* Read the header */
	bytes = recv(fd, &h, sizeof(h), 0);
	dprintf(5,"bytes: %d\n", bytes);
	if (bytes < 0) return -1;
	dprintf(1,"header: opcode: %02x, unit: %d, control: %04x, len: %d\n", h.opcode, h.unit, h.control, h.len);
	if (debug >= 5) bindump("RECEIVED",&h,sizeof(h));

	/* Read the data */
	len = h.len;
	if (len > datasz) len = datasz;
	dprintf(1,"len: %d\n",len);
	bytes_left = h.len - len;
	dprintf(1,"bytes_left: %d\n", bytes_left);
	if (data && len) {
		bytes = recv(fd, data, len, 0);
		dprintf(5,"bytes: %d\n", bytes);
		if (bytes < 0) return -1;
		if (debug >= 5) bindump("RECEIVED",data,bytes);
		/* XXX */
		if (bytes != len) return 0;
	}
	for(i=0; i < bytes_left; i++) recv(fd, &ch, 1, 0);
	dprintf(5,"returning: %d\n", bytes);
	*opcode = h.opcode;
	*unit = h.unit;
	*control = h.control;
	return bytes;
#if 0
	bytes = read(fd,&ch,1);
	dprintf(5,"bytes: %d\n", bytes);
	if (bytes < 0) perror("read");

	dprintf(5,"ch: %02x\n", ch);
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
//	*control = devserver_get16(&pkt[i]);
	*control = *(unsigned short *) &pkt[i];
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
		dptr = data;
		dprintf(1,"getting data...\n");
		for(j=0; j < len; j++) {
			bytes = recv(fd,&ch,1,0);
			dprintf(5,"bytes: %d\n", bytes);
			if (bytes < 1) return -1;
			dprintf(1,"ch: %02x\n", ch);
			if (ch == PACKET_ESC) {
				dprintf(1,"***** ESC ******\n");
				bytes = recv(fd,&ch,1,0);
				dprintf(5,"bytes: %d\n", bytes);
				if (bytes < 1) return -1;
				dprintf(1,"ch: %02x\n", ch);
				switch(ch) {
				case PACKET_ESC_START:
					dptr[j] = PACKET_START;
					break;
				case PACKET_ESC_END:
					dptr[j] = PACKET_END;
					break;
				case PACKET_ESC_ESC:
					dptr[j] = PACKET_ESC;
					break;
				}
			} else {
				dptr[j] = ch;
			}
		}
		bytes_left -= len;
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
	dprintf(5,"opcode: %d, unit: %d, control: %d, data: %p, datasz: %d, timeout: %d\n",
		*opcode, *unit, *control, data, datasz, timeout);
	/* Return total bytes written into buffer */
	dprintf(5,"returning: %d\n", 1);
#endif
	return 1;
}

int devserver_request(socket_t fd, uint8_t opcode, uint8_t unit, uint16_t control, void *data, int len) {
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
static int devserver_reply(socket_t fd, uint8_t status, uint8_t unit, uint16_t control, uint8_t *data, int len) {
	int bytes;

	bytes = devserver_send(fd,status,unit,control,data,len);
	dprintf(7,"bytes: %d\n", bytes);
	return (bytes < 0 ? 1 : 0);
}

/* Send error reply */
int devserver_error(socket_t fd, uint8_t code) {
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
	dprintf(1,"conf->count: %d\n", conf->count);
	for(i=0; i < conf->count; i++) {
		dprintf(1,"units[%d].name: %s\n", i, conf->units[i].name);
		if (strcmp(conf->units[i].name,(char *)data) == 0) {
			dprintf(1,"found!\n");
			io = &conf->units[i];
			unit = i;
			break;
		}
	}
	errno = ENOENT;
	if (!io) {
		dprintf(1,"NOT found!\n");
		return devserver_error(c,ENOENT);
	}
	dprintf(1,"io->open: %p\n", io->open);
	if (!io->open) return devserver_reply(c,DEVSERVER_SUCCESS,unit,control,0,0);
	dprintf(1,"io->handle: %p\n", io->handle);
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
	int bytes,len;

	dprintf(5,"unit: %d, control: %d, data: %p, len: %d\n", unit, control, data, datasz);
	if (unit < 0 || unit > conf->count) return devserver_error(c,EBADF);

	io = &conf->units[unit];
	/* control has bytes to write */
	if (strcmp(io->type,"can")==0)
		bytes = io->write(io->handle, data, sizeof(struct can_frame));
	else {
		len = control > datasz ? datasz : control;
		dprintf(1,"len: %d\n",len);
		bytes = io->write(io->handle, data, control > datasz ? datasz : control);
	}
	dprintf(5,"bytes: %d\n", bytes);
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

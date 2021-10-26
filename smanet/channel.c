
/*
Copyright (c) 2021, Stephen P. Shoecraft
All rights reserved.

This source code is licensed under the BSD-style license found in the
LICENSE file in the root directory of this source tree.
*/

#include "smanet_internal.h"
#include <sys/stat.h>
#ifdef __WIN32
#include <windows.h>
#endif

#define dlevel 1

struct chanver {
	unsigned char sig1;
	unsigned char sig2;
	unsigned char major;
	unsigned char minor;
};

#define CHANVER_SIG1 0xd5
#define CHANVER_SIG2 0xaa

int smanet_destroy_channels(smanet_session_t *s) {
	smanet_channel_t *c;

	list_reset(s->channels);
	while((c = list_get_next(s->channels)) != 0) {
		if (c->mask & CH_STATUS)
			list_destroy(c->strings);
	}
	list_destroy(s->channels);
	return 0;
}

static int parse_channels(smanet_session_t *s, uint8_t *data, int data_len) {
	smanet_channel_t newchan;
	register uint8_t *sptr, *eptr;
	int id;
//	uint8_t tmask;

	/* Now parse the structure into channels */
	dprintf(dlevel,"data_len: %d\n", data_len);
	sptr = data;
	eptr = data + data_len;
	id = 1;
	while(sptr < eptr) {
		memset(&newchan,0,sizeof(newchan));
		newchan.id = id++;
		newchan.index = *sptr++;
		newchan.mask = getshort(sptr);
		sptr += 2;
		newchan.format = getshort(sptr);
		sptr += 2;
		newchan.level = getshort(sptr);
		sptr += 2;
		memcpy(newchan.name,sptr,16);
		sptr += 16;
		switch(newchan.mask & 0x0f) {
		case CH_ANALOG:
//			newchan.class = SMANET_CHANCLASS_ANALOG;
			memcpy(newchan.unit,sptr,8);
			sptr += 8;
			newchan.gain = *(float *)sptr;
			sptr += 4;
			newchan.offset = *(float *)sptr;
			sptr += 4;
			dprintf(dlevel,"unit: %s, gain: %f, offset: %f\n", newchan.unit, newchan.gain, newchan.offset);
			break;
		case CH_DIGITAL:
//			newchan.class = SMANET_CHANCLASS_DIGITAL;
			memcpy(newchan.txtlo,sptr,16);
			sptr += 16;
			memcpy(newchan.txthi,sptr,16);
			sptr += 16;
			dprintf(dlevel,"txtlo: %s, txthi: %s\n", newchan.txtlo, newchan.txthi);
			break;
		case CH_COUNTER:
//			newchan.class = SMANET_CHANCLASS_COUNTER;
			memcpy(newchan.unit,sptr,8);
			sptr += 8;
			newchan.gain = *(float *)sptr;
			sptr += 4;
			dprintf(dlevel,"unit: %s, gain: %f\n", newchan.unit, newchan.gain);
			break;
		case CH_STATUS:
//			newchan.class = SMANET_CHANCLASS_STATUS;
			newchan.size = *(uint16_t *)sptr;
			dprintf(dlevel,"newchan.size: %d\n",newchan.size);
			sptr += 2;
			{
				register int i;
				for(i=0; i < newchan.size - 1; i++) {
					if (sptr[i] == 0)
						sptr[i] = ',';
				}
			}
			dprintf(1,"fixed status: %s\n", sptr);
			conv_type(DATA_TYPE_LIST,&newchan.strings,0,DATA_TYPE_STRING,sptr,newchan.size);
			dprintf(1,"count: %d\n", list_count(newchan.strings));
			sptr += newchan.size;
			break;
		default:
			dprintf(dlevel,"ERROR: unhandled type!\n");
			sptr = eptr;
			break;
		}
		/* Set the type */
		switch(newchan.format & 0xf) {
		case CH_BYTE:
			newchan.type = DATA_TYPE_BYTE;
			break;
		case CH_SHORT:
			newchan.type = DATA_TYPE_SHORT;
			break;
		case CH_LONG:
			newchan.type = DATA_TYPE_LONG;
			break;
		case CH_FLOAT:
			newchan.type = DATA_TYPE_FLOAT;
			break;
		case CH_DOUBLE:
			newchan.type = DATA_TYPE_DOUBLE;
			break;
		case CH_ARRAY:
			newchan.type = DATA_TYPE_LIST;
			break;
		default:
			newchan.type = 0;
			break;
		}
		dprintf(dlevel,"newchan: id: %d, name: %s, index: %02x, mask: %04x, format: %04x, level: %d, type: %d(%s)\n",
			newchan.id, newchan.name, newchan.index, newchan.mask, newchan.format, newchan.level, newchan.type, typestr(newchan.type));
		list_add(s->channels,&newchan,sizeof(newchan));
	}
	return 0;
}

int smanet_read_channels(smanet_session_t *s) {
	smanet_packet_t *p;
	FILE *fp;
#if 0
	char user[32];

#ifdef __WIN32
	DWORD bufsz = sizeof(user);
	GetUserName(user,&bufsz);
#else
	getlogin_r(user,sizeof(user));
#endif
#endif
	fp = fopen(s->chanpath,"rb");
	if (fp) {
		int fd;
		struct stat buf;

		fd = fileno(fp);
		dprintf(1,"fd: %d\n", fd);
		if (fstat(fd, &buf) == 0) {
			dprintf(1,"st_size: %d\n", buf.st_size);
			if (buf.st_size > 0) {
				p = smanet_alloc_packet(buf.st_size);
				if (!p) return 1;
				p->dataidx = fread(p->data,1,buf.st_size,fp);
				parse_channels(s,p->data,p->dataidx);
				smanet_free_packet(p);
				return 0;
			}
		}
		fclose(fp);
	}

	p = smanet_alloc_packet(256);
	if (!p) return 1;
	if (smanet_command(s,CMD_GET_CINFO,p,0,0)) return 1;
	dprintf(1,"p->dataidx: %d\n", p->dataidx);
	if (debug) bindump("channel data",p->data,p->dataidx);
	fp = fopen(s->chanpath,"wb+");
	if (fp) {
		fwrite(p->data,1,p->dataidx,fp);
		fclose(fp);
	}
	parse_channels(s,p->data,p->dataidx);
	return 0;
}

int smanet_save_channels(smanet_session_t *s) {
	FILE *fp;
	uint8_t temp[256];
	smanet_channel_t *c;
	struct chanver chanver;
	int r,bytes;
	uint16_t len;

	fp = fopen(s->chanpath,"wb+");
	if (!fp) {
		log_write(LOG_SYSERR,"smanet_save_channels: fopen %s",s->chanpath);
		return 1;
	}
	r = 1;
	chanver.sig1 = CHANVER_SIG1;
	chanver.sig2 = CHANVER_SIG2;
	chanver.major = CHANVER_MAJOR;
	chanver.minor = CHANVER_MINOR;
	bytes = fwrite(&chanver,1,sizeof(chanver),fp);
	list_reset(s->channels);
	while((c = list_get_next(s->channels)) != 0) {
		bytes = fwrite(c,1,sizeof(*c),fp);
		if (bytes < 0) {
			log_write(LOG_SYSERR,"smanet_save_channels: fwrite");
			goto smanet_save_channels_error;
		}
		if (c->mask & CH_STATUS) {
			dprintf(1,"count: %d\n", list_count(c->strings));
			conv_type(DATA_TYPE_STRING,&temp,sizeof(temp)-1,DATA_TYPE_LIST,&c->strings,0);
			dprintf(1,"temp: %s\n", temp);
			len = strlen((char *)temp);
			if (fwrite(&len,1,sizeof(len),fp) < 0) goto smanet_save_channels_error;
			if (fwrite(temp,1,len,fp) < 0) goto smanet_save_channels_error;
		}
//		dprintf(1,"c: name: %s, timestamp: %d\n", c->name, c->value.timestamp);
	}
	r = 0;
smanet_save_channels_error:
	fclose(fp);
	return r;
}

void smanet_set_chanpath(smanet_session_t *s, char *path) {
	strncpy(s->chanpath, path, sizeof(s->chanpath)-1);
}

int smanet_load_channels(smanet_session_t *s) {
	FILE *fp;
	char temp[256];
	smanet_channel_t newchan;
	struct chanver chanver;
	int bytes,r;
	uint16_t len;

	dprintf(1,"chanpath: %s\n", s->chanpath);
	fp = fopen(s->chanpath,"rb");
	if (!fp) {
		log_write(LOG_SYSERR,"smanet_load_channels: fopen %s",s->chanpath);
		return 1;
	}

	smanet_destroy_channels(s);
	s->channels = list_create();

	bytes = fread(&chanver,1,sizeof(chanver),fp);
	if (bytes != sizeof(chanver)) {
		log_write(LOG_SYSERR,"smanet_load_channels: read error");
		return 1;
	}
	if (chanver.sig1 != CHANVER_SIG1 || chanver.sig2 != CHANVER_SIG2) {
		log_write(LOG_ERROR,"smanet_load_channels: bad sig (%02x%02x != %02x%02x), ignoring", chanver.sig1, chanver.sig2,
			CHANVER_SIG1, CHANVER_SIG2);
		return 1;
	}
	if (chanver.major != CHANVER_MAJOR || chanver.minor != CHANVER_MINOR) {
		log_write(LOG_SYSERR,"smanet_load_channels: version mismatch(%d.%d != %d.%d)",chanver.major,chanver.minor,
			CHANVER_MAJOR,CHANVER_MINOR);
		return 1;
	}

	r = 1;
	dprintf(1,"fp: %p\n", fp);
	while(!feof(fp)) {
		bytes = fread(&newchan,1,sizeof(newchan),fp);
		if (bytes < 0) {
			log_write(LOG_SYSERR,"smanet_load_channels: fread");
			goto smanet_load_channels_error;
		} else if (bytes == 0) break;
		if (newchan.mask & CH_STATUS) {
//			dprintf(dlevel,"size: %d\n", newchan.size);
			if (fread(&len,1,sizeof(len),fp) < 0) goto smanet_load_channels_error;
//			dprintf(dlevel,"name: %s, len: %d\n", newchan.name, len);
			if (fread(temp,1,len,fp) < 0) goto smanet_load_channels_error;
			temp[len] = 0;
//			dprintf(dlevel,"temp: %s\n", temp);
			conv_type(DATA_TYPE_LIST,&newchan.strings,0,DATA_TYPE_STRING,temp,len);
//			dprintf(dlevel,"count: %d\n", list_count(newchan.strings));
		}
//		dprintf(1,"newchan: name: %s, timestamp: %d\n", newchan.name, newchan.value.timestamp);
		list_add(s->channels,&newchan,sizeof(newchan));
	}
	r = 0;
smanet_load_channels_error:
	fclose(fp);
	return r;
}

smanet_channel_t *smanet_get_channel(smanet_session_t *s, char *name) {
	smanet_channel_t *c;

	dprintf(1,"name: %s\n", name);

	list_reset(s->channels);
	while((c = list_get_next(s->channels)) != 0) {
		if (strcmp(c->name,name) == 0) {
			dprintf(1,"found!\n");
			return c;
		}
	}
	dprintf(1,"NOT found!\n");
	return 0;
}

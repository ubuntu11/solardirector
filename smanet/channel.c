
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

#define dlevel 6

#define CHANFILE_SIG1		0xd5
#define CHANFILE_SIG2 		0xaa
#define CHANFILE_VERSION 	2

int smanet_destroy_channels(smanet_session_t *s) {
	register int i;

	if (!s->chans) return 0;

	for(i=0; i < s->chancount; i++) {
		if (s->chans[i].mask & CH_STATUS)
			list_destroy(s->chans[i].strings);
	}
	free(s->chans);
	s->chans = 0;
	return 0;
}

static int parse_channels(smanet_session_t *s, uint8_t *data, int data_len) {
	smanet_channel_t newchan,*c;
	register uint8_t *sptr, *eptr;
	int type,format,i;
	list channels;

	/* Create a temp list to hold the channels */
	channels = list_create();

	/* Now parse the structure into channels */
	dprintf(dlevel,"data_len: %d\n", data_len);
	sptr = data;
	eptr = data + data_len;
	i = 0;
	while(sptr < eptr) {
		memset(&newchan,0,sizeof(newchan));
		newchan.id = i++;
		newchan.index = *sptr++;
		newchan.mask = getshort(sptr);
		sptr += 2;
		newchan.format = getshort(sptr);
		sptr += 2;
		newchan.level = getshort(sptr);
		sptr += 2;
		memcpy(newchan.name,sptr,16);
		sptr += 16;
		type = newchan.mask & 0x0f;
		dprintf(smanet_dlevel,"type: %x\n",type);
		switch(type) {
		case CH_ANALOG:
			memcpy(newchan.unit,sptr,8);
			sptr += 8;
//			newchan.gain = *(float *)sptr;
			newchan.gain = _getf32(sptr);
			sptr += 4;
//			newchan.offset = *(float *)sptr;
			newchan.gain = _getf32(sptr);
			sptr += 4;
			dprintf(dlevel,"unit: %s, gain: %f, offset: %f\n", newchan.unit, newchan.gain, newchan.offset);
			break;
		case CH_DIGITAL:
			memcpy(newchan.txtlo,sptr,16);
			sptr += 16;
			memcpy(newchan.txthi,sptr,16);
			sptr += 16;
			dprintf(dlevel,"txtlo: %s, txthi: %s\n", newchan.txtlo, newchan.txthi);
			break;
		case CH_COUNTER:
			memcpy(newchan.unit,sptr,8);
			sptr += 8;
//			newchan.gain = *(float *)sptr;
			newchan.gain = _getf32(sptr);
			sptr += 4;
			dprintf(dlevel,"unit: %s, gain: %f\n", newchan.unit, newchan.gain);
			break;
		case CH_STATUS:
//			newchan.size = *(uint16_t *)sptr;
			newchan.size = _getu16(sptr);
			dprintf(dlevel,"newchan.size: %d\n",newchan.size);
			sptr += 2;
			{
				register int i;
				for(i=0; i < newchan.size - 1; i++) {
					if (sptr[i] == 0)
						sptr[i] = ',';
				}
			}
			dprintf(smanet_dlevel,"fixed status: %s\n", sptr);
			conv_type(DATA_TYPE_STRING_LIST,&newchan.strings,0,DATA_TYPE_STRING,sptr,newchan.size);
			dprintf(smanet_dlevel,"count: %d\n", list_count(newchan.strings));
			sptr += newchan.size;
			break;
		default:
			log_error("parse_channels: unhandled type: %x\n", type);
			sptr = eptr;
			break;
		}
		/* Set the type */
		format = newchan.format & 0x0f;
		dprintf(smanet_dlevel,"format: %x\n",format);
		switch(format) {
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
		case CH_BYTE | CH_ARRAY:
			newchan.type = DATA_TYPE_BYTE_ARRAY;
			break;
		case CH_SHORT | CH_ARRAY:
			newchan.type = DATA_TYPE_SHORT_ARRAY;
			break;
		case CH_LONG | CH_ARRAY:
			newchan.type = DATA_TYPE_LONG_ARRAY;
			break;
		case CH_FLOAT | CH_ARRAY:
			newchan.type = DATA_TYPE_FLOAT_ARRAY;
			break;
		case CH_DOUBLE | CH_ARRAY:
			newchan.type = DATA_TYPE_DOUBLE_ARRAY;
			break;
		default:
			newchan.type = DATA_TYPE_UNKNOWN;
			log_error("parse_channels: unhandled format: %x\n", format);
			break;
		}
		if (format & CH_ARRAY) {
			newchan.count = (newchan.format & 0xf0) >> 8;
			printf("===> channel %s count: %d\n", newchan.name, newchan.count);
		}
		dprintf(dlevel,"newchan: id: %d, name: %s, index: %02x, mask: %04x, format: %04x, level: %d, type: %d(%s), count: %d\n",
			newchan.id, newchan.name, newchan.index, newchan.mask, newchan.format, newchan.level, newchan.type, typestr(newchan.type), newchan.count);
		list_add(channels,&newchan,sizeof(newchan));
	}

	/* Now that we know how many channels we have, alloc the mem and copy the list */
	dprintf(dlevel,"count: %d\n", list_count(channels));
	s->chans = malloc(sizeof(smanet_channel_t)*list_count(channels));
	if (!s->chans) {
		log_syserror("smanet_parse_channels: malloc");
		return 1;
	}
	i = 0;
	list_reset(channels);
	while((c = list_get_next(channels)) != 0) s->chans[i++] = *c;
	s->chancount = i;
	list_destroy(channels);
	return 0;
}

int smanet_read_channels(smanet_session_t *s) {
	smanet_packet_t *p;

	p = smanet_alloc_packet(8192);
	if (!p) return 1;
	if (smanet_command(s,CMD_GET_CINFO,p,0,0)) return 1;
	dprintf(smanet_dlevel,"p->dataidx: %d\n", p->dataidx);
//	if (debug) bindump("channel data",p->data,p->dataidx);

	parse_channels(s,p->data,p->dataidx);
	return 0;
}

int smanet_save_channels(smanet_session_t *s, char *filename) {
	FILE *fp;
	uint8_t temp[256];
	smanet_channel_t *c;
//	struct chanver chanver;
	int r,bytes;
	uint16_t len;
	int i;

	fp = fopen(filename,"wb+");
	if (!fp) {
		log_write(LOG_SYSERR,"smanet_save_channels: fopen %s",filename);
		return 1;
	}
	r = 1;
	/* our channels file is a 5-byte header: SIG1:1 SIG2:1 VERSION:1 COUNT:2 */
	temp[0] = CHANFILE_SIG1;
	temp[1] = CHANFILE_SIG2;
	temp[2] = CHANFILE_VERSION;
	_putu16(&temp[3],s->chancount);
	bytes = fwrite(&temp,1,5,fp);
	if (bytes < 1) {
		log_syserror("smanet_save_channels: error writing header");
		goto smanet_save_channels_error;
	}
	dprintf(dlevel,"count: %d\n", s->chancount);
	for(i=0; i < s->chancount; i++) {
		c = &s->chans[i];
		dprintf(dlevel,"chan: id: %d, name: %s, index: %02x, mask: %04x, format: %04x, level: %d, type: %d(%s), count: %d\n",
			c->id, c->name, c->index, c->mask, c->format, c->level, c->type, typestr(c->type), c->count);
		bytes = fwrite(c,1,sizeof(smanet_channel_t),fp);
		dprintf(dlevel,"bytes written: %d\n", bytes);
		if (bytes < 0) {
			log_write(LOG_SYSERR,"smanet_save_channels: fwrite");
			goto smanet_save_channels_error;
		}
		/* channel text options are a comma-seperated list preceded by a 2 byte length */
		if (c->mask & CH_STATUS) {
			dprintf(smanet_dlevel,"count: %d\n", list_count(c->strings));
			conv_type(DATA_TYPE_STRING,temp,sizeof(temp)-1,DATA_TYPE_STRING_LIST,&c->strings,0);
			dprintf(smanet_dlevel,"temp: %s\n", temp);
			len = strlen((char *)temp);
			if (fwrite(&len,1,2,fp) < 0) {
				log_syserror("smanet_save_channels: fwrite len");
				goto smanet_save_channels_error;
			}
			if (fwrite(temp,1,len,fp) < 0) {
				log_syserror("smanet_save_channels: fwrite temp");
				goto smanet_save_channels_error;
			}
		}
//		dprintf(smanet_dlevel,"c: name: %s, timestamp: %d\n", c->name, c->value.timestamp);
	}
	r = 0;
smanet_save_channels_error:
	fclose(fp);
	return r;
}

int smanet_load_channels(smanet_session_t *s, char *filename) {
	FILE *fp;
	char temp[256];
	uint8_t header[5];
	smanet_channel_t newchan;
//	struct chanver chanver;
	int bytes,i;
	uint16_t len;

	dprintf(smanet_dlevel,"filename: %s\n", filename);
	fp = fopen(filename,"rb");
	if (!fp) {
		log_write(LOG_SYSERR,"smanet_load_channels: fopen %s",filename);
		return 1;
	}

	smanet_destroy_channels(s);

	/* our channels file is a 5-byte header: SIG1:1 SIG2:1 VERSION:1 COUNT:2 */
	bytes = fread(header,1,sizeof(header),fp);
	if (bytes != sizeof(header)) {
		log_error("smanet_load_channels: error reading header");
		return 1;
	}
	if (header[0] != CHANFILE_SIG1 || header[1] != CHANFILE_SIG2) {
		log_write(LOG_ERROR,"smanet_load_channels: bad sig (%02x%02x != %02x%02x), ignoring",
			header[0], header[1], CHANFILE_SIG1, CHANFILE_SIG2);
		return 1;
	}
	if (header[2] != CHANFILE_VERSION) {
		log_error("smanet_load_channels: version mismatch(%d != %d)",header[2],CHANFILE_VERSION);
		return 1;
	}
	s->chancount = _getu16(&header[3]);
	dprintf(dlevel,"count: %d\n", s->chancount);

	s->chans = malloc(sizeof(smanet_channel_t)*s->chancount);
	if (!s->chans) {
		log_syserror("smanet_load_channels: malloc");
		return 1;
	}
	for(i=0; i < s->chancount; i++) {
		bytes = fread(&newchan,1,sizeof(newchan),fp);
		if (bytes != sizeof(newchan)) {
			log_write(LOG_SYSERR,"smanet_load_channels: short read");
			goto smanet_load_channels_error;
		}
		dprintf(dlevel,"id: %d, i: %d\n", newchan.id, i);
		if (newchan.id != i) {
			log_error("smanet_load_channels: id does not match index (%d:%d) for channel %s",
				newchan.id, i,  newchan.name);
			goto smanet_load_channels_error;
		}
		/* has text field? */
		if (newchan.mask & CH_STATUS) {
			if (fread(&len,1,sizeof(len),fp) < 0) {
				log_syserror("smanet_load_channels: error reading length for channel %s", newchan.name);
				goto smanet_load_channels_error;
			}
			dprintf(dlevel,"name: %s, len: %d\n", newchan.name, len);
			if (fread(temp,1,len,fp) < 0) {
				log_syserror("smanet_load_channels: error reading text for channel %s", newchan.name);
				goto smanet_load_channels_error;
			}
			temp[len] = 0;
			dprintf(dlevel,"temp: %s\n", temp);
			newchan.strings = list_create();
			conv_type(DATA_TYPE_STRING_LIST,newchan.strings,0,DATA_TYPE_STRING,temp,len);
			dprintf(dlevel,"count: %d\n", list_count(newchan.strings));
		}
		s->chans[i] = newchan;
	}
//	dprintf(dlevel,"i: %d, chancount: %d\n", i, s->chancount);
	fclose(fp);
	return 0;

smanet_load_channels_error:
	fclose(fp);
	free(s->chans);
	s->chans = 0;
	s->chancount = 0;
	return 1;
}

smanet_channel_t *smanet_get_channel(smanet_session_t *s, char *name) {
	register int i;

	dprintf(smanet_dlevel,"name: %s\n", name);

	if (s->chans) {
	for(i=0; i < s->chancount; i++) {
		if (strcmp(s->chans[i].name,name) == 0) {
			dprintf(smanet_dlevel,"found!\n");
			return &s->chans[i];
		}
	}
	}
	dprintf(smanet_dlevel,"NOT found!\n");
	return 0;
}

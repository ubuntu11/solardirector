
/*
Copyright (c) 2021, Stephen P. Shoecraft
All rights reserved.

This source code is licensed under the BSD-style license found in the
LICENSE file in the root directory of this source tree.
*/

#include "smanet_internal.h"
#include <sys/stat.h>

#if 0
#define  CH_ANALOG      0x0001
#define  CH_DIGITAL     0x0002
#define  CH_COUNTER     0x0004
#define  CH_STATUS      0x0008
#define  CH_ALL      0x000f

#define  CH_IN                  0x0100
#define  CH_OUT         0x0200

#define  CH_ANA_IN   (CH_ANALOG  | CH_IN)
#define  CH_DIG_IN   (CH_DIGITAL | CH_IN)
#define  CH_CNT_IN   (CH_COUNTER | CH_IN)
#define  CH_STA_IN   (CH_STATUS  | CH_IN)

#define  CH_PARA_ALL    (CH_PARA  | CH_ALL)
#define  CH_SPOT_ALL    (CH_SPOT  | CH_ALL)
#define  CH_MEAN_ALL    (CH_MEAN  | CH_ALL)
#define  CH_TEST_ALL    (CH_SPOT  | CH_IN  |  CH_ALL  | CH_TEST)
#endif

#define dlevel 1

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
		dprintf(dlevel,"newchan: id: %d, name: %s, index: %02x, mask: %04x, format: %04x, level: %d\n",
			newchan.id, newchan.name, newchan.index, newchan.mask, newchan.format, newchan.level);
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
#if 0
		tmask = (newchan.mask >> 10) & 0x0F;
		dprintf(1,"tmask: %02X\n", tmask);
		switch(tmask) {
		case 0x01:
			newchan.type = SMANET_CHANTYPE_PARAMETER;
			break;
		case 0x02:
			newchan.type = SMANET_CHANTYPE_SPOTVALUE;
			break;
		case 0x04:
			newchan.type = SMANET_CHANTYPE_ARCHIVE;
			break;
		case 0x08:
			newchan.type = SMANET_CHANTYPE_TEST;
			break;
		}
		dprintf(1,"newchan.class: %d, newchan.type: %d\n", newchan.class, newchan.type);
#endif
		list_add(s->channels,&newchan,sizeof(newchan));
	}
	return 0;
}

int smanet_get_channels(smanet_session_t *s) {
	smanet_packet_t *p;
	char user[32],path[256];
	FILE *fp;

	getlogin_r(user,sizeof(user));
	sprintf(path,"/tmp/%s_%s_clist.dat",user,s->type);
	dprintf(1,"path: %s\n", path);
	fp = fopen(path,"rb");
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
	bindump("channel data",p->data,p->dataidx);
	fp = fopen(path,"wb+");
	if (fp) {
		fwrite(p->data,1,p->dataidx,fp);
		fclose(fp);
	}
	parse_channels(s,p->data,p->dataidx);
	return 0;
}

int smanet_save_channels(smanet_session_t *s, char *path) {
	FILE *fp;
	uint8_t temp[256];
	smanet_channel_t *c;
	int r,bytes;
	uint16_t len;

	dprintf(1,"path: %s\n", path);

	fp = fopen(path,"wb+");
	if (!fp) {
		log_write(LOG_SYSERR,"smanet_save_channels: fopen %s",path);
		return 1;
	}
	r = 1;
	list_reset(s->channels);
	while((c = list_get_next(s->channels)) != 0) {
		bytes = fwrite(c,1,sizeof(*c),fp);
		if (bytes < 0) {
			log_write(LOG_SYSERR,"smanet_save_channels: fwrite");
			goto smanet_save_channels_error;
		}
		if (c->mask & CH_STATUS) {
//			dprintf(1,"count: %d\n", list_count(c->strings));
			conv_type(DATA_TYPE_STRING,&temp,sizeof(temp)-1,DATA_TYPE_LIST,&c->strings,0);
//			dprintf(1,"temp: %s\n", temp);
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

int smanet_load_channels(smanet_session_t *s, char *path) {
	FILE *fp;
	char temp[256];
	smanet_channel_t newchan;
	int bytes,r;
	uint16_t len;

	dprintf(1,"path: %s\n", path);

	fp = fopen(path,"rb");
	if (!fp) {
		log_write(LOG_SYSERR,"smanet_save_channels: fopen %s",path);
		return 1;
	}

	smanet_destroy_channels(s);
	s->channels = list_create();

	r = 1;
	while(!feof(fp)) {
		bytes = fread(&newchan,1,sizeof(newchan),fp);
		if (bytes < 0) {
			log_write(LOG_SYSERR,"smanet_save_channels: fread");
			goto smanet_load_channels_error;
		} else if (bytes == 0) break;
		if (newchan.mask & CH_STATUS) {
//			dprintf(dlevel,"size: %d\n", newchan.size);
	//		fread(&len,1,sizeof(len),fp);
	//		fread(temp,1,len,fp);
			if (fread(&len,1,sizeof(len),fp) < 0) goto smanet_load_channels_error;
			if (fread(temp,1,len,fp) < 0) goto smanet_load_channels_error;
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

smanet_channel_t *smanet_get_channelbyid(smanet_session_t *s, int id) {
	smanet_channel_t *c;

	list_reset(s->channels);
	while((c = list_get_next(s->channels)) != 0) {
		if (c->id == id)
			return c;
	}
	return 0;
}

smanet_channel_t *smanet_get_channelbyname(smanet_session_t *s, char *name) {
	smanet_channel_t *c;

	list_reset(s->channels);
	while((c = list_get_next(s->channels)) != 0) {
		if (strcmp(c->name,name) == 0)
			return c;
	}
	return 0;
}

smanet_channel_t *smanet_get_channelbyindex(smanet_session_t *s, uint16_t mask, uint8_t index) {
	smanet_channel_t *c;

	dprintf(1,"mask: %04x, index: %02x\n", mask, index);

	list_reset(s->channels);
	while((c = list_get_next(s->channels)) != 0) {
		if (c->mask == mask && c->index == index)
			return c;
	}
	return 0;
}

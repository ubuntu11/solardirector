
/*
Copyright (c) 2021, Stephen P. Shoecraft
All rights reserved.

This source code is licensed under the BSD-style license found in the
LICENSE file in the root directory of this source tree.
*/

#include "jk.h"

#define _getshort(p) (short)(*(p) | (*((p)+1) << 8))
#define _getint(p) (int)(*(p) | (*((p)+1) << 8) | (*((p)+2) << 16) | (*((p)+3) << 24))

static int jk_can_read(jk_session_t *s, solard_battery_t *bp) {
	return 1;
}

static void _getvolts(solard_battery_t *pp, uint8_t *data) {
	int i,j;

//	bindump("getvolts",data,300);
	i = 6;
	for(j=0; j < 24; j++) {
		pp->cellvolt[j] = _getshort(&data[i]) / 1000.0;
		if (!pp->cellvolt[j]) break;
		dprintf(4,"cellvolt[%02d] = data[%02d] = %.3f\n", j, i, (_getshort(&data[i]) / 1000.0));
		i += 2;
	}
	pp->ncells = j;
	dprintf(4,"cells: %d\n", pp->ncells);
	pp->voltage = ((unsigned short)_getshort(&data[118])) / 1000.0;
	dprintf(1,"voltage: %.2f\n", pp->voltage);
//	dprintf(1,"data[126]: %d %d %04x\n", _getshort(&data[126]),(unsigned short)_getshort(&data[126]),(unsigned short)_getshort(&data[126]));
	pp->current = _getshort(&data[126]) / 1000.0;
	dprintf(4,"current: %.2f\n", pp->current);
	pp->ntemps = 2;
	pp->temps[0] = ((unsigned short)_getshort(&data[130])) / 10.0;
	pp->temps[1] = ((unsigned short)_getshort(&data[132])) / 10.0;
	/* Dont include mosfet temp ... ? */
//	pp->temps[2] = ((unsigned short)_getshort(&data[134])) / 10.0;
}

#define GOT_RES 0x01
#define GOT_VOLT 0x02
#define GOT_INFO 0x04

static int getdata(solard_battery_t *pp, uint8_t *data, int bytes) {
	uint8_t sig[] = { 0x55,0xAA,0xEB,0x90 };
	int i,j,start,r;

	r = 0;
//	bindump("data",data,bytes);
	for(i=j=0; i < bytes; i++) {
		dprintf(6,"data[%d]: %02x, sig[%d]: %02x\n", i, data[i], j, sig[j]);
		if (data[i] == sig[j]) {
			if (j == 0) start = i;
			j++;
			if (j >= sizeof(sig) && (start + 300) < bytes) {
				dprintf(1,"found sig\n");
				if (data[i+1] == 1)  {
//					_getres(pp,&data[start]);
					r |= GOT_RES;
				} else if (data[i+1] == 2) {
					_getvolts(pp,&data[start]);
					r |= GOT_VOLT;
				} else if (data[i+1] == 3) {
//					_getinfo(pp,&data[start]);
					r |= GOT_INFO;
				}
				i += 300;
				j = 0;
			}
		}
		if (r & GOT_VOLT) break;
	}
	dprintf(4,"returning: %d\n", r);
	return r;
}

static int jk_std_read(jk_session_t *s, solard_battery_t *bp) {
	unsigned char getInfo[] =     { 0xaa,0x55,0x90,0xeb,0x97,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x11 };
	unsigned char getCellInfo[] = { 0xaa,0x55,0x90,0xeb,0x96,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x10 };
	uint8_t data[2048];
	int bytes,r,retries;

	/* Have to getInfo before can getCellInfo ... */
	retries=5;
	while(retries--) {
		bytes = s->tp->write(s->tp_handle,getInfo,sizeof(getInfo));
		dprintf(4,"bytes: %d\n", bytes);
		if (bytes < 0) return -1;
		bytes = s->tp->read(s->tp_handle,data,sizeof(data));
		dprintf(4,"bytes: %d\n", bytes);
		if (bytes < 0) return -1;
		r = getdata(bp,data,bytes);
		if (r & GOT_INFO) break;
		sleep(1);
	}
	retries=5;
	bytes = s->tp->write(s->tp_handle,getCellInfo,sizeof(getCellInfo));
	dprintf(4,"bytes: %d\n", bytes);
	if (bytes < 0) return -1;
	while(retries--) {
		bytes = s->tp->read(s->tp_handle,data,sizeof(data));
		dprintf(4,"bytes: %d\n", bytes);
		r = getdata(bp,data,bytes);
		if (r & GOT_VOLT) break;
	}
	return (retries < 1 ? -1 : 0);
}

int jk_read(void *handle, void *buf, int buflen) {
	jk_session_t *s = handle;
	solard_battery_t *bp = buf;;
	int r;

//	pp = buf;

	dprintf(2,"transport: %s\n", s->tp->name);
	r = 1;
	if (strncmp(s->tp->name,"can",3)==0) 
		r = jk_can_read(s,bp);
	else
		r = jk_std_read(s,bp);
	return r;
}

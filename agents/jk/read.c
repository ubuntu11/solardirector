
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

void dshort(int i, uint8_t *p) {
	uint16_t dw = _getshort(p);
	int ival = _getint(p);
	float wf10 = dw / 10.0;
	float wf1000 = dw / 1000.0;
	float if10 = ival / 10.0;
	float if1000 = ival / 1000.0;
	dprintf(1,"%02x: %d %04x d10: %f, d1000: %f\n", i, dw, dw, wf10, wf1000);
	dprintf(1,"%02x: %d %08x d10: %f, d1000: %f\n", i, ival, ival, if10, if1000);
}

static void _getvolts(solard_battery_t *bp, uint8_t *data) {
	int i,j;
	float f;

	if (debug) bindump("getvolts",data,300);

	i = 6;
	/* 6: Cell voltages */
	for(j=0; j < 24; j++) {
		bp->cellvolt[j] = _getshort(&data[i]) / 1000.0;
		if (!bp->cellvolt[j]) break;
		dprintf(1,"cellvolt[%02d] = data[%02d] = %.3f\n", j, i, (_getshort(&data[i]) / 1000.0));
		i += 2;
	}
	bp->ncells = j;
	dprintf(4,"cells: %d\n", bp->ncells);
	while(i < 58) {
		dshort(i,&data[i]);
		i += 2;
	}
	dprintf(1,"i: %02x\n", i);
	/* 0x3a: Average cell voltage */
//	bp->avg_cell = data[34];
	dprintf(1,"cell_avg: %f\n", _getshort(&data[58]) / 1000.0);
	i += 2;
	/* 0x3c: Delta cell voltage */
	dprintf(1,"cell_diff: %f\n", _getshort(&data[60]) / 1000.0);
	i += 2;
	/* 0x3e: Current balancer?? */
	dprintf(1,"current: %d\n", _getshort(&data[62]));
	i += 2;
//#if BATTERY_CELLRES
	dprintf(1,"i: %02x\n", i);
	/* 0x40: Cell resistances */
	for(j=0; j < 24; j++) {
//		bp->cellvolt[j] = _getshort(&data[i]) / 1000.0;
//		if (!bp->cellvolt[j]) break;
		dprintf(1,"cellres[%02d] = data[%02d] = %.3f\n", j, i, (_getshort(&data[i]) / 1000.0));
		i += 2;
	}
//#endif
	dprintf(1,"i: %02x\n", i);
	while(i < 0x76) {
		dshort(i,&data[i]);
		i += 2;
	}
	/* 0x76: Battery voltage */
	dprintf(1,"i: %02x\n", i);
	bp->voltage = ((unsigned short)_getshort(&data[i])) / 1000.0;
	dprintf(1,"voltage: %.2f\n", bp->voltage);
	i += 2;
	while(i < 0x7e) {
		dshort(i,&data[i]);
		i += 2;
	}
	/* 0x7e: Battery current */
	dprintf(1,"i: %02x\n", i);
	bp->current = _getshort(&data[i]) / 1000.0;
	dprintf(1,"current: %.2f\n", bp->current);
	i += 2;
	while(i < 0x82) {
		dshort(i,&data[i]);
		i += 2;
	}
	/* 0x82: Temps */
	dprintf(1,"i: %02x\n", i);
	bp->ntemps = 2;
	bp->temps[0] = ((unsigned short)_getshort(&data[i])) / 10.0;
	dprintf(1,"temp 1: %f\n", bp->temps[0]);
	i += 2;
	bp->temps[1] = ((unsigned short)_getshort(&data[i])) / 10.0;
	dprintf(1,"temp 2: %f\n", bp->temps[1]);
	i += 2;
	bp->temps[2] = ((unsigned short)_getshort(&data[i])) / 10.0;
	dprintf(1,"MOS Temp: %f\n", bp->temps[2]);
	i += 2;
	dprintf(1,"i: %02x\n", i);
	while(i < 0x8e) {
		dshort(i,&data[i]);
		i += 2;
	}
#if 0
read.c(26) dshort: 8e: 59867 e9db d10: 5986.700195, d1000: 59.867001
read.c(27) dshort: 8e: 59867 0000e9db d10: 5986.700195, d1000: 59.867001
read.c(26) dshort: 90: 0 0000 d10: 0.000000, d1000: 0.000000
read.c(27) dshort: 90: 947912704 38800000 d10: 94791272.000000, d1000: 947912.687500
read.c(26) dshort: 92: 14464 3880 d10: 1446.400024, d1000: 14.464000
read.c(27) dshort: 92: 80000 00013880 d10: 8000.000000, d1000: 80.000000
	/* 0x8d: Percent remain */
	dprintf(1,"i: %02x\n", i);
	f = _getint(&data[i]);
	dprintf(1,"% remain: %f\n", f);
	i += 4;
#endif
	/* 0x8e: Capacity remain */
	dprintf(1,"i: %02x\n", i);
	f = _getint(&data[i]);
	dprintf(1,"capacity remain: %f\n", f);
	i += 4;
	/* 0x92: Nominal capacity */
	dprintf(1,"i: %02x\n", i);
	bp->capacity = _getint(&data[i]) / 1000.0;
	dprintf(1,"capacity: %.2f\n", bp->capacity);
	i += 4;
	dprintf(1,"i: %02x\n", i);
	while(i < 0xd4) {
		dshort(i,&data[i]);
		i += 2;
	}
}

static void _getinfo(jk_info_t *info, uint8_t *data) {
	int i,j,uptime;

//	bindump("getInfo",data,300);

	j=0;
	/* Model */
	for(i=6; i < 300 && data[i]; i++) {
		info->model[j++] = data[i];
		if (j >= sizeof(info->model)-1) break;
	}
	info->model[j] = 0;
	dprintf(1,"Model: %s\n", info->model);
	/* Skip 0s */
	while(i < 300 && !data[i]) i++;
	/* HWVers */
	dprintf(1,"i: %x\n", i);
	j=0;
	while(i < 300 && data[i]) {
		info->hwvers[j++] = data[i++];
		if (j >= sizeof(info->hwvers)-1) break;
	}
	info->hwvers[j] = 0;
	dprintf(1,"HWVers: %s\n", info->hwvers);
	/* Skip 0s */
	while(i < 300 && !data[i]) i++;
	/* SWVers */
	j=0;
	dprintf(1,"i: %x\n", i);
	while(i < 300 && data[i]) {
		info->swvers[j++] = data[i++];
		if (j >= sizeof(info->swvers)-1) break;
	}
	info->swvers[j] = 0;
	dprintf(1,"SWVers: %s\n", info->swvers);
	/* Skip 0s */
	while(i < 300 && !data[i]) i++;
	dprintf(1,"i: %x\n", i);
	uptime = _getint(&data[i]);
	dprintf(1,"uptime: %04x\n", uptime);
	i += 4;
//	unk = _getshort(&data[i]);
//	i += 2;
	/* Skip 0s */
	while(i < 300 && !data[i]) i++;
	/* Device */
	j=0;
	dprintf(1,"i: %x\n", i);
	while(i < 300 && data[i]) {
		info->device[j++] = data[i++];
		if (j >= sizeof(info->device)-1) break;
	}
	info->device[j] = 0;
	dprintf(1,"Device: %s\n", info->device);
}

#define GOT_RES 0x01
#define GOT_VOLT 0x02
#define GOT_INFO 0x04

static int getdata(jk_info_t *info, solard_battery_t *bp, uint8_t *data, int bytes) {
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
				dprintf(1,"found sig, type: %d\n", data[i+1]);
				if (data[i+1] == 1)  {
#if BATTERY_CELLRES
					_getres(bp,&data[start]);
#endif
					r |= GOT_RES;
				} else if (data[i+1] == 2) {
					if (bp) _getvolts(bp,&data[start]);
					r |= GOT_VOLT;
				} else if (data[i+1] == 3) {
					if (!bp) _getinfo(info,&data[start]);
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

int jk_bt_read(jk_session_t *s, solard_battery_t *bp) {
	unsigned char getInfo[] =     { 0xaa,0x55,0x90,0xeb,0x97,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x11 };
	unsigned char getCellInfo[] = { 0xaa,0x55,0x90,0xeb,0x96,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x10 };
	uint8_t data[2048];
	int bytes,r,retries;

	dprintf(1,"bp: %p\n", bp);

	/* Have to getInfo before can getCellInfo ... */
	dprintf(1,"getInfo...\n");
	retries=5;
	while(retries--) {
		bytes = s->tp->write(s->tp_handle,getInfo,sizeof(getInfo));
		dprintf(4,"bytes: %d\n", bytes);
		if (bytes < 0) return -1;
		bytes = s->tp->read(s->tp_handle,data,sizeof(data));
		dprintf(4,"bytes: %d\n", bytes);
		if (bytes < 0) return -1;
		r = getdata(&s->info,bp,data,bytes);
		if (r & GOT_INFO) break;
		sleep(1);
	}
	dprintf(1,"retries: %d\n", retries);
	if (!bp) return (retries < 1 ? -1 : 0);

	dprintf(1,"getCellInfo...\n");
	retries=5;
	bytes = s->tp->write(s->tp_handle,getCellInfo,sizeof(getCellInfo));
	dprintf(4,"bytes: %d\n", bytes);
	if (bytes < 0) return -1;
	while(retries--) {
		bytes = s->tp->read(s->tp_handle,data,sizeof(data));
		dprintf(4,"bytes: %d\n", bytes);
		r = getdata(&s->info,bp,data,bytes);
		if (r & GOT_VOLT) break;
	}
	return (retries < 1 ? -1 : 0);
}

static int jk_std_read(jk_session_t *s, solard_battery_t *bp) {
	uint8_t data[128];
	int i,j,bytes;
//	struct jk_protect prot;

	dprintf(3,"getting HWINFO...\n");
	if ((bytes = jk_rw(s, JK_CMD_READ, JK_CMD_HWINFO, data, sizeof(data))) < 0) {
		dprintf(1,"returning 1!\n");
		return 1;
	}

	bp->voltage = (float)jk_getshort(&data[0]) / 100.0;
	bp->current = (float)jk_getshort(&data[2]) / 100.0;
	bp->capacity = (float)jk_getshort(&data[6]) / 100.0;
        dprintf(2,"voltage: %.2f\n", bp->voltage);
        dprintf(2,"current: %.2f\n", bp->current);
        dprintf(2,"capacity: %.2f\n", bp->capacity);

	/* Balance */
	bp->balancebits = jk_getshort(&data[12]);
	bp->balancebits |= jk_getshort(&data[14]) << 16;
#ifdef DEBUG
	{
		char bits[40];
		unsigned short mask = 1;
		i = 0;
		while(mask) {
			bits[i++] = ((bp->balancebits & mask) != 0 ? '1' : '0');
			mask <<= 1;
		}
		bits[i] = 0;
		dprintf(5,"balancebits: %s\n",bits);
	}
#endif

	/* Protectbits */
//	jk_get_protect(&prot,jk_getshort(&data[16]));

	s->fetstate = data[20];
	dprintf(2,"s->fetstate: %02x\n", s->fetstate);
	if (s->fetstate & JK_MOS_CHARGE) solard_set_state(bp,BATTERY_STATE_CHARGING);
	else solard_clear_state(bp,BATTERY_STATE_CHARGING);
	if (s->fetstate & JK_MOS_DISCHARGE) solard_set_state(bp,BATTERY_STATE_DISCHARGING);
	else solard_clear_state(bp,BATTERY_STATE_DISCHARGING);

	bp->ncells = data[21];
	dprintf(2,"cells: %d\n", bp->ncells);
	bp->ntemps = data[22];

	/* Temps */
#define CTEMP(v) ( (v - 2731) / 10 )
	/* 1st temp is MOS - we dont care about it */
	j=0;
	for(i=1; i < bp->ntemps; i++) {
		bp->temps[j++] = CTEMP((float)jk_getshort(&data[23+(i*2)]));
	}
	bp->ntemps--;

	/* Cell volts */
	if ((bytes = jk_rw(s, JK_CMD_READ, JK_CMD_CELLINFO, data, sizeof(data))) < 0) return 1;

	for(i=0; i < bp->ncells; i++) {
		bp->cellvolt[i] = (float)jk_getshort(&data[i*2]) / 1000;
	}
	return 0;
}

int jk_read(void *handle, void *buf, int buflen) {
	jk_session_t *s = handle;
	solard_battery_t *bp = buf;;
	int r;

	dprintf(2,"transport: %s\n", s->tp->name);
	r = 1;
	if (strcmp(s->tp->name,"can")==0) 
		r = jk_can_read(s,bp);
	else if (strcmp(s->tp->name,"bt")==0) 
		r = jk_bt_read(s,bp);
	else
		r = jk_std_read(s,bp);
	dprintf(1,"returning: %d\n", r);
	return r;
}

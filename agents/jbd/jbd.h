
/*
Copyright (c) 2021, Stephen P. Shoecraft
All rights reserved.

This source code is licensed under the BSD-style license found in the
LICENSE file in the root directory of this source tree.
*/

#ifndef __JBD_H
#define __JBD_H

#include <pthread.h>
#include "agent.h"
#include "jbd_regs.h"

#define JBD_NAME_LEN 32
#define JBD_MAX_TEMPS 8
#define JBD_MAX_CELLS 32


struct jbd_data {
	float capacity;			/* Battery pack capacity, in AH */
	float voltage;			/* Pack voltage */
	float current;			/* Pack current */
	int ntemps;			/* Number of temps */
	float temps[JBD_MAX_TEMPS];	/* Temp values */
	int ncells;			/* Number of cells, updated by BMS */
	float cellvolt[JBD_MAX_CELLS]; /* Per-cell voltages, updated by BMS */
	float cell_min;
	float cell_max;
	float cell_diff;
	float cell_avg;
	float cell_total;
	uint32_t balancebits;		/* Balance bitmask */
};
typedef struct jbd_data jbd_data_t;

struct jbd_session {
	solard_agent_t *ap;		/* Agent config pointer */
	char transport[SOLARD_TRANSPORT_LEN];
	char target[SOLARD_TARGET_LEN];
	char topts[SOLARD_TOPTS_LEN];
	solard_driver_t *tp;		/* Our transport */
	void *tp_handle;		/* Our transport handle */
	int (*can_get)(struct jbd_session *s, int id, uint8_t *data, int datasz);
	int (*reader)(struct jbd_session *);
	uint16_t state;			/* Pack state */
	jbd_data_t data;
	uint8_t fetstate;		/* Mosfet state */
	uint8_t balancing;		/* 0=off, 1=on, 2=only when charging */
	int errcode;			/* error indicator */
	char errmsg[256];		/* Error message if errcode !0 */
	pthread_mutex_t lock;
};
typedef struct jbd_session jbd_session_t;

/* States */
#define JBD_STATE_OPEN		0x01
#define JBD_STATE_RUNNING	0x02
#define JBD_STATE_CHARGING	0x10
#define JBD_STATE_DISCHARGING	0x20
#define JBD_STATE_BALANCING	0x40

struct jbd_protect {
	unsigned sover: 1;		/* Single overvoltage protection */
	unsigned sunder: 1;		/* Single undervoltage protection */
	unsigned gover: 1;		/* Whole group overvoltage protection */
	unsigned gunder: 1;		/* Whole group undervoltage protection */
	unsigned chitemp: 1;		/* Charge over temperature protection */
	unsigned clowtemp: 1;		/* Charge low temperature protection */
	unsigned dhitemp: 1;		/* Discharge over temperature protection */
	unsigned dlowtemp: 1;		/* Discharge low temperature protection */
	unsigned cover: 1;		/* Charge overcurrent protection */
	unsigned cunder: 1;		/* Discharge overcurrent protection */
	unsigned shorted: 1;		/* Short circuit protection */
	unsigned ic: 1;			/* Front detection IC error */
	unsigned mos: 1;		/* Software lock MOS */
};

struct jbd_info {
	char manufacturer[32];		/* Model name */
	char model[64];			/* Model name */
	char mfgdate[9];		/* the production date, YYYYMMDD, zero terminated */
	float version;			/* the software version */
};
typedef struct jbd_info jbd_info_t;

/* I/O */
int jbd_can_get_crc(jbd_session_t *s, int id, unsigned char *data, int len);
int jbd_can_get(jbd_session_t *s, int id, unsigned char *data, int datalen, int chk);
int jbd_eeprom_start(jbd_session_t *s);
int jbd_eeprom_end(jbd_session_t *s);
int jbd_rw(jbd_session_t *, uint8_t action, uint8_t reg, uint8_t *data, int datasz);
int jbd_verify(uint8_t *buf, int len);
int jbd_cmd(uint8_t *pkt, int pkt_size, int action, uint16_t reg, uint8_t *data, int data_len);
int jbd_rw(jbd_session_t *s, uint8_t action, uint8_t reg, uint8_t *data, int datasz);

/* Driver */
int jbd_open(void *handle);
int jbd_close(void *handle);

/* Read */
int jbd_get_fetstate(jbd_session_t *);
int jbd_read(void *handle,void *buf,int buflen);

/* Config */
int jbd_config(void *,int,...);
int jbd_config_add_params(json_value_t *);

/* Info */
int jbd_info(jbd_session_t *);

/* Control */
int jbd_control(void *handle,char *,char *,json_value_t *);
int charge_control(jbd_session_t *s, int);
int discharge_control(jbd_session_t *s, int);
int balance_control(jbd_session_t *s, int);

/* Misc */
int jbd_set_mosfet(jbd_session_t *s, int val);

#define jbd_getshort(p) ((short) ((*((p)) << 8) | *((p)+1) ))
#define jbd_putshort(p,v) { float tmp; *((p)) = ((int)(tmp = v) >> 8); *((p+1)) = (int)(tmp = v); }

#define JBD_PKT_START		0xDD
#define JBD_PKT_END		0x77
#define JBD_CMD_READ		0xA5
#define JBD_CMD_WRITE		0x5A

#define JBD_CMD_HWINFO		0x03
#define JBD_CMD_CELLINFO	0x04
#define JBD_CMD_HWVER		0x05
#define JBD_CMD_MOS		0xE1

#define JBD_MOS_CHARGE          0x01
#define JBD_MOS_DISCHARGE       0x02

#define JBD_FUNC_SWITCH 	0x01
#define JBD_FUNC_SCRL		0x02
#define JBD_FUNC_BALANCE_EN	0x04
#define JBD_FUNC_CHG_BALANCE	0x08
#define JBD_FUNC_LED_EN		0x10
#define JBD_FUNC_LED_NUM	0x20
#define JBD_FUNC_RTC		0x40
#define JBD_FUNC_EDV		0x80

#define JBD_NTC1		0x01
#define JBD_NTC2		0x02
#define JBD_NTC3		0x04
#define JBD_NTC4		0x08
#define JBD_NTC5		0x10
#define JBD_NTC6		0x20
#define JBD_NTC7		0x40
#define JBD_NTC8		0x80

#endif

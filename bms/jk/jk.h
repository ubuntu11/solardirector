
/*
Copyright (c) 2021, Stephen P. Shoecraft
All rights reserved.

This source code is licensed under the BSD-style license found in the
LICENSE file in the root directory of this source tree.
*/

#ifndef __JBD_H
#define __JBD_H

#include <stdint.h>
#include "battery.h"
#include "state.h"

struct jk_session {
	solard_agent_t *conf;		/* Our config */
	solard_module_t *tp;		/* Our transport */
	void *tp_handle;		/* Our transport handle */
	uint16_t state;			/* Pack state */
};
typedef struct jk_session jk_session_t;

struct jk_info {
	char manufacturer[32];		/* Model name */
	char model[64];			/* Model name */
	char mfgdate[9];		/* the production date, YYYYMMDD, zero terminated */
	float version;			/* the software version */
};
typedef struct jk_info jk_info_t;

/* I/O */
int jk_can_get_crc(jk_session_t *s, int id, unsigned char *data, int len);
int jk_can_get(jk_session_t *s, int id, unsigned char *data, int datalen, int chk);
int jk_eeprom_start(jk_session_t *s);
int jk_eeprom_end(jk_session_t *s);
int jk_rw(jk_session_t *, uint8_t action, uint8_t reg, uint8_t *data, int datasz);
int jk_verify(uint8_t *buf, int len);
int jk_cmd(uint8_t *pkt, int pkt_size, int action, uint16_t reg, uint8_t *data, int data_len);
int jk_rw(jk_session_t *s, uint8_t action, uint8_t reg, uint8_t *data, int datasz);

/* Main */
int jk_open(void *handle);
int jk_close(void *handle);

/* Read */
//int jk_can_get_pack(struct jk_session *s);
int jk_read(void *handle, void *, int);

/* Config */
int jk_config(void *,char *,char *,list);
int jk_config_add_params(json_object_t *);

/* Info */
//int jk_can_get_info(jk_session_t *s,jk_info_t *info);
//int jk_std_get_info(jk_session_t *s,jk_info_t *info);
//int jk_get_info(void *handle,jk_info_t *info);
char *jk_info(void *handle);

/* Control */
int jk_control(void *handle,...);

#endif


/*
Copyright (c) 2021, Stephen P. Shoecraft
All rights reserved.

This source code is licensed under the BSD-style license found in the
LICENSE file in the root directory of this source tree.
*/

#include "jbd.h"

int charge_control(jbd_session_t *s, int charge) {
	int opt,bytes;
	uint8_t data[2];

	dprintf(1,"charge: %d\n", charge);

	opt = 0;
	dprintf(2,"fetstate: %x\n", s->fetstate);
	/* Each bit set TURNS IT OFF */
	if ((s->fetstate & JBD_MOS_CHARGE)==0) opt = JBD_MOS_CHARGE;
	if ((s->fetstate & JBD_MOS_DISCHARGE)==0) opt = JBD_MOS_DISCHARGE;
	dprintf(2,"opt: %x\n", opt);
	if (charge == 0) opt |= JBD_MOS_CHARGE;
	else if (charge == 1) opt &= ~JBD_MOS_CHARGE;
	dprintf(2,"opt: %x\n", opt);
	jbd_putshort(data,opt);
	bytes = jbd_rw(s, JBD_CMD_WRITE, JBD_REG_MOSFET, data, sizeof(data));
	dprintf(1,"bytes: %d\n", bytes);
	return (bytes < 0 ? 1 : 0);
}

int discharge_control(jbd_session_t *s, int discharge) {
	int opt,bytes;
	uint8_t data[2];

	dprintf(1,"discharge: %d\n", discharge);

	opt = 0;
	dprintf(2,"fetstate: %x\n", s->fetstate);
	/* Each bit set TURNS IT OFF */
	if ((s->fetstate & JBD_MOS_CHARGE)==0) opt = JBD_MOS_CHARGE;
	if ((s->fetstate & JBD_MOS_DISCHARGE)==0) opt = JBD_MOS_DISCHARGE;
	dprintf(2,"opt: %x\n", opt);
	if (discharge == 0) opt |= JBD_MOS_DISCHARGE;
	else if (discharge == 1) opt &= ~JBD_MOS_DISCHARGE;
	dprintf(2,"opt: %x\n", opt);
	jbd_putshort(data,opt);
	bytes = jbd_rw(s, JBD_CMD_WRITE, JBD_REG_MOSFET, data, sizeof(data));
	dprintf(1,"bytes: %d\n", bytes);
	return (bytes < 0 ? 1 : 0);
}

int balance_control(jbd_session_t *s, int balance) {
	int opt,bytes;
	uint8_t data[2];

	dprintf(1,"balance: %d\n", balance);

	/* Get current value */
	bytes = jbd_rw(s, JBD_CMD_READ, JBD_REG_FUNCMASK, data, sizeof(data));
	dprintf(1,"bytes: %d\n", bytes);
	if (bytes < 0) return 1;
	opt = jbd_getshort(data);
	dprintf(2,"opt: %x\n", opt);
	switch(balance) {
	case 0:
		opt &= ~(JBD_FUNC_BALANCE_EN | JBD_FUNC_CHG_BALANCE);
		break;
	case 1:
		opt &= ~(JBD_FUNC_BALANCE_EN | JBD_FUNC_CHG_BALANCE);
		opt |= JBD_FUNC_BALANCE_EN;
		break;
	case 2:
		opt |= (JBD_FUNC_BALANCE_EN | JBD_FUNC_CHG_BALANCE);
		break;
	default:
		return 1;
	}
	dprintf(2,"opt: %x\n", opt);
	jbd_putshort(data,opt);
	bytes = jbd_rw(s, JBD_CMD_WRITE, JBD_REG_FUNCMASK, data, sizeof(data));
	dprintf(1,"bytes: %d\n", bytes);
	if (bytes < 0) return 1;

	/* For balance, we need to pub the config for it */
	s->balancing = balance;
	{
		solard_message_t *msg;
		char *payload = "[\"BatteryConfig\"]";

		msg = solard_message_alloc(payload,strlen(payload));
//		strcpy(msg->action,"Get");
		jbd_config(s,SOLARD_CONFIG_MESSAGE,msg);
		free(msg);
	}
	return 0;
}

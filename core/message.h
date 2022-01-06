
/*
Copyright (c) 2021, Stephen P. Shoecraft
All rights reserved.

This source code is licensed under the BSD-style license found in the
LICENSE file in the root directory of this source tree.
*/

#ifndef __SD_MESSAGE_H
#define __SD_MESSAGE_H

#include "mqtt.h"

/* message format is:
	Root (SolarD)
	Role (Controller/Inverter/Battery)
	Agent (pack_01/si/etc)
	Func (Config/Data/etc)
	Action  (Get/Set/Add/Del/etc)
*/

/* 64k atm */
//#define SOLARD_MAX_PAYLOAD_SIZE 65536
#define SOLARD_MAX_PAYLOAD_SIZE 1048576

struct solard_message {
	union {
		char id[SOLARD_ID_LEN];
		char name[SOLARD_NAME_LEN];
	};
	char func[SOLARD_FUNC_LEN];
	char replyto[SOLARD_ID_LEN];
	char data[SOLARD_MAX_PAYLOAD_SIZE];
//	char *data;
};
typedef struct solard_message solard_message_t;

solard_message_t *solard_getmsg(char *, char *, int, char *);
void solard_message_dump(solard_message_t *,int);
int solard_message_wait(list, int);

typedef int (solard_msghandler_t)(void *,solard_message_t *);

solard_message_t *solard_message_wait_id(list lp, char *id, int timeout);
solard_message_t *solard_message_wait_target(list lp, char *target, int timeout);

#endif

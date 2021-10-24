
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

struct solard_message {
	char role[SOLARD_ROLE_LEN];
	char name[SOLARD_NAME_LEN];
	char func[SOLARD_FUNC_LEN];
#if 0
	union {
		char action[SOLARD_ACTION_LEN];
		char param[SOLARD_ACTION_LEN];
	};
#endif
	char replyto[SOLARD_ID_LEN];
	int size;
	char *data;
	int data_len;
	char _d;
};
typedef struct solard_message solard_message_t;

solard_message_t *solard_message_alloc(char *data, int data_len);
void solard_message_free(solard_message_t *);
//void solard_getmsg(void *, char *, char *, int, char *);
solard_message_t *solard_getmsg(char *, char *, int, char *);
void solard_ingest(list, int);
void solard_message_dump(solard_message_t *,int);

typedef int (solard_msghandler_t)(void *,solard_message_t *);

#endif

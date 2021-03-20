
#ifndef __SD_COMMON_H
#define __SD_COMMON_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <stdarg.h>
#include <stdint.h>
#include <errno.h>
#include <time.h>

#include "types.h"
#include "opts.h"
#include "cfg.h"
#include "message.h"
#include "json.h"
#include "utils.h"
#include "debug.h"
#include "state.h"
#include "mqtt.h"

#if 0
/* MQTT message from agent/client */
struct solard_message {
        char topic[128];
        char payload[262144];
};
typedef struct solard_message solard_message_t;
#endif

int solard_common_init(int argc,char **argv,opt_proctab_t *add_opts,int start_opts);

#endif

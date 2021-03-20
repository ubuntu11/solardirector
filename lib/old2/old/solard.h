
#ifndef __SOLARD_H__
#define __SOLARD_H__

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <stdarg.h>
#include <stdint.h>
#include <errno.h>

#include "utils.h"
#include "mqtt.h"
#include "list.h"
#include "worker.h"
#include "debug.h"

//typedef unsigned char uint8_t;

#define SOLARD_MAX_INVERTERS	32
#define SOLARD_MAX_PACKS		32
#define SOLARD_PACK_NAME_LEN	32
#define SOLARD_PACK_TYPE_LEN	8
#define SOLARD_MODULE_NAME_LEN	32

#include "agent.h"
#include "module.h"
#include "inverter.h"
#include "pack.h"

#define SOLARD_TARGET_LEN 32

/* States */
#define SOLARD_SHUTDOWN		0x0001		/* Emergency Shutdown */
#define SOLARD_GEN_ACTIVE	0x0002		/* Generator Active */
#define SOLARD_CHARGING		0x0004		/* Charging */
#define SOLARD_CRIT_CELLVOLT	0x0008		/* A cell voltage is critical */
#define SOLARD_FORCE_SOC		0x0010		/* Force State of Charge */
#define SOLARD_CONFIG_DIRTY	0x0020		/* Config has been updated */
#define SOLARD_GRID_ENABLE	0x0040		/* Grid is enabled */
#define SOLARD_GEN_ENABLE	0x0080		/* Generator is enabled */
#define SOLARD_GRID_ACTIVE	0x0100		/* Grid is active */

/* State Macros */
#define solard_set_state(c,v)	(c->state |= (v))
#define solard_clear_state(c,v)	(c->state &= (~v))
#define solard_check_state(c,v)	((c->state & v) != 0)
#define solard_set_cap(c,v)	(c->capabilities |= (v))
#define solard_clear_cap(c,v)	(c->capabilities &= (~v))
#define solard_check_cap(c,v)	((c->capabilities & v) != 0)
#define solard_is_shutdown(c)	solard_check_state(c,SOLARD_SHUTDOWN)
#define solard_is_gen(c) 	solard_check_state(c,SOLARD_GEN_ACTIVE)
#define solard_is_charging(c)	solard_check_state(c,SOLARD_CHARGING)
#define solard_is_critvolt(c)	solard_check_state(c,SOLARD_CRIT_CELLVOLT)

#define solard_force_soc(c,v)	{ c->soc = v; solard_set_state(c,SOLARD_FORCE_SOC); }
#define solard_disable_charging(c) { c->charge_amps = 0.0; solard_clear_state(c,SOLARD_CHARGING); }

#endif

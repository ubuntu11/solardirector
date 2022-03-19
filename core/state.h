
/*
Copyright (c) 2021, Stephen P. Shoecraft
All rights reserved.

This source code is licensed under the BSD-style license found in the
LICENSE file in the root directory of this source tree.
*/

#ifndef __SD_STATE_H
#define __SD_STATE_H

#include "common.h"

#define solard_set_state(c,v)	solard_set_bit((c)->state,v)
#define solard_clear_state(c,v)	solard_clear_bit((c)->state,v)
#define solard_check_state(c,v)	solard_check_bit((c)->state,v)

#endif

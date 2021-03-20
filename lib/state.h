
#ifndef __SD_STATE_H
#define __SD_STATE_H

/* State Macros */
#define solard_set_state(c,v)	(c->state |= (v))
#define solard_clear_state(c,v)	(c->state &= (~v))
#define solard_check_state(c,v)	((c->state & v) != 0)
#define solard_set_cap(c,v)	(c->capabilities |= (v))
#define solard_clear_cap(c,v)	(c->capabilities &= (~v))
#define solard_check_cap(c,v)	((c->capabilities & v) != 0)

#endif

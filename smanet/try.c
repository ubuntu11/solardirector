
/*
Copyright (c) 2021, Stephen P. Shoecraft
All rights reserved.

This source code is licensed under the BSD-style license found in the
LICENSE file in the root directory of this source tree.
*/

#include <stdio.h>
#include <strings.h>
#include <unistd.h>

#include "smanet_internal.h"

#define DEBUG_STARTUP 0

#if DEBUG_STARTUP
#define _ST_DEBUG LOG_DEBUG
#else
#define _ST_DEBUG 0
#endif

extern solard_module_t serial_module;
extern solard_module_t rdev_module;

int main(int argc,char **argv) {
	int logopts = LOG_INFO|LOG_WARNING|LOG_ERROR|LOG_SYSERR|_ST_DEBUG;
	solard_module_t *tp;
	void *tp_handle;
	char *args[] = { "t2", "-d", "6" };
	#define nargs (sizeof(args)/sizeof(char *))
	smanet_session_t *s;
	smanet_value_t *v;
	char temp[256];
		smanet_channel_t *c;

	solard_common_init(nargs,args,0,logopts);
//	solard_common_init(argc,argv,opts,logopts);

#if 0
	tp = &rdev_module;
	tp_handle = tp->new(0,"192.168.1.7:3900","tty0");
#else
	tp = &serial_module;
	tp_handle = tp->new(0,"/dev/ttyS0","19200");
#endif
	dprintf(1,"tp_handle: %p\n", tp_handle);

	s = smanet_init(tp,tp_handle);
	if (!s) return 1;

	if (smanet_load_channels(s,"mychans.dat")) {
		dprintf(1,"getting channels...\n");
		smanet_get_channels(s);
		dprintf(1,"count: %d\n", list_count(s->channels));
		smanet_save_channels(s,"mychans.dat");
	}
	dprintf(1,"count: %d\n", list_count(s->channels));
#if 0
	{
		list_reset(s->channels);
		while((c = list_get_next(s->channels)) != 0) {
			dprintf(1,"chan: id: %d, mask: %04x, index: %d, name: %s, format: %04x, level: %d\n",
				c->id, c->mask, c->index, c->name, c->format, c->level);
			smanet_get_valuebyname(s,c->name,&v);
		}
	}
#endif
//	return 0;
	
#if 0
	smanet_get_valuebyname(s,"AutoStr",&v);
	dprintf(1,"AutoStr: %d\n", (int) smanet_get_value(&v));
	smanet_get_valuebyname(s,"SN",&v);
	dprintf(1,"SN: %d\n", (int) smanet_get_value(&v));
	smanet_get_valuebyname(s,"SNSlv1",&v);
	dprintf(1,"SNSlv1: %d\n", (int) smanet_get_value(&v));
	smanet_get_valuebyname(s,"SNSlv2",&v);
	dprintf(1,"SNSlv2: %d\n", (int) smanet_get_value(&v));
	smanet_get_valuebyname(s,"SNSlv3",&v);
	dprintf(1,"SNSlv3: %d\n", (int) smanet_get_value(&v));
#endif
	smanet_set_optionbyname(s,"GdManStr","Start");
//	smanet_clear_values(s);
//	if (smanet_get_optionbyname(s,"GnManStr",temp,sizeof(temp)-1) == 0) dprintf(1,"value: %s\n", temp);
//	smanet_set_valuebyname(s,"GnManStr",0);

//	smanet_save_channels(s,"mychans.dat");
//	smanet_syn_online(s);

//	c = smanet_get_channelbyname(s,"GnManStr");
//	smanet_getval(s,c);
//	v = smanet_get_value(s,c);
//	smanet_set_value(s,c);

	printf("timeouts: %d, commands: %d\n", s->timeouts, s->commands);
	return 0;
}

/*
Copyright (c) 2021, Stephen P. Shoecraft
All rights reserved.

This source code is licensed under the BSD-style license found in the
LICENSE file in the root directory of this source tree.
*/

#include "sb.h"
#include "__sd_build.h"
#include <sys/stat.h>

#define TESTING 0

char *sb_agent_version_string = "1.0-" STRINGIFY(__SD_BUILD);
char strfile[256],objfile[256];

int main(int argc, char **argv) {
	sb_session_t *s;
#if TESTING
	char *args[] = { "sb", "-N", "-d", "4", "-c", "sbtest.conf" };
	argc = sizeof(args)/sizeof(char *);
	argv = args;
#endif

	/* Init the driver */
	s = sb_driver.new(0,0);
	if (!s) return 1;

	/* Agent init */
	if (sb_agent_init(s,argc,argv)) return 1;

#if 0
	/* -E takes precedence over config */
	dprintf(1,"tpinfo: %s\n", tpinfo);
	if (strlen(tpinfo)) {
		*s->transport = *s->target = *s->topts = 0;
		strncat(s->transport,strele(0,",",tpinfo),sizeof(s->transport)-1);
		strncat(s->target,strele(1,",",tpinfo),sizeof(s->target)-1);
		strncat(s->topts,strele(2,",",tpinfo),sizeof(s->topts)-1);
	}
	dprintf(1,"s->transport: %s, s->target: %s, s->topts: %s\n", s->transport, s->target, s->topts);
	if (sb_tp_init(s)) {
		log_error(s->errmsg);
		return 1;
	}
#endif

	sb_driver.read(s,0,0,0);

	/* Main loop */
	dprintf(1,"norun_flag: %d", s->norun_flag);
	if (!s->norun_flag) agent_run(s->ap);
	sb_driver.close(s);
	return 0;
}


/*
Copyright (c) 2022, Stephen P. Shoecraft
All rights reserved.

This source code is licensed under the BSD-style license found in the
LICENSE file in the root directory of this source tree.
*/

function write_init() {
	return 0;
	dprintf(1,"disable_si_write: %s\n", si.disable_si_write);
	if (!si.disable_si_write) {
		log_warning("disabling si_write!\n");
		si.disable_si_write = true;
	}
}

function write_main() {
	//printf("rtsize: %d\n", si.agent.rtsize);
//	if (memused() > si.agent.rtsize) gc();
	if (si.startup == 0 && typeof(gc_done) == "undefined") {
		gc();
		gc_done = true;
	}
}


/*
Copyright (c) 2021, Stephen P. Shoecraft
All rights reserved.

This source code is licensed under the BSD-style license found in the
LICENSE file in the root directory of this source tree.
*/

#include "solard.h"

solard_battery_t *find_pack_by_name(solard_config_t *conf, char *name) {
	solard_battery_t *pp;

	dprintf(7,"name: %s\n", name);
	list_reset(conf->batteries);
	while((pp = list_get_next(conf->batteries)) != 0) {
		if (strcmp(pp->name,name) == 0) {
			dprintf(7,"found!\n");
			return pp;
		}
	}
	dprintf(7,"NOT found!\n");
	return 0;
}

int sort_batteries(void *i1, void *i2) {
	solard_battery_t *p1 = (solard_battery_t *)i1;
	solard_battery_t *p2 = (solard_battery_t *)i2;
	int val;

	dprintf(7,"p1: %s, p2: %s\n", p1->name, p2->name);
	val = strcmp(p1->name,p2->name);
	if (val < 0)
		val =  -1;
	else if (val > 0)
		val = 1;
	dprintf(7,"returning: %d\n", val);
	return val;

}

void getpack(solard_config_t *conf, char *name, char *data) {
	solard_battery_t bat,*pp = &bat;

	battery_from_json(&bat,data);
//	battery_dump(&bat,3);
	solard_set_state((&bat),BATTERY_STATE_UPDATED);
	time(&bat.last_update);

	pp = find_pack_by_name(conf,bat.name);
	if (!pp) {
		dprintf(7,"adding pack...\n");
		list_add(conf->batteries,&bat,sizeof(bat));
		dprintf(7,"sorting batteries...\n");
		list_sort(conf->batteries,sort_batteries,0);
	} else {
		dprintf(7,"updating pack...\n");
		memcpy(pp,&bat,sizeof(bat));
	}
	return;
};

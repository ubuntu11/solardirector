
#include <string.h>
#include "driver.h"
#include "debug.h"

solard_driver_t *find_driver(solard_driver_t **transports, char *name) {
	solard_driver_t *tp;
	int i;

	/* Find the transport in the list of transports */
	tp = 0;
	for(i=0; transports[i]; i++) {
		dprintf(1,"type: %d\n", transports[i]->type);
		if (transports[i]->type != SOLARD_DRIVER_TRANSPORT) continue;
		dprintf(1,"name: %s\n", transports[i]->name);
		if (strcmp(transports[i]->name,name)==0) {
			tp = transports[i];
			break;
		}
	}
	dprintf(1,"tp: %p\n", tp);
	return tp;
}

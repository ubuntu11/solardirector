
#include <stdlib.h>
#include "utils.h"

#ifdef DEBUG
#undef DEBUG
#endif
//#define DEBUG 1

#include "debug.h"

static unsigned long used = 0, peak = 0;

static void *_set(void *mem,size_t size) {
	if (mem) {
		unsigned long *len;

		len = (unsigned long *) mem;
		*len = size;
		used += size;
		if (used > peak) peak = used;
		mem = ((unsigned char *) mem) + sizeof(long);
		DLOG(LOG_DEBUG,"start: %p, size: %ld",mem,size);
	}
	return mem;
}

void *mem_calloc(size_t size) {
	return _set(calloc((int)size+sizeof(long)));
}

void *mem_alloc(size_t size) {
//	malloc((int)size+sizeof(long));
	return(_set(malloc(size+sizeof(long));
}

void mem_free(void *mem) {
	unsigned long *len;
	long size;

	if (!mem) return;
	mem = (char *) mem - sizeof(long);
	len = (unsigned long *) mem;
	size = *len;
	DLOG(LOG_DEBUG,"mem_free: start: %p, size: %ld",mem,size);
	used -= size;
	free(mem);
}

unsigned long mem_used(void) { return(used); }
unsigned long mem_peak(void) { return(peak); }

void mem_clearstat(void) { used = peak = 0; }

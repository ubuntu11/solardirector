
/*
Copyright (c) 2021, Stephen P. Shoecraft
All rights reserved.

This source code is licensed under the BSD-style license found in the
LICENSE file in the root directory of this source tree.
*/

#if defined(DEBUG_MEM) && !defined(__WIN64)
#include <stdlib.h>
#include "utils.h"
#include "debug.h"
#undef malloc
#undef calloc
#undef realloc
#undef free

static unsigned long used = 0, peak = 0;

unsigned long mem_used(void) { return(used); }
unsigned long mem_peak(void) { return(peak); }

void *mem_alloc(size_t size, int clear) {
	unsigned long *len;
	char *mem;

	if (clear)
		mem = (char *) calloc((int)size+sizeof(long),1);
	else
		mem = (char *) malloc((int)size+sizeof(long));
	if (mem) {
		len = (unsigned long *) mem;
		*len = size;
		used += size;
		if (used > peak)
			peak = used;
		mem = (char *) mem + sizeof(long);
		dprintf(9,"mem_alloc: start: %p, size: %ld\n",mem,size);
	}
	dprintf(9,"used: %ld\n",used);
	return(mem);
}

void *mem_malloc(size_t size) {
	return (mem_alloc(size,0));
}

void *mem_calloc(size_t num, size_t size) {
	return (mem_alloc((num*size),1));
}

void *mem_realloc(void *mem, size_t size) {
	unsigned long *len;

	mem = (char *) mem - sizeof(long);
	mem = realloc(mem,size);
	len = (unsigned long *) mem;
	*len = size;
	used += size;
	if (used > peak)
		peak = used;
	mem = (char *) mem + sizeof(long);
	dprintf(9,"mem_realloc: start: %p, size: %ld\n",mem,size);
	return mem;
}

void mem_free(void *mem) {
	unsigned long *len;
	long size;

	dprintf(9,"ptr: %p\n", mem);
	if (!mem) return;
	dprintf(9,"start: used: %ld\n",used);
	mem = (char *) mem - sizeof(long);
	len = (unsigned long *) mem;
	size = *len;
//	dprintf(9,"mem_free: start: %p, size: %ld\n",mem,size);
	used -= size;
	free(mem);
	dprintf(9,"end: used: %ld\n",used);
}
#endif

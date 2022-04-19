#ifdef DEBUG_MEM
/*
Copyright (c) 2021, Stephen P. Shoecraft
All rights reserved.

This source code is licensed under the BSD-style license found in the
LICENSE file in the root directory of this source tree.
*/

#include <stdlib.h>
#include "utils.h"
#include "debug.h"
#undef malloc
#undef calloc
#undef realloc
#undef free

#define MEM_ENABLE 1

static size_t used = 0, peak = 0;

size_t mem_used(void) { return(used); }
size_t mem_peak(void) { return(peak); }

void *mem_alloc(size_t size, int clear) {
	void *mem;

	if (clear)
		mem = calloc(size+sizeof(size_t),1);
	else
		mem = malloc(size+sizeof(size_t));
#if MEM_ENABLE
	if (mem) {
		*((size_t *)mem) = size;
		used += size;
		if (used > peak) peak = used;
		mem += sizeof(size_t);
	}
	dprintf(9,"mem: %p, size: %ld, used: %ld\n",mem,size,used);
#endif
	return(mem);
}

void *mem_malloc(size_t size) {
	return (mem_alloc(size,0));
}

void *mem_calloc(size_t num, size_t size) {
	return (mem_alloc((num*size),1));
}

void *mem_realloc(void *mem, size_t size) {
	void *newmem;

#if MEM_ENABLE
	if (mem) mem -= sizeof(size_t);
#endif
	newmem = realloc(mem,size+sizeof(size_t));
#if MEM_ENABLE
	used += size;
	if (used > peak) peak = used;
	*((size_t *)newmem) = size;
	newmem += sizeof(size_t);
	dprintf(9,"newmem: %p, size: %ld, used: %ld\n",newmem,size,used);
#endif
	return newmem;
}

void mem_free(void *mem) {
	size_t size;

	if (!mem) return;
#if MEM_ENABLE
	mem -= sizeof(size_t);
	size = *((size_t *)mem);
	used -= size;
	dprintf(9,"mem: %p, size: %ld, used: %ld\n", mem, size, used);
#endif
	free(mem);
}
#endif /* DEBUG_MEM */

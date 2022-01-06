
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
//	unsigned long *len;
//	unsigned char *mem;
	void *mem;

	if (clear)
		mem = calloc((int)size+sizeof(long),1);
	else
		mem = malloc((int)size+sizeof(long));
	if (mem) {
		*((size_t *)mem) = size;
		used += size;
		if (used > peak) peak = used;
		mem += sizeof(size_t);
	}
	dprintf(9,"mem: %p, size: %ld, used: %ld\n",mem,size,used);
//	printf("=====> mem_alloc: mem: %p, size: %ld, used: %ld\n",mem,size,used);
	return(mem);
}

void *mem_malloc(size_t size) {
	return (mem_alloc(size,0));
}

void *mem_calloc(size_t num, size_t size) {
	return (mem_alloc((num*size),1));
}

void *mem_realloc(void *mem, size_t size) {
	size_t oldsz;
	unsigned char *newmem;

	oldsz = *( (size_t *)(mem - sizeof(size_t)) );
//	printf("===> oldsz: %ld\n", oldsz);
	newmem = realloc((mem - sizeof(size_t)),size);
	if (!newmem) return 0;
	*((size_t *)newmem) = size;
	newmem += sizeof(size_t);
	used += (size - oldsz);
	if (used > peak) peak = used;
	dprintf(9,"newmem: %p, size: %ld, used: %ld\n",newmem,size,used);
//	printf("====> mem_realloc: newmem: %p, size: %ld, used: %ld\n",newmem,size,used);
	return newmem;
}

void mem_free(void *mem) {
	size_t size;

	dprintf(9,"mem: %p\n", mem);
	if (!mem) return;
	size = *( (size_t *)(mem - sizeof(size_t)) );
	used -= size;
	mem -= sizeof(size_t);
	dprintf(9,"mem: %p, size: %ld, used: %ld\n", mem, size, used);
//	printf("====> mem_free: mem: %p, size: %ld, used: %ld\n", mem, size, used);
	free(mem);
}
#endif

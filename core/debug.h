
/*
Copyright (c) 2021, Stephen P. Shoecraft
All rights reserved.

This source code is licensed under the BSD-style license found in the
LICENSE file in the root directory of this source tree.
*/

#ifndef __DEBUG_H
#define __DEBUG_H

#include <stdio.h>
#include "utils.h"

extern int debug;

#if defined(DEBUG) && DEBUG > 0
#define dprintf(level, format, args...) { if (debug >= level) log_write(LOG_DEBUG, "%s(%d) %s: " format,__FILE__,__LINE__, __FUNCTION__, ## args); }
#else
//#define dprintf(level,format,args...) printf("%s(%d) %s: " format,__FILE__,__LINE__, __FUNCTION__, ## args)
#define dprintf(level,format,args...) /* noop */
#endif

#if defined(DEBUG_MEM)
#if defined(MEMWATCH)
#include "memwatch.h"
#else
void *mem_alloc(size_t size, int clear);
void *mem_malloc(size_t size);
void *mem_calloc(size_t nmemb, size_t size);
void *mem_realloc(void *, size_t size);
void mem_free(void *mem);
size_t mem_used(void);
size_t mem_peak(void);
#ifdef malloc
#undef malloc
#endif
#define malloc(s) mem_alloc((s),0)
#ifdef calloc
#undef calloc
#endif
#define calloc(n,s) mem_alloc(((n)*(s)),1)
#ifdef realloc
#undef realloc
#endif
#define realloc(n,s) mem_realloc((n),(s))
#ifdef free
#undef free
#endif
#define free(s) mem_free((s))
#endif /* MEMWATCH */
#else
#define mem_used() 0
#define mem_peak() 0
#endif

#endif /* __DEBUG_H */

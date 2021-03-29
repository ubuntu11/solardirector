
#ifndef __DEBUG_H
#define __DEBUG_H

#include <stdio.h>
#include "utils.h"

extern int debug;

#if DEBUG
//#define dprintf(level, format, args...) { if (debug >= level) printf("%s(%d): " format,__FUNCTION__,__LINE__, ## args); }
#define dprintf(level, format, args...) { if (debug >= level) log_write(LOG_DEBUG, "%s(%d): " format, __FUNCTION__, __LINE__, ## args); }
#define DPRINTF(format, args...) printf("%s(%d): " format,__FUNCTION__,__LINE__, ## args)
#define DLOG(opts, format, args...) log_write(opts, "%s(%d): " format, __FUNCTION__, __LINE__, ## args)
#define DDLOG(format, args...) log_write(LOG_DEBUG, "%s(%d): " format, __FUNCTION__, __LINE__, ## args)
#else
#define dprintf(level,format,args...) /* noop */
#define DPRINTF(format, args...) /* noop */
#define DLOG(opts, format, args...) /* noop */
#define DDLOG(format, args...) /* noop */
#endif

/* dlevels
1-9 programs
2-9 libs
5 transports
7 protocol bindump 
*/


#ifdef DEBUG_MEM
void *mem_alloc(size_t size, int clear);
void *mem_malloc(size_t size);
void *mem_calloc(size_t nmemb, size_t size);
void mem_free(void *mem);
unsigned long mem_used(void);
unsigned long mem_peak(void);
#ifdef malloc
#undef malloc
#endif
#define malloc(s) mem_alloc(s,0)
#ifdef calloc
#undef calloc
#endif
#define calloc(n,s) mem_alloc((n)*(s),1)
#ifdef free
#undef free
#endif
#define free(s) mem_free(s)
#else
#include <stdlib.h>
#define mem_alloc(s,c) (c ? calloc(s,1) : malloc(s))
#define mem_free(m) free(m)
#define mem_used() 0L
#define mem_peak() 0L
#endif

#endif /* __DEBUG_H */

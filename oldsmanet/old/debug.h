#ifndef DEBUG_H

#include <stdio.h>
#include <assert.h>
#include "os.h"

/* all software parts should be modules */
//#define MODULE

/* Debug Levels to switch some parts on */
#define VERBOSE_WARNING       (1L)     /* general warnings: every time visible */
#define VERBOSE_MESSAGE       (1L<<1)  /* general message : every time visible */
#define VERBOSE_ERROR         (1L<<2)  /* general error   : every time visible */

#define VERBOSE_MISC          (1L<<3)  /* Misc debugs */
#define VERBOSE_HWL           (1L<<4)  /* Driver Layer ( Hardware Layer ) */
#define VERBOSE_SMADATALIB    (1L<<5)  /* SMAData Layer Object Debug */
#define VERBOSE_ROUTER 	      (1L<<6)  /* Router Object Debug */
#define VERBOSE_MASTER 	      (1L<<7)  /* SMAData-Master Debug */
#define VERBOSE_CHANNELLIST   (1L<<8)  /* Channellist debug */
#define VERBOSE_LIBRARY       (1L<<9)  /* Shared Library Debug */
#define VERBOSE_REPOSITORY    (1L<<10) /* Repository (Data Access) debug */
#define VERBOSE_FRACIONIZER   (1L<<11) /* Fractionizer Debug Messages*/

#define VERBOSE_BUGFINDER     (1L<<12) /* special debug finder */
#define VERBOSE_PACKETS       (1L<<13) /* viewing sendet packets */
#define VERBOSE_PROTLAYER     (1L<<14) /* debug protocol layer... */
#define VERBOSE_BUFMANAGEMENT (1L<<15) /* debugging buffer management */

#define VERBOSE_IOREQUEST     (1<<16)  /* IORequests... */
#define VERBOSE_MEMMANAGEMENT (1<<17)  // Memory Management
#define VERBOSE_SCHEDULER     (1<<18)  //Scheduler/timer debugs
#define VERBOSE_BCN           (1<<19)  //debugging bluetooth

#define VERBOSE_BCNECU VERBOSE_BCN

#include "debug.h"

#ifdef DEBUG
extern int debug;
#define YASDI_DEBUG(x) os_Debug x
#define LOG(x)         os_Debug x 
#else
#define YASDI_DEBUG(x)
#define LOG(x)
#define dprintf(format,args...) /* noop */
#endif

#endif


#ifndef __SIUTIL_H
#define __SITUIL_H

#include "si.h"

extern int outfmt;
extern FILE *outfp;
extern char sepch;
extern char *sepstr;

void display_info(si_session_t *);
int monitor(si_session_t *,int);

#endif

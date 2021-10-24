
/*
Copyright (c) 2021, Stephen P. Shoecraft
All rights reserved.

This source code is licensed under the BSD-style license found in the
LICENSE file in the root directory of this source tree.
*/

#ifndef __OPTS_H
#define __OPTS_H

struct opt_process_table {
	char *keyword;                  /* Keyword */
	void *dest;                     /* Dest addr */
	int type;                       /* Dest type */
	int len;                        /* Dest size */
	int reqd;                       /* Required flag */
	void *value;                    /* Default value */
	int have;                       /* Internal flags */
};
typedef struct opt_process_table opt_proctab_t;
#define OPTS_END { 0, 0, 0, 0, 0 ,0, 0 }

#ifdef __cplusplus
extern "C" {
#endif
void opt_init(opt_proctab_t *);
opt_proctab_t *opt_addopts(opt_proctab_t *std_opts,opt_proctab_t *add_opts);
int opt_process(int,char **,opt_proctab_t *);
int opt_getopt(int,char **,char *);
void opt_setopt(opt_proctab_t *,char *);
char *opt_type(char);
void opt_usage(char *,opt_proctab_t *);
#ifdef __cplusplus
}
#endif

#endif /* __OPTS_H */

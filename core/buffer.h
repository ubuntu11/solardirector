
/*
Copyright (c) 2022, Stephen P. Shoecraft
All rights reserved.

This source code is licensed under the BSD-style license found in the
LICENSE file in the root directory of this source tree.
*/

#ifndef __BUFFER_H
#define __BUFFER_H

#include <stdint.h>

typedef struct buffer_info buffer_t;

typedef int (buffer_read_t)(void *,uint8_t *,int);
struct buffer_info {
	uint8_t *buffer;		/* buffer */
	int size;			/* size of buffer */
	int bytes;			/* number of bytes in buffer */
	int index;			/* next read position in buffer */
	buffer_read_t *read;		/* read function */
	void *ctx;			/* read function context */
};

buffer_t *buffer_init(int,buffer_read_t *,void *);
void buffer_free(buffer_t *b);
int buffer_get(buffer_t *,uint8_t *,int);

#endif

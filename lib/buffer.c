
#include <stdlib.h>
#include "buffer.h"
#include "debug.h"

buffer_t *buffer_init(int size, buffer_read_t *func, void *ctx) {
	buffer_t *b;

	b = malloc(sizeof(*b));
	if (!b) return 0;
	b->buffer = malloc(size);
	if (!b->buffer) {
		free(b);
		return 0;
	}
	b->ctx = ctx;
	b->size = size;
	b->index = b->bytes = 0;
	b->read = func;

	return b;
}

void buffer_free(buffer_t *b) {
	if (!b) return;
	free(b->buffer);
	free(b);
}

int buffer_get(buffer_t *b, uint8_t *data, int datasz) {
	if (b->index >= b->bytes) {
		b->bytes = b->read(b->ctx, b->buffer, b->size);
		if (b->bytes < 0)  return -1;
	}
	dprintf(1,"bytes: %d, datasz: %d\n", b->bytes, datasz);
	return 0;
}

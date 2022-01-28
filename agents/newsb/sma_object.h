
#ifndef _SMA_OBJECT_H
#define _SMA_OBJECT_H

struct sma_object {
	char *id;
	int tag;
	int taghier[8];
	int unit;
	float mult;
};
typedef struct sma_object sma_object_t;

sma_object_t *getsmaobject(char *id);

#endif

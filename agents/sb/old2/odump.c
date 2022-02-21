
#include <stdio.h>
#include "sma_strings.c"
#include "sma_objects.c"

int main(void) {
	sma_object_t *o;
	char *s;
	char path[256];

	for(o=sma_objects; o->id; o++) {
		getsmaobjectpath(path,sizeof(path),o);
		printf("%s\n", path, s);
		continue;
		s = getsmastring(o->tag);
		if (!s) {
			printf("tag not found: %d\n", o->tag);
			continue;
		}
	}
}

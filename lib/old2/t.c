
#include <stdio.h>
#include <sys/stat.h>
#include <stdlib.h>
#include "json.h"

#define FNAME "test.json"

int main(void) {
	char *str;
	FILE *fp;
	struct stat buf;
	json_value_t *v,*v2,*v3;
	json_array_t *a;
	json_object_t *o;
	int i;

	stat(FNAME,&buf);
	printf("buf.st_size: %ld\n", buf.st_size);
	str = malloc(buf.st_size+1);
	fp = fopen(FNAME,"r");
	if (!fp) return 1;
	fread(str,1,buf.st_size,fp);
	fclose(fp);
	printf("str: %s\n", str);
	v = json_parse(str);
	printf("v: %p\n", v);
	printf("type: %d\n", v->type);
	switch(v->type) {
	case JSONString:
	case JSONNumber:
	case JSONObject:
		o = v->value.object;
		printf("o->count: %d\n", o->count);
		for(i=0; i < o->count; i++) {
			v3 = o->values[i];
		}
		break;
	case JSONArray:
		a = v->value.array;
		printf("a->count: %d\n", a->count);
		for(i=0; i < a->count; i++) {
			v2 = a->items[i];
			printf("v2 type: %d\n", v2->type);
			switch(v2->type) {
			case JSONObject:
				o = v2->value.object;
				printf("o->count: %d\n", o->count);
				for(i=0; i < o->count; i++) {
					printf("names[%d]: %s\n", i, o->names[i]);
					printf("values[%d]: %d\n", i, o->values[i]->type);
				}
				break;
			}
		}
		break;
	case JSONBoolean:
		break;
	}
}

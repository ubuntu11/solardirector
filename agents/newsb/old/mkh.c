
#include "common.h"
#include <sys/stat.h>

int main(void) {
	FILE *fp;
	char *buf;
	struct stat sb;
	int bytes;
	json_value_t *v;
	json_object_t *o;

	if (stat("sma_strings_en.json",&sb) < 0) {
		perror("stat");
		return 1;
	}
	buf = malloc(sb.st_size+1);
	if (!buf) return 1;
	fp = fopen("sma_strings_en.json","rb");
	if (!fp) return 1;
	bytes = fread(buf,1,sb.st_size,fp);
	fclose(fp);
	printf("bytes: %d\n", bytes);
	v = json_parse(buf);
	printf("v: %p\n", v);
	o = json_value_get_object(v);
	for(i=0; i < o->count; i++) {
	}
}

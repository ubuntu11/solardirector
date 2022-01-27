#include <stdio.h>
#include <string.h>

int main(void) {
	FILE *in, *out;
	char line[256], *p, *s;
	char msg[256], *d;
	int num;
	
	in = fopen("TagListEN-US.txt","r");
	while((fgets(line,sizeof(line),in))) {
		line[strlen(line)-1] = 0;
		s = strrchr(line,'\\');
		if (s) s++;
		p = strchr(line,'=');
		if (!p) continue;
		*p = 0;
		num = atoi(line);
		if (!s) s = p + 1;
		d = msg;
		for(p = s; *p; p++) {
			if (*p == '\"') *d++ = '\\';
			*d++ = *p;
		}
		*d = 0;
		printf("\t{ %d, \"%s\" },\n",num,msg);
	}
	fclose(in);
}

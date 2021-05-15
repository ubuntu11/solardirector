
#include <stdio.h>
#include <windows.h>

int main(void) {
	       static char value[2048];
	int r;

	r = GetEnvironmentVariable("PATH", value, sizeof(value)-1);
	printf("r: %d\n", r);
	if (r) printf("value: %s\n", value);
}


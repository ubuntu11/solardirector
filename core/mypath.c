
#include<stdio.h>
#include <unistd.h>
#include <string.h>
#include <libgen.h>

/* TODO: __WIN32 */
int getmypath(char *dest, int len) {
	return readlink ("/proc/self/exe", dest, len);
}

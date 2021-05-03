
#ifndef __WIN32
#include "common.h"
#include "debug.h"

#if 0
Check the environmental variable TMPDIR (getenv("TMPDIR")) only if the program is not running as SUID/SGID (issetugid() == 0)
Otherwise use P_tmpdir if it is defined and is valid
and finally, should those fail, use _PATH_TMP available from paths.h

ISO/IEC 9945 (POSIX): The path supplied by the first environment variable found in the list TMPDIR, TMP, TEMP, TEMPDIR. If none of these are found, "/tmp", or, if macro __ANDROID__ is defined, "/data/local/tmp"
#endif

static int issetugid(void) {
	if (getuid() != geteuid()) return 1; 
	if (getgid() != getegid()) return 1; 
	return 0; 
}

void tmpdir(char *dest, int dest_len) {
	char *p,*vars[] = { "TMPDIR", "TMP", "TEMP", "TEMPDIR" };
	int i;

	if (!dest) return;

	*dest = 0;
	if (!issetugid()) {
		for(i=0; i < (sizeof(vars)/sizeof(char *)); i++) {
//			dprintf(1,"trying: %s\n", vars[i]);
			p = getenv(vars[i]);
			if (p) break;
		}
	}
	if (!p) p = "/tmp";
	strncat(dest,p,dest_len);
//	dprintf(1,"dest: %s\n", dest);
	return;
}
#else
void tmpdir(char *dest, int dest_len) { GetTempPath(dest, dest_len); }
#endif

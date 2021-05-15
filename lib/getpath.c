
#include "common.h"
#include <ctype.h>

#define _MAX_PATHLEN 1024

void fixpath(char *string, int stringsz) {
	char newstr[2048],temp[128],last_ch,*e;
	register char *p,*s;
	int newidx,i,have_start;

	dprintf(1,"string: %s\n", string);

	s = string;
	newidx = have_start = 0;
	for(p = string; *p; p++) {
//		dprintf(1,"p: %c\n", *p);
		if (have_start) {
			if (*p == '%') {
				temp[i] = 0;
				dprintf(1,"temp: %s\n", temp);
				e = os_getenv(temp);
				dprintf(1,"e: %p\n", e);
				if (e) {
					strncpy(&newstr[newidx],e,strlen(e));
					newidx += strlen(e);
				} else {
					newidx += sprintf(&newstr[newidx],"%%%s%%",temp);
				}		
				s = p+1;
				have_start = 0;
				last_ch = 0;
			} else {
				temp[i++] = *p;
			}
		} else if (*p == '%') {
			if (last_ch == '%') {
				newstr[newidx++] = '%';
			} else {
				i = 0;
				have_start = 1;
				strncpy(&newstr[newidx],s,p-s);
				newidx += p-s;
			}
		} else {
			newstr[newidx++] = *p;
		}
		last_ch = *p;
	}
	dprintf(1,"have_start: %d\n", have_start);
	newstr[newidx] = 0;
#ifdef __WIN32
	i = 0;
	for(p = newstr; *p; p++) {
		if (*p == '/') *p = '\\';
		string[i++] = *p;
		if (i >= stringsz-1) break;
	}
	string[i] = 0;
#else
	strncpy(string,newstr,stringsz);
#endif
	dprintf(1,"NEW string: %s\n", string);
}

#ifdef __WIN32
#include <windows.h>
BOOL FileExists(LPCTSTR szPath) {
	DWORD dwAttrib = GetFileAttributes(szPath);
	return (dwAttrib != INVALID_FILE_ATTRIBUTES && !(dwAttrib & FILE_ATTRIBUTE_DIRECTORY));
}
#endif

static int _checkpath(char *path, int pathlen, char *prog, char *dest, int dest_len) {
	char temp[_MAX_PATHLEN];
	int r,sz;

	r = 0;
	*temp = 0;
	sz = sizeof(temp)-1;
	strncat(temp,path,(pathlen > sz ? sz : pathlen));
	sz -= pathlen;
	dprintf(1,"temp: %s\n", temp);
	strncat(temp,"/",sz); sz--;
	strncat(temp,prog,sz);
	sz -= strlen(prog);
#ifdef __WIN32
	if (strlen(temp) > 4 && strcmp(&temp[strlen(temp)-4],".exe") != 0) strncat(temp,".exe",sz);
#endif
	fixpath(temp,sizeof(temp)-1);
	dprintf(1,"temp: %s\n", temp);
#ifdef __WIN32
	if (FileExists(temp) != 0) {
#else
	if (access(temp,X_OK) == 0) {
#endif
		dprintf(1,"found!\n");
		strncpy(dest,temp,dest_len);
		r = 1;
	}
	dprintf(1,"returning: %d\n", r);
	return r;
}

/* Search the path for an executable */
int solard_get_path(char *prog, char *dest, int dest_len) {
//	char temp[256],*path,*s,*e;
	char *path,*s,*e;
	int len;

	dprintf(1,"prog: %s, dest: %p, dest_len: %d\n", prog, dest, dest_len);

	/* Check SOLARD_BINDIR first */
	len = strlen(SOLARD_BINDIR) > _MAX_PATHLEN-1 ? _MAX_PATHLEN-1 : strlen(SOLARD_BINDIR);
	if (_checkpath(SOLARD_BINDIR,len,prog,dest,dest_len)) return 0;

	path = os_getenv("PATH");
	dprintf(1,"env PATH: %s\n", path);
	if (!path) return 1;
	s = e = path;
	while(1) {
#ifdef __WIN32
		if (*e == 0 || *e == ';') {
#else
		if (*e == 0 || *e == ':') {
#endif
			len = (e - s) > _MAX_PATHLEN-1 ? _MAX_PATHLEN-1 : e - s;
			dprintf(1,"len: %d\n",len);
			if (len) if (_checkpath(s,len,prog,dest,dest_len)) return 0;
			if (*e == 0) break;
			s = e+1;
		}
		e++;
	}

	/* Not found */
	return 1;
}

#if 0
/* This one uses which command on old exec method */
int solard_get_path(char *prog, char *dest, int dest_len) {
	char cmd[256],*path;

	dprintf(1,"prog: %s, dest: %p, dest_len: %d\n", prog, dest, dest_len);

	sprintf(cmd,"which %s",prog);
	if (solard_exec(cmd,&path,0)) return 1;
	dprintf(1,"path: %s\n", path);
	*dest = 0;
	strncat(dest,path,dest_len);
	free(path);
	return 0;
}
#endif

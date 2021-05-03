
#include "common.h"
#include <ctype.h>

/* Search the path for an executable */
int solard_get_path(char *prog, char *dest, int dest_len) {
	char temp[256],*path,*s,*e;
	int len;

	dprintf(1,"prog: %s, dest: %p, dest_len: %d\n", prog, dest, dest_len);

	path = getenv("PATH");
	dprintf(1,"path: %s\n", path);
	if (!path) return 1;
	s = e = path;
	while(1) {
		if (*e == 0 || *e == ':') {
			len = (e - s) > sizeof(temp)-1 ? sizeof(temp)-1 : e - s;
			dprintf(1,"len: %d\n",len);
			*temp = 0;
			strncat(temp,s,len);
			dprintf(1,"temp: %s\n", temp);
			strncat(temp,"/",(sizeof(temp)-strlen(temp))-1);
			strncat(temp,prog,(sizeof(temp)-strlen(temp))-1);
			dprintf(1,"temp: %s\n", temp);
			if (access(temp,X_OK)==0) {
				*dest = 0;
				strncat(dest,temp,dest_len);
				return 0;
			}
			if (*e == 0) break;
			s = e+1;
		}
		e++;
	}

	/* Not found */
	return 1;
}

#if 0
int solard_exec(char *path, char **output, int ignore) {
	char buffer[1024],*outp,*p;
	list lines;
	int len,status;
	FILE *fp;

	dprintf(1,"path: %s, output: %p\n", path, output);

	fp = popen(path,"r");
	if (!fp) {
		log_write(LOG_DEBUG|LOG_SYSERR,"popen(%d)",path);
		return 1;
	}
	lines = list_create();
	if (!lines) {
		log_write(LOG_DEBUG|LOG_SYSERR,"list_create");
		return 1;
	}
	dprintf(1,"getting output...\n");
	while (fgets(buffer, sizeof(buffer)-1, fp) != NULL) {
		dprintf(7,"line: %s\n", buffer);
		list_add(lines,buffer,strlen(buffer)+1);
	}
	dprintf(1,"lines count: %d\n", list_count(lines));
	status = pclose(fp);
	dprintf(1,"status: %d\n", status);
	if (!ignore && status != 0) return 1;

	len = 0;
	list_reset(lines);
	while((p = list_get_next(lines)) != 0) len += strlen(p);

	/* Alloc the output */
//	dprintf(1,"len: %d\n", len);
	outp = malloc(len+1);
	if (!outp) {
		log_write(LOG_DEBUG|LOG_SYSERR,"malloc(%d)",len+1);
		return 1;
	}

	*outp = 0;
	list_reset(lines);
	while((p = list_get_next(lines)) != 0) strcat(outp,p);
//	dprintf(1,"output: %s\n", outp);

	if (len > 1) {
		while(isspace(outp[len-1])) len--;
		dprintf(1,"len: %d\n", len);
		outp[len] = 0;
	}

	*output = outp;
//	dprintf(1,"output: %s\n", *output);
	return 0;
}

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

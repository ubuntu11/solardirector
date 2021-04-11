
/*
Copyright (c) 2021, Stephen P. Shoecraft
All rights reserved.

This source code is licensed under the BSD-style license found in the
LICENSE file in the root directory of this source tree.
*/

#include "common.h"
#include <pwd.h>

int find_config_file(char *name, char *temp, int len) {
	long uid;
	struct passwd *pw;

	/* SD path */
#ifdef ETCDIR
	snprintf(temp,len,"%s/%s",ETCDIR,name);
	if (access(temp,R_OK)==0) return 0;
#endif

	/* Current path */
	strncpy(temp,name,len);
	if (access(temp,R_OK)==0) return 0;

	/* HOME/etc */
	uid = getuid();
	pw = getpwuid(uid);
	if (pw) {
		snprintf(temp,len,"%s/etc/%s",pw->pw_dir,name);
		if (access(temp,R_OK)==0) return 0;
	}

	/* /etc, /usr/local/etc */
	snprintf(temp,len,"/etc/%s",name);
	if (access(temp,R_OK)==0) return 0;
	snprintf(temp,len,"/usr/local/etc/%s",name);
	if (access(temp,R_OK)==0) return 0;
	return 1;
}

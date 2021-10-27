
#ifndef __SDSCRIPT_H
#define __SDSCRIPT_H

#include "jsapi.h"

struct solard_script {
	JSContext* context;
	JSScript* script;
	JSRuntime* runtime;
	char *filename;
	char *cfile;
	int state;
};
typedef struct solard_script solard_script_t;

enum SOLARD_SCRIPT_STATE {
	SOLARD_SCRIPT_STATE_STOPPED,
	SOLARD_SCRIPT_STATE_STARTED,
	SOLARD_SCRIPT_STATE_PAUSED,
	SOLARD_SCRIPT_STATE_ERROR
};

int script_running(solard_script_t *);
solard_script_t *script_start(char *filename);

#endif

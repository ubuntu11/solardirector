
#include "common.h"
#include "jsengine.h"

#define TESTING 0

int main(int argc, char **argv) {
	JSEngine *e;
	char script[256];
	opt_proctab_t opts[] = {
		{ "%|script",&script,DATA_TYPE_STRING,sizeof(script)-1,1,"" },
		{ 0 }
	};
#if TESTING
	char *args[] = { "sdjs", "-d", "7", "json.js" };
	argc = (sizeof(args)/sizeof(char *));
	argv = args;
#endif

	if (solard_common_init(argc,argv,opts,0xffff)) return 1;

	e = JS_EngineInit(64*1048576,(js_outputfunc_t *)printf);
	if (!e) {
		perror("JSEngineInit");
		return 1;
	}
	if (JS_EngineExec(e,script,0)) {
		printf("%s: %s\n",script,strerror(errno));
	}
	return 0;
}

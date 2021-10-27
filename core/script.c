
#include "script.h"

int script_end(solard_script_t *sp) {
	if (JS_IsInRequest(sp->runtime)) JS_EndRequest(sp->context);

	return 1;
}

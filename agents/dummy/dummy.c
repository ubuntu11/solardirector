
#include "agent.h"

struct dummy_session {
	solard_agent_t *ap;
};
typedef struct dummy_session dummy_session_t;

int dummy_config(void *h, int req, ...) {
	dummy_session_t *s = h;
	va_list va;

	va_start(va,req);
	dprintf(1,"req: %d\n", req);
	switch(req) {
	case SOLARD_CONFIG_INIT:
		s->ap = va_arg(va,solard_agent_t *);
		break;
	}
	return 0;
}

int main(void) {
	solard_driver_t dummy;
	dummy_session_t *s;

	memset(&dummy,0,sizeof(dummy));
	dummy.name = "dummy";
	dummy.config = dummy_config;
	s = calloc(sizeof(*s),1);
	agent_init(0,0,"1.0",0,&dummy,s,0,0);
	if (!s->ap) return 1;
	agent_run(s->ap);
	return 0;
}

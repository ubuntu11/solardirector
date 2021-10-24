
#include "agent.h"
#include "storage.h"

struct storage_session {
	solard_agent_t *ap;
	solard_driver_t *driver;
	void *handle;
	solard_storage_t info;
};
typedef struct storage_session storage_session_t;

#define SP ((storage_session_t *)h)

static int storage_get_config(storage_session_t *s) {
	solard_storage_t *bp = &s->info;
	char *section_name = s->ap->section_name;
	struct cfg_proctab packtab[] = {
//		{ section_name, "name", 0, DATA_TYPE_STRING,&bp->name,sizeof(bp->name)-1, s->ap->instance_name },
		{ section_name, "capacity", 0, DATA_TYPE_FLOAT,&bp->capacity, 0, 0 },
		CFG_PROCTAB_END
	};

	cfg_get_tab(s->ap->cfg,packtab);
	if (debug >= 3) cfg_disp_tab(packtab,0,1);
	return 0;
}

static void *storage_new(void *conf, void *driver, void *handle) {
	storage_session_t *s;

	s = calloc(1,sizeof(*s));
	if (!s) {
		log_write(LOG_SYSERR,"storage_new: calloc");
		return 0;
	}
	s->ap = conf;
	s->driver = driver;
	s->handle = handle;

	/* Get our specific config */
	storage_get_config(s);

	/* Update the role_data in agent conf */
	s->ap->role_data = &s->info;

	/* Save a copy of the name */
//	strncpy(s->name,s->info.name,sizeof(s->name)-1);

	return s;
}

static int storage_open(void *h) {
	return SP->driver->open(SP->handle);
}

static int storage_close(void *h) {
	return SP->driver->close(SP->handle);
	return 1;
}

static int storage_read(void *h, void *buf, int len) {
	return SP->driver->read(SP->handle,buf,len);
}

static int storage_write(void *h, void *buf, int len) {
	return SP->driver->write(SP->handle,buf,len);
}

static int storage_config(void *h, int req, ...) {
	storage_session_t *s = h;
	va_list ap;
	int r;

	r = 1;
	va_start(ap,req);
	dprintf(1,"req: %d\n", req);
	switch(req) {
	case SOLARD_CONFIG_GET_HANDLE:
		{
			void **hp;

			hp = va_arg(ap,void **);
			if (hp) {
				*hp = s->handle;
				r = 0;
			}
		}
		break;
	default:
		r = s->driver->config(s->handle,req,va_arg(ap,void *));
	}
	dprintf(1,"returning: %d\n", r);
	return r;
}

solard_driver_t storage_driver = {
	SOLARD_DRIVER_ROLE,
	"Storage",
	storage_new,
	storage_open,
	storage_close,
	storage_read,
	storage_write,
	storage_config,
};

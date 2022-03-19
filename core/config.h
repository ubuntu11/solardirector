
#ifndef __CONFIG_H
#define __CONFIG_H

#include "cfg.h"
#include "json.h"

enum CONFIG_FILE_FORMAT {
	CONFIG_FILE_FORMAT_UNKNOWN,
	CONFIG_FILE_FORMAT_INI,
	CONFIG_FILE_FORMAT_JSON,
	CONFIG_FILE_FORMAT_CUSTOM,
	CONFIG_FILE_FORMAT_MAX,
};

#define CONFIG_FLAG_READONLY	0x01	/* Do not update */
#define CONFIG_FLAG_NOSAVE	0x02	/* Do not save */
#define CONFIG_FLAG_NOID	0x04	/* Do not assign an ID */
#define CONFIG_FLAG_FILE	0x08	/* Exists in storage (file/mqtt) */
#define CONFIG_FLAG_FILEONLY	0x10	/* Exists in file only (not visible by mqtt/js) */
#define CONFIG_FLAG_ALLOC	0x20	/* Contents of dest have been malloc'd (and must be freed) */
#define CONFIG_FLAG_NOPUB	0x40	/* Do not publish */
#define CONFIG_FLAG_DIRTY	0x80	/* Has been updated since last write */

struct config_property {
	char *name;
	int type;			/* Data type */
	void *dest;			/* Ptr to storage */
	int dsize;			/* Size of storage */
	void *def;			/* Default value */
	uint16_t flags;			/* Flags */
        char *scope;			/* Scope (select/range/etc) */
        int nvalues;			/* # of scope values */
        void *values;			/* Scope values */
        int nlabels;			/* # of labels */
        char **labels;			/* Labels */
        char *units;			/* Units */
        float scale;			/* Scale for numerics */
	int precision;			/* Decimal places */
	/* Not exposed */
	int len;			/* actual length of storage */
	int id;				/* Property ID */
	int dirty;			/* Has been updated since last write */
};
typedef struct config_property config_property_t;

//typedef int (config_funccall_t)(void *ctx,char *name,char *value,char *errmsg);
typedef int (config_funccall_t)(void *ctx, list args, char *errmsg);
struct config_function {
	char *name;
	config_funccall_t *func;
	void *ctx;
	int nargs;
};
typedef struct config_function config_function_t;

#define CONFIG_SECTION_NAME_SIZE 64
struct config_section {
	char name[CONFIG_SECTION_NAME_SIZE];
	list items;
	uint32_t flags;
};
typedef struct config_section config_section_t;

struct config;
typedef int (config_reader_t)(struct config *, char *);
typedef int (config_writer_t)(struct config *);

#define CONFIG_FILENAME_SIZE 256
struct config {
	int id;
	list sections;
	list funcs;
	char filename[CONFIG_FILENAME_SIZE];
	enum CONFIG_FILE_FORMAT file_format;
	config_reader_t *reader;
	void *reader_ctx;
	config_writer_t *writer;
	void *writer_ctx;
	char errmsg[256];
	config_property_t **map;		/* ID map */
	int map_maxid;
	union {
		cfg_info_t *cfg;
		json_value_t *v;
	};
	int dirty;
};
typedef struct config config_t;

config_t *config_init(char *, config_property_t *,config_function_t *);
char *config_get_errmsg(config_t *);
void config_dump(config_t *cp);
config_property_t *config_combine_props(config_property_t *p1, config_property_t *p2);
void config_add_props(config_t *, char *, config_property_t *, int flags);
config_function_t *config_combine_funcs(config_function_t *f1, config_function_t *f2);
void config_add_funcs(config_t *, config_function_t *);
int config_add_info(config_t *, json_object_t *);

config_section_t *config_get_section(config_t *, char *);
int config_read(config_t *, char *, enum CONFIG_FILE_FORMAT);
int config_set_filename(config_t *, char *, enum CONFIG_FILE_FORMAT);
int config_set_reader(config_t *, config_reader_t *, void *);
int config_set_writer(config_t *, config_writer_t *, void *);

config_property_t *config_section_dup_property(config_section_t *, config_property_t *, char *);

config_section_t *config_get_section(config_t *,char *);
config_section_t *config_create_section(config_t *,char *,int);
config_property_t *config_get_property(config_t *cp, char *sname, char *name);
int config_delete_property(config_t *cp, char *sname, char *name);
int config_set_property(config_t *cp, char *sname, char *name, int type, void *src, int len);
config_property_t *config_section_get_property(config_section_t *s, char *name);
int config_section_get_properties(config_section_t *s, config_property_t *props);
config_property_t *config_section_add_property(config_t *cp, config_section_t *s, config_property_t *p, int flags);
config_property_t *config_section_add_props(config_t *cp, config_section_t *s, config_property_t *p);

config_function_t *config_function_get(config_t *, char *name);
int config_function_set(config_t *, char *name, config_function_t *);

json_value_t *config_to_json(config_t *cp, int noflags, int dirty);
int config_from_json(config_t *cp, json_value_t *v);
int config_write(config_t *);
int config_add(config_t *cp,char *section, char *label, char *value);
int config_del(config_t *cp,char *section, char *label, char *value);
int config_get(config_t *cp,char *section, char *label, char *value);
//int config_set(config_t *cp,char *section, char *label, char *value);
//int config_pub(config_t *cp, mqtt_session_t *m);
config_property_t *config_find_property(config_t *cp, char *name);
config_property_t *config_get_propbyid(config_t *cp, int);

//typedef int (config_process_callback_t)(void *,char *,char *,char *,char *);
int config_process(config_t *cp, char *req);


void config_build_propmap(config_t *cp);

#define CONFIG_GETMAP(cp,id) ((cp->map && id >= 0 && id < cp->map_maxid && cp->map[id]) ? cp->map[id] : 0)

#ifdef JS
#include "jsapi.h"
#include "jsengine.h"
JSPropertySpec *config_to_props(config_t *cp, char *name, JSPropertySpec *add);
JSBool config_jsgetprop(JSContext *cx, JSObject *obj, jsval id, jsval *rval, config_t *, JSPropertySpec *);
JSBool config_jssetprop(JSContext *cx, JSObject *obj, jsval id, jsval *rval, config_t *, JSPropertySpec *);
JSObject *JSConfig(JSContext *cx, void *priv);
int config_jsinit(JSEngine *e, config_t *cp);
#endif

#endif

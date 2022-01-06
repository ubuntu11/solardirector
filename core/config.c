
#define DEBUG_CONFIG 1
#define dlevel 4

#ifdef DEBUG
#undef DEBUG
#endif
#define DEBUG DEBUG_CONFIG
#include "debug.h"

#include "common.h"
#include "config.h"
#include <strings.h>
#include <ctype.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>

char *config_get_errmsg(config_t *cp) { return cp->errmsg; }

config_function_t *config_function_get(config_t *cp, char *name) {
	config_function_t *f;

	dprintf(dlevel,"name: %s\n", name);

	list_reset(cp->funcs);
	while((f = list_get_next(cp->funcs)) != 0) {
		if (strcasecmp(f->name,name) == 0) {
			dprintf(dlevel,"found\n");
			return f;
		}
	}
	dprintf(dlevel,"NOT found\n");
	return 0;
}

config_property_t *config_section_dup_property(config_section_t *s, config_property_t *p, char *newname) {
	config_property_t newp;

	if (!newname) return 0;

	newp = *p;
	newp.name = newname;
	newp.dirty =1 ;
	return list_add(s->items,&newp,sizeof(newp));
}

config_property_t *config_section_get_property(config_section_t *s, char *name) {
	config_property_t *p;

	if (!s) return 0;

	dprintf(dlevel,"section: %s, name: %s\n", s->name, name);

	list_reset(s->items);
	while((p = list_get_next(s->items)) != 0) {
		dprintf(7,"p->name: %s\n", p->name);
		if (strcasecmp(p->name,name)==0) {
			dprintf(dlevel,"found\n");
			return p;
		}
	}
	dprintf(dlevel,"NOT found\n");
	return 0;
}

static void *_getstr(int count, void *values) {
	char **s, **newstr;
	int i;

	newstr = malloc(count * sizeof(char *));
	if (!newstr) return 0;
	s = values;
	for(i = 0; i < count; i++) {
		newstr[i] = malloc(strlen(s[i])+1);
		if (!newstr[i]) return 0;
		strcpy(newstr[i],s[i]);
	}
	return newstr;
}

config_property_t * config_section_add_property(config_t *cp, config_section_t *s, config_property_t *p, int flags) {
	config_property_t *pp,*rp;

	dprintf(dlevel,"prop: name: %s, id: %d, flags: %02x, def: %p\n",p->name, p->id, p->flags, p->def);

	pp = config_section_get_property(s, p->name);
	dprintf(dlevel,"p: %p, pp: %p\n", p, pp);
	/* Item alread added (via dup?) */
	if (pp == p) return 0;
	if (pp) {
		if (pp->flags & CONFIG_FLAG_FILEONLY) {
			printf("==> freepp: name: %p, dest: %p\n", pp->name, pp->dest);
			free(pp->name);
			free(pp->dest);
		}
		list_delete(s->items,pp);
	}
	dprintf(dlevel,"nvalues: %d, values: %p\n", p->nvalues, p->values);
	if (p->nvalues && p->values) {
		if (DATA_TYPE_ISNUMBER(p->type) || DATA_TYPE_ISARRAY(p->type)) {
			void *values;
			int size;

			size = typesize(p->type) * p->nvalues;
			dprintf(dlevel,"size: %d\n", size);
			values = malloc(size);
			if (values) memcpy(values,p->values,size);
			p->values = values;
		} else if (p->type == DATA_TYPE_STRING) {
			p->values = _getstr(p->nvalues, p->values);
		} else {
			dprintf(dlevel,"unhandled type: %d(%s)\n", p->type, typestr(p->type));
		}
	}
	dprintf(dlevel,"nlabels: %d, labels: %p\n", p->nlabels, p->labels);
	if (p->nlabels && p->labels) {
		p->labels = _getstr(p->nlabels, p->labels);
	}
	if (!solard_check_bit(flags,CONFIG_FLAG_NOID)) p->id = (p->flags & CONFIG_FLAG_NOID ? -1 : cp->id++);
	if (p->def && p->dest) p->len = conv_type(p->type, p->dest, p->dsize, DATA_TYPE_STRING, p->def, strlen(p->def));
	dprintf(dlevel,"adding: %s\n", p->name);
	rp = list_add(s->items, p, sizeof(*p));
	return rp;
}

void config_build_propmap(config_t *cp) {
	config_section_t *s;
	config_property_t *p;

	if (cp->map) free(cp->map);
	cp->map = malloc(cp->id * sizeof(*p));
	if (!cp->map) {
		log_syserror("config_build_propmap: malloc");
		return;
	}
	list_reset(cp->sections);
	while((s = list_get_next(cp->sections)) != 0) {
		list_reset(s->items);
		while((p = list_get_next(s->items)) != 0) {
			if (p->id < 0 || p->id > cp->id) continue;
			cp->map[p->id] = p;
		}
	}
	cp->map_maxid = cp->id;
}

config_section_t *config_create_section(config_t *cp, char *name, int flags) {
	config_section_t newsec;

	dprintf(dlevel,"name: %s, flags: %02x\n", name, flags);

	memset(&newsec,0,sizeof(newsec));

	/* Name can be null */
	if (name) strncpy(newsec.name,name,sizeof(newsec.name)-1);
	newsec.flags = flags;
	newsec.items = list_create();

	/* Add the new section to the list */
	return list_add(cp->sections,&newsec,sizeof(newsec));
}

config_section_t *config_get_section(config_t *cp,char *name) {
	config_section_t *section;

	if (!cp) return 0;

	/* Find the section */
	dprintf(dlevel,"looking for section: %s", name);
	list_reset(cp->sections);
	while( (section = list_get_next(cp->sections)) != 0) {
		dprintf(dlevel,"name: %s", section->name);
		if (strcasecmp(section->name,name)==0) {
			dprintf(dlevel,"found");
			*cp->errmsg = 0;
			return section;
		}
	}
	dprintf(dlevel,"NOT found!");
	sprintf(cp->errmsg,"unable to find config section: %s\n", name);
	return 0;
}

void config_add_props(config_t *cp, char *section_name, config_property_t *props, int flags) {
	config_property_t *p;
	config_section_t *section;

	if (!cp || !props) return;

	section = config_get_section(cp,section_name);
	if (!section) section = config_create_section(cp,section_name,flags);
	if (!section) return;
	for(p = props; p->name; p++) config_section_add_property(cp, section, p, flags);
}

void config_add_funcs(config_t *cp, config_function_t *funcs) {
	config_function_t *f;

	if (!cp || !funcs) return;
	for(f = funcs; f->name; f++) list_add(cp->funcs, f, sizeof(*f));
}

config_t *config_init(char *section_name, config_property_t *props, config_function_t *funcs) {
	config_t *cp;

	cp = calloc(sizeof(*cp),1);
	if (!cp) {
		log_syserror("config_init: calloc");
		return 0;
	}
	/* Ingest the props */
	cp->sections = list_create();
	cp->funcs = list_create();
	cp->id = 1;
	if (props) config_add_props(cp,section_name,props,0);
	if (funcs) config_add_funcs(cp,funcs);

	return cp;
}

void config_dump(config_t *cp) {
	config_section_t *section;
	config_property_t *p;
	char value[1024];

	dprintf(dlevel,"dumping...\n");

	list_reset(cp->sections);
	while((section = list_get_next(cp->sections)) != 0) {
		dprintf(dlevel,"section: %s\n", section->name);
		list_reset(section->items);
		while((p = list_get_next(section->items)) != 0) {
			if (p->dest) conv_type(DATA_TYPE_STRING,value,sizeof(value)-1,p->type, p->dest, p->len);
			else *value = 0;
			dprintf(dlevel,"  prop: name: %s, id: %d, type: %d(%s), value: %s\n", p->name, p->id, p->type, typestr(p->type), value);
		}
	}
}

config_property_t *config_get_property(config_section_t *section, char *name) {
	config_property_t *p;

	dprintf(dlevel,"name: %s\n", name);

	list_reset(section->items);
	while((p = list_get_next(section->items)) != 0) {
		if (strcasecmp(p->name,name) == 0) {
			dprintf(dlevel,"found\n");
			return p;
		}
	}
	dprintf(dlevel,"NOT found\n");
	return 0;
}

#if 0
static json_proctab_t *config_jsontab(config_t *cp, config_section_t *s) {
	json_proctab_t *tab,*tp;
	config_property_t *p;
	config_section_t *cs;
	list ds;
	int count,size;

	dprintf(dlevel,"s->name: %s\n", s->name);

	count=0;
	ds = list_dup(cp->sections);
	list_reset(ds);
	while((cs = list_get_next(ds)) != 0) {
		list_reset(cs->items);
		while((p = list_get_next(cs->items)) != 0) {
			if (p->flags & CONFIG_FLAG_NOSAVE) continue;
			count++;
		}
	}
	list_destroy(ds);
	count++;
	dprintf(dlevel,"count: %d\n", count);

	size = count * sizeof(*tab);
	dprintf(dlevel,"size: %d\n", size);
	tab = calloc(1,size);
	if (!tab) {
		sprintf(cp->errmsg,"config_jsontab: malloc(%d): %s\n", size, strerror(errno));
	}

	tp = tab;
	list_reset(s->items);
	while((p = list_get_next(s->items)) != 0) {
		tp->field = p->name;
		tp->type = p->type;
		tp->ptr = p->dest;
		tp->len = p->len;
		tp++;
	}
	tp->field = 0;

	for(tp = tab; tp->field; tp++) dprintf(dlevel,"tp: field: %s, ptr: %p, type: %d, cb: %p\n", tp->field, tp->ptr, tp->type, tp->cb);

	return tab;
}
#endif

int config_parse_json(config_t *cp, json_value_t *v) {
	json_object_t *o;
	config_section_t *s;
//	json_proctab_t *tab;
	int i,type;

	type = json_value_get_type(v);
	dprintf(dlevel,"type: %d (%s)\n", type, json_typestr(type));
	if (type != JSON_TYPE_OBJECT) {
		strcpy(cp->errmsg,"invalid json format");
		return 1;
	}
	o = json_value_get_object(v);
	if (!o) {
		strcpy(cp->errmsg,"invalid json format");
		return 1;
	}
	dprintf(dlevel,"object count: %d\n", o->count);
	for(i=0; i < o->count; i++) {
//		sname = o->names[i];
		dprintf(dlevel,"section: %s\n", o->names[i]);
		s = config_get_section(cp, o->names[i]);
		if (!s) continue;
#if 0
		tab = config_jsontab(cp, s);
		json_to_tab(tab, o->values[i]);
		free(tab);
			p = config_section_get_property(sec,item->keyword);
			if (p) {
				p->len = conv_type(p->type,p->dest,p->dsize,DATA_TYPE_STRING,item->value,strlen(item->value));
			} else {
				memset(&newprop,0,sizeof(newprop));
				newprop.name = malloc(strlen(item->keyword)+1);
				strncpy(newprop.name,item->keyword,strlen(item->keyword)+1);
				newprop.type = DATA_TYPE_STRING;
				newprop.dest = malloc(strlen(item->value)+1);
				strcpy(newprop.dest,item->value);
				newprop.len = strlen(item->value)+1;
				newprop.flags |= CONFIG_FLAG_FILEONLY;
				dprintf(dlevel,"adding...\n");
				list_add(sec->items,&newprop,sizeof(newprop));
			}
#endif
	}

	return 0;
}

#if 0
cfg_proctab_t *config_cfgtab(config_t *cp) {
	config_section_t *s;
	config_property_t *p;
	cfg_proctab_t *tab,*tp;
	list ds;
	int count,size;

	/* Get a count of all props */
	count=0;
	ds = list_dup(cp->sections);
	list_reset(ds);
	while((s = list_get_next(ds)) != 0) {
		list_reset(s->items);
		while((p = list_get_next(s->items)) != 0) {
			if (p->flags & CONFIG_FLAG_NOSAVE) continue;
			count++;
		}
	}
	list_destroy(ds);
	count++;
	dprintf(dlevel,"count: %d\n", count);
	size = count * sizeof(*tab);
	dprintf(dlevel,"size: %d\n", size);
	tab = calloc(size,1);
	dprintf(dlevel,"tab: %p\n", tab);
	if (!tab) {
		sprintf(cp->errmsg,"config_cfgtab: malloc(%d): %s\n", size, strerror(errno));
		return 0;
	}

	tp = tab;
//	dprintf(dlevel,"section count: %d\n", list_count(cp->sections));
	list_reset(cp->sections);
	while((s = list_get_next(cp->sections)) != 0) {
//		dprintf(dlevel,"s->name: %s\n", s->name);
		list_reset(s->items);
		while((p = list_get_next(s->items)) != 0) {
//			dprintf(dlevel,"prop: name: %s, type: %d(%s), flags: %02x\n", p->name, p->type, typestr(p->type), p->flags);
			if (p->flags & CONFIG_FLAG_NOSAVE) continue;
			tp->section = s->name;
			tp->keyword = p->name;
			tp->type = p->type;
			tp->dest = p->dest;
			tp->dlen = p->len;
			tp->def = 0;
			tp++;
		}
	}
	tp->section = tp->keyword = 0;

	for(tp = tab; tp->keyword; tp++) dprintf(dlevel,"tab: section: %s, keyword: %s, type: %d, dest: %p, dlen: %d\n", 
		tp->section, tp->keyword, tp->type, tp->dest, tp->dlen);
	return tab;
}
#endif

int config_read_ini(config_t *cp, char *filename) {
	cfg_info_t *cfg;
	config_section_t *sec;
	cfg_section_t *cfgsec;
	cfg_item_t *item;
	config_property_t *p,newprop;

	dprintf(dlevel,"filename: %s\n", filename);
	cfg = cfg_read(filename);
	dprintf(dlevel,"cfg: %p\n", cfg);
	if (!cfg) {
		sprintf(cp->errmsg, "unable to read ini file");
		return 1;
	}
	list_reset(cfg->sections);
	while((cfgsec = list_get_next(cfg->sections)) != 0) {
		dprintf(dlevel,"cfgsec->name: %s\n", cfgsec->name);
		sec = config_get_section(cp, cfgsec->name);
		if (!sec) sec = config_create_section(cp, cfgsec->name,CONFIG_FLAG_FILEONLY);
		list_reset(cfgsec->items);
		while((item = list_get_next(cfgsec->items)) != 0) {
			dprintf(dlevel,"item->keyword: %s\n", item->keyword);
			p = config_section_get_property(sec,item->keyword);
			if (p) {
				p->len = conv_type(p->type,p->dest,p->dsize,DATA_TYPE_STRING,item->value,strlen(item->value));
			} else {
				memset(&newprop,0,sizeof(newprop));
				newprop.name = malloc(strlen(item->keyword)+1);
				strncpy(newprop.name,item->keyword,strlen(item->keyword)+1);
				newprop.type = DATA_TYPE_STRING;
				newprop.dest = malloc(strlen(item->value)+1);
				strcpy(newprop.dest,item->value);
				newprop.dsize = strlen(item->value)+1;
				newprop.len = strlen(item->value);
				newprop.flags |= CONFIG_FLAG_FILEONLY;
				dprintf(dlevel,"adding...\n");
				list_add(sec->items,&newprop,sizeof(newprop));
			}
		}
	}

	return 0;
}

#if 0
int config_read_ini(config_t *cp, char *filename) {
//	cfg_info_t *cfg;
	cfg_proctab_t *tab;

	if (cp->cfg) cfg_destroy(cp->cfg);

	dprintf(dlevel,"filename: %s\n", filename);
	cp->cfg = cfg_read(filename);
	dprintf(dlevel,"cfg: %p\n", cp->cfg);
	if (!cp->cfg) {
		sprintf(cp->errmsg, "unable to read ini file");
		return 1;
	}
	tab = config_cfgtab(cp);
	dprintf(dlevel,"tab: %p\n", tab);
	if (!tab) return 1;
	cfg_get_tab(cp->cfg, tab);
	cfg_disp_tab(tab, 0, 0);
	free(tab);

	return 0;
}
#endif

int config_read_json(config_t *cp, char *filename) {
	json_value_t *v;
	struct stat sb;
	int fd,size,bytes;
	char *buf;

	/* Unable to open is not an error... */
	dprintf(dlevel,"filename: %s\n", filename);
	fd = open(filename,O_RDONLY);
	dprintf(dlevel,"fd: %d\n", fd);
	if (fd < 0) return 0;

	/* Get file size */
	size = (fstat(fd, &sb) < 0 ?  size = 1048576 : sb.st_size);
	dprintf(dlevel,"size: %d\n", size);

	/* Alloc buffer to hold it */
	buf = malloc(size);
	if (!buf) {
		sprintf(cp->errmsg, "config_read_json: malloc(%d): %s\n", size, strerror(errno));
		close(fd);
		return 1;
	}

	/* Read entire file into buffer */
	bytes = read(fd, buf, size);
	dprintf(dlevel,"bytes: %d\n", bytes);
	if (bytes != size) {
		sprintf(cp->errmsg, "config_read_json: read bytes(%d) != size(%d)\n", bytes, size);
		free(buf);
		close(fd);
		return 1;
	}
	close(fd);

	/* Parse the buffer */
	v = json_parse(buf);
	free(buf);
	if (!v) {
		sprintf(cp->errmsg, "config_read_json: error parsing json data\n");
		return 1;
	}
	if (config_parse_json(cp,v)) {
		json_destroy_value(v);
		return 1;
	}

	return 0;
}

int config_read(config_t *cp, char *filename, enum CONFIG_FILE_FORMAT format) {

	dprintf(dlevel,"filename: %s\n", filename);
	if (!strlen(filename)) {
		sprintf(cp->errmsg, "filename is empty");
		return 1;
	}

	if (!cp->reader) {
		if (format == CONFIG_FILE_FORMAT_INI)
			cp->reader = config_read_ini;
		else if (format == CONFIG_FILE_FORMAT_JSON)
			cp->reader = config_read_json;
		else {
			sprintf(cp->errmsg, "config_read: invalid file format: %d\n", format);
			return 1;
		}
		cp->reader_ctx = cp;
	}
	if (cp->reader(cp->reader_ctx,filename)) return 1;

	strncpy(cp->filename, filename, sizeof(cp->filename)-1);
	cp->file_format = format;

	return 0;
}

int config_write_ini(config_t *cp) {
	char value[1024];
	cfg_info_t *cfg;
	config_section_t *s;
	config_property_t *p;

	cfg = cfg_create(cp->filename);
	if (!cfg) {
		sprintf(cp->errmsg, "unable to create ini info: %s",strerror(errno));
		return 1;
	}
	list_reset(cp->sections);
	while((s = list_get_next(cp->sections)) != 0) {
		dprintf(dlevel,"s->flags: %02x\n", s->flags);
		if (s->flags & CONFIG_FLAG_NOSAVE) continue;
		list_reset(s->items);
		while((p = list_get_next(s->items)) != 0) {
			dprintf(dlevel,"p->flags: %02x\n", p->flags);
			if (p->flags & CONFIG_FLAG_NOSAVE) continue;
			conv_type(DATA_TYPE_STRING,value,sizeof(value)-1,p->type,p->dest,p->len);
			if (!strlen(value)) continue;
			dprintf(dlevel,"section: %s, item: %s, value(%d): %s\n", s->name, p->name, strlen(value), value);
			cfg_set_item(cfg, s->name, p->name, 0, value);
		}
	}
	if (cfg_write(cfg)) {
		sprintf(cp->errmsg, "unable to write ini file: %s",strerror(errno));
		return 1;
	}
	return 0;
}

json_value_t *config_to_json(config_t *cp) {
	config_section_t *s;
	config_property_t *p;
	json_object_t *co,*so;
	json_value_t *v;

	co = json_create_object();
	list_reset(cp->sections);
	while((s = list_get_next(cp->sections)) != 0) {
		if (s->flags & CONFIG_FLAG_NOSAVE) continue;
		dprintf(dlevel,"section: %s\n", s->name);
		if (!list_count(s->items)) continue;
		so = json_create_object();
		list_reset(s->items);
		while((p = list_get_next(s->items)) != 0) {
			if (p->flags & CONFIG_FLAG_NOSAVE) continue;
			v = json_from_type(p->type,p->dest,p->len);
			if (!v) continue;
			json_object_set_value(so,p->name,v);
		}
		json_object_set_object(co,s->name,so);
	}

	return json_object_get_value(co);
}

int config_write_json(config_t *cp) {
	char data[65536];
	json_object_t *o;
	config_section_t *section;
//	json_proctab_t *tab;
	FILE *fp;
	int size,bytes;

	o = json_create_object();
	if (!o) {
		sprintf(cp->errmsg, "unable to create json object: %s",strerror(errno));
		return 1;
	}
	list_reset(cp->sections);
	while((section = list_get_next(cp->sections)) != 0) {
		dprintf(dlevel,"section: %s\n", section->name);
		if (!list_count(section->items)) continue;
#if 0
		tab = config_jsontab(cp,section);
		if (!tab) return 1;
		o = json_from_tab(tab);
		if (!o) {
			sprintf(cp->errmsg, "json_from_tab");
			return 1;
		}
		free(tab);
		json_object_set_object(o,section->name,oo);
#endif
	}
	json_tostring(json_object_get_value(o), data, sizeof(data)-1, 1);

	fp = fopen(cp->filename, "w+");
	if (!fp) {
		sprintf(cp->errmsg, "config_write_json: fopen(%s): %s",cp->filename,strerror(errno));
		return 1;
	}
	size = strlen(data);
//	bytes = fwrite(data,1,size,fp);
	bytes = fprintf(fp,"%s\n",data);
	dprintf(dlevel,"bytes: %d, size: %d\n", bytes, size);
	if (bytes != size) {
		sprintf(cp->errmsg, "config_write_json: write bytes(%d) != size(%d)\n", bytes, size);
		fclose(fp);
		return 1;
	}
	fclose(fp);
	return 0;
}

int config_write(config_t *cp) {

	if (!cp) return 1;

	dprintf(dlevel,"filename: %s\n", cp->filename);
	if (!strlen(cp->filename)) {
		sprintf(cp->errmsg, "config filename is empty");
		return 1;
	}

	if (!cp->writer) {
		if (cp->file_format == CONFIG_FILE_FORMAT_INI)
			cp->writer = config_write_ini;
		else if (cp->file_format == CONFIG_FILE_FORMAT_JSON)
			cp->writer = config_write_json;
		else {
			sprintf(cp->errmsg, "config_read: invalid file format: %d\n", cp->file_format);
			return 1;
		}
		cp->writer_ctx = cp;
		dprintf(dlevel,"writer: %p, ctx: %p\n", cp->writer, cp->writer_ctx);
	}
	return cp->writer(cp->writer_ctx);
}

int config_set_filename(config_t *cp, char *filename, enum CONFIG_FILE_FORMAT format) {

	dprintf(dlevel,"cp: %p, filename: %s, format: %d\n", cp, filename, format);
	if (!cp) return 1;

	if (format >= CONFIG_FILE_FORMAT_UNKNOWN && format < CONFIG_FILE_FORMAT_MAX) {
		if (format == CONFIG_FILE_FORMAT_UNKNOWN) {
			char *p;

			/* Try to determine format */
			p = strrchr(filename,'.');
			if (p && (strcasecmp(p,".json") == 0)) format = CONFIG_FILE_FORMAT_JSON;
			else format = CONFIG_FILE_FORMAT_INI;
			dprintf(dlevel,"NEW format: %d\n", format);
		}
		strncpy(cp->filename, filename, sizeof(cp->filename)-1);
		cp->file_format = format;
		if (cp->file_format == CONFIG_FILE_FORMAT_INI) {
			cp->writer = config_write_ini;
			cp->writer_ctx = cp;
		} else if (cp->file_format == CONFIG_FILE_FORMAT_JSON) {
			cp->writer = config_write_json;
			cp->writer_ctx = cp;
		}
		return 0;
	} else {
		sprintf(cp->errmsg, "config_read: invalid file format: %d\n", cp->file_format);
		return 1;
	}
}

config_property_t *config_find_property(config_t *cp, char *name) {
	config_section_t *s;
	config_property_t *p;

	dprintf(dlevel,"name: %s\n", name);
	list_reset(cp->sections);
	while((s = list_get_next(cp->sections)) != 0) {
		list_reset(s->items);
		while((p = list_get_next(s->items)) != 0) {
			dprintf(dlevel,"p->name: %s\n", p->name);
			if (strcasecmp(p->name,name)==0) {
				dprintf(dlevel,"found\n");
				return p;
			}
		}
	}
	dprintf(dlevel,"NOT found\n");
	return 0;
}

config_property_t *config_get_propbyid(config_t *cp, int id) {
	config_section_t *s;
	config_property_t *p;

	dprintf(dlevel,"checking map...\n");
	if (cp->map && id >= 0 && id < cp->map_maxid && cp->map[id]) {
		dprintf(dlevel,"found: %s\n", cp->map[id]->name);
		return cp->map[id];
	}
	dprintf(dlevel,"NOT found, checking lists\n");

	dprintf(dlevel,"id: %d\n", id);
	list_reset(cp->sections);
	while((s = list_get_next(cp->sections)) != 0) {
		list_reset(s->items);
		while((p = list_get_next(s->items)) != 0) {
			dprintf(dlevel,"p->id: %d\n", p->id);
			if (p->id == id) {
				dprintf(dlevel,"found\n");
				return p;
			}
		}
	}
	dprintf(dlevel,"NOT found\n");
	return 0;
}

#if 0
static void json_dump(json_value_t *v) {
	char *str;

	str = json_dumps(v,1);
	printf("%s\n", str);
	free(str);
}
#endif

static json_array_t *_get_json_section(json_array_t *a, char *name) {
	json_object_t *so;
	int i,type;

	dprintf(dlevel,"name: %s\n", name);

	/* Search the configuration array for a section */
	/* MUST be an array of objects */
	for(i=0; i < a->count; i++) {
		type = json_value_get_type(a->items[i]);
		dprintf(dlevel,"type: %s\n", json_typestr(type));
		if (type != JSON_TYPE_OBJECT) return 0;
		/* section object has only 1 value and is an array */
		so = json_value_get_object(a->items[i]);
		if (strcmp(so->names[0],name) == 0) {
			if (so->count != 1 || json_value_get_type(so->values[0]) != JSON_TYPE_ARRAY) return 0;
			return json_value_get_array(so->values[0]);
		}
	}
	dprintf(dlevel,"NOT found\n");
	return 0;
}

static void _add_scope(json_object_t *o, int type, char *scope, int nvalues, void *values) {
//	int i,*ip;
//	float *fp;
	double *da;
	int i;
	json_array_t *a;

	dprintf(dlevel,"type: %s, scope: %s, nvalues: %d\n", typestr(type), scope, nvalues);
	json_object_set_string(o,"scope",scope);
	da = malloc(nvalues * sizeof(double));
	if (!da) {
		log_error("config: _add_scope: malloc(%d)\n", nvalues * sizeof(double));
		return;
	}
	conv_type(DATA_TYPE_F64_ARRAY,da,nvalues,type | DATA_TYPE_ARRAY,values,nvalues);
	a = json_create_array();
	if (strcmp(scope,"range")==0) {
		if (nvalues) json_array_add_number(a,da[0]);
		if (nvalues > 1) json_array_add_number(a,da[1]);
		if (nvalues > 2) json_array_add_number(a,da[2]);
	} else if (strcmp(scope,"select")==0 || strcmp(scope,"mask")==0) {
		for(i=0; i < nvalues; i++) json_array_add_number(a,da[i]);
	}
	json_object_set_array(o,"values",a);
	free(da);
#if 0
	switch(type) {
	case DATA_TYPE_FLOAT:
		fp = (float *) values;

		a = json_create_array();
		if (strcmp(scope,"range")==0) {
			if (nvalues) json_array_add_number(a,fp[0]);
			if (nvalues > 1) json_array_add_number(a,fp[1]);
			if (nvalues > 2) json_array_add_number(a,fp[2]);
		} else if (strcmp(scope,"select")==0 || strcmp(scope,"mask")==0) {
			for(i=0; i < nvalues; i++) json_array_add_number(a,fp[i]);
		}
		json_object_set_array(o,"values",a);
		break;
	case DATA_TYPE_S16:
		ip = (uint16_t *) values;

		a = json_create_array();
		if (strcmp(scope,"range")==0) {
			if (nvalues) json_array_add_number(a,ip[0]);
			if (nvalues > 1) json_array_add_number(a,ip[1]);
			if (nvalues > 2) json_array_add_number(a,ip[2]);
		} else if (strcmp(scope,"select")==0 || strcmp(scope,"mask")==0) {
			for(i=0; i < nvalues; i++) json_array_add_number(a,ip[i]);
		}
		json_object_set_array(o,"values",a);
		break;
	case DATA_TYPE_BOOL:
	case DATA_TYPE_INT:
		ip = (int *) values;

		a = json_create_array();
		if (strcmp(scope,"range")==0) {
			if (nvalues) json_array_add_number(a,ip[0]);
			if (nvalues > 1) json_array_add_number(a,ip[1]);
			if (nvalues > 2) json_array_add_number(a,ip[2]);
		} else if (strcmp(scope,"select")==0 || strcmp(scope,"mask")==0) {
			for(i=0; i < nvalues; i++) json_array_add_number(a,ip[i]);
		}
		json_object_set_array(o,"values",a);
		break;
	default:
		log_error("config: _add_scope: unhandled type: %d(%s)\n", type, typestr(type));
		break;
	}
#endif
}

json_object_t *prop_to_json(config_property_t *p) {
	json_object_t *to;
	int i;

	dprintf(dlevel,"name: %s, type: %d(%s), scope: %s, nvalues: %d, values: %p, nlabels: %d, labels: %p\n", p->name, p->type, typestr(p->type), p->scope, p->nvalues, p->values, p->nlabels, p->labels);

	to = json_create_object();

	json_object_set_string(to,"name",p->name);
	json_object_set_string(to,"type",typestr(p->type));
	if (p->dsize) json_object_set_number(to,"size",p->dsize);
	if (p->nvalues && p->values) _add_scope(to,p->type,p->scope,p->nvalues,p->values);
	if (p->nlabels && p->labels) {
		json_array_t *a = json_create_array();
		for(i=0; i < p->nlabels; i++) {
			dprintf(dlevel,"labels[%d]: %s\n", i, p->labels[i]);
			json_array_add_string(a,p->labels[i]);
		}
		json_object_set_array(to,"labels",a);
	}
	if (p->units) json_object_set_string(to,"units",p->units);
	if ((int)p->scale != 0) json_object_set_number(to,"scale",p->scale);
	return to;
}

static json_object_t *_get_json_prop(json_array_t *a, char *name) {
	json_object_t *o;
	char *p;
	int i;

	dprintf(dlevel,"name: %s\n", name);

	for(i=0; i < a->count; i++) {
		o = json_value_get_object(a->items[i]);
//		dprintf(dlevel+1,"o: %p\n", o);
		if (!o) continue;
//		for(j=0; j < o->count; j++) dprintf(dlevel+1,"names[%d]: %s\n", j, o->names[j]);
		p = json_object_get_string(o, name);
//		dprintf(dlevel+1,"p: %p\n", p);
		if (!p) continue;
		if (strcmp(p, name) == 0) return o;
	}
	return 0;
}

static int config_get_value(void *ctx, list args, char *errmsg) {
	dprintf(dlevel,"args count: %d\n", list_count(args));
	/* we dont actually do anything here */
	return 0;
}

static int config_set_value(void *ctx, list args, char *errmsg) {
	config_t *cp = ctx;
	char **argv;
	config_property_t *p;

	list_reset(args);
	while((argv = list_get_next(args)) != 0) {
		p = config_find_property(cp, argv[0]);
		dprintf(dlevel,"p: %p\n", p);
		if (!p) {
			sprintf(errmsg,"property %s not found",argv[0]);
			return 1;
		}
		p->len = conv_type(p->type,p->dest,p->dsize,DATA_TYPE_STRING,argv[1],strlen(argv[1]));
	}
	return 0;
}

static void _addgetset(config_t *cp) {
	int have_get, have_set;
	config_function_t newfunc,*f;

	list_reset(cp->funcs);
	have_get = have_set = 0;
	while((f = list_get_next(cp->funcs)) != 0) {
		if (strcasecmp(f->name,"get")==0) have_get = 1;
		if (strcasecmp(f->name,"set")==0) have_set = 1;
	}
	if (!have_get) {
		newfunc.name = "get";
		newfunc.func = config_get_value;
		newfunc.ctx = cp;
		newfunc.nargs = 1;
		list_add(cp->funcs,&newfunc,sizeof(newfunc));
	}
	if (!have_set) {
		newfunc.name = "set";
		newfunc.func = config_set_value;
		newfunc.ctx = cp;
		newfunc.nargs = 2;
		list_add(cp->funcs,&newfunc,sizeof(newfunc));
	}
}

int config_add_info(config_t *cp, json_object_t *o) {
	config_section_t *s;
	config_property_t *p;
	config_function_t *f;
	json_array_t *ca,*sa;
	json_object_t *so,*po;
	int addsa;

	ca = json_create_array();
	dprintf(dlevel,"ca: %p\n", ca);
	if (!ca) {
		sprintf(cp->errmsg,"unable to create json array(configuration): %s",strerror(errno));
		return 1;
	}

	list_reset(cp->sections);
	while((s = list_get_next(cp->sections)) != 0) {
		if (s->flags & (CONFIG_FLAG_NOSAVE | CONFIG_FLAG_NOID)) continue;
		/* it's an array of objects */
		sa = _get_json_section(ca,s->name);
		if (!sa) {
		        so = json_create_object();
		        if (!so) {
				sprintf(cp->errmsg,"unable to create json section object(%s): %s",s->name,strerror(errno));
				return 1;
			}
		        sa = json_create_array();
		        if (!sa) {
				sprintf(cp->errmsg,"unable to create json section array(%s): %s",s->name,strerror(errno));
				return 1;
			}
			addsa = 1;
		} else {
			addsa = 0;
		}
		list_reset(s->items);
		while((p = list_get_next(s->items)) != 0) {
			if (p->flags & (CONFIG_FLAG_NOSAVE | CONFIG_FLAG_FILEONLY)) continue;
			if (_get_json_prop(sa,p->name)) continue;
			dprintf(dlevel,"adding prop: %s\n", p->name);
			po = prop_to_json(p);
			if (!po) {
				sprintf(cp->errmsg,"unable to create json from property(%s): %s",s->name,strerror(errno));
				return 1;
			}
			json_array_add_object(sa, po);
#if 0
			if(strcmp(p->name,"AutoStr")==0) {
				json_dump(json_array_get_value(sa));
			}
#endif
		}
		if (addsa) {
			json_object_set_array(so, s->name, sa);
			json_array_add_object(ca, so);
		}
	}

	json_object_set_array(o, "configuration", ca);

	/* Add the functions */
	_addgetset(cp);
	sa = json_create_array();
	list_reset(cp->funcs);
	while((f = list_get_next(cp->funcs)) != 0) {
		so = json_create_object();
		json_object_set_string(so,"name",f->name);
		json_object_set_number(so,"nargs",f->nargs);
		json_array_add_object(sa,so);
	}
	json_object_set_array(o, "functions", sa);

	return 0;
}

int config_process(config_t *cp, char *request) {
	json_value_t *v,*vv,*vvv;
	json_object_t *o;
	json_array_t *a;
	int type,i,j,k,status;
        config_function_t *f;
	char *args[32];
	int nargs;
	list fargs;

	status = 1;
	dprintf(dlevel,"parsing json data...\n");
	v = json_parse(request);
	if (!v) {
		strcpy(cp->errmsg,"invalid json format");
		goto _process_error;
	}

	/* nargs = 1: Object with function name and array of arguments */
	/* { "func": [ "arg", "argn", ... ] } */
	/* nargs = 2: Object with function name and array of objects with key:value pairs */
	/* { "func": [ { "key": "value" }, { "keyn": "valuen" }, ... ] } */
	/* nargs > 1: Object with function name and array of argument arrays */
	/* { "func": [ [ "arg", "argn", ... ], [ "arg", "argn" ], ... ] } */
	type = json_value_get_type(v);
	dprintf(dlevel,"type: %d (%s)\n",type, json_typestr(type));
	if (type != JSON_TYPE_OBJECT) {
		strcpy(cp->errmsg,"invalid json format");
		goto _process_error;
	}
	o = json_value_get_object(v);
	dprintf(dlevel,"object count: %d\n", o->count);
	for(i=0; i < o->count; i++) {
        	f = config_function_get(cp, o->names[i]);
		if (!f) {
			sprintf(cp->errmsg,"invalid function: %s",o->names[i]);
			return 1;
		}
		fargs = list_create();

		/* Value MUST be array */
		vv = o->values[i];
		type = json_value_get_type(vv);
		dprintf(dlevel,"value[%d]: type: %d (%s)\n", i, type, json_typestr(type));
		if (type != JSON_TYPE_ARRAY) {
			strcpy(cp->errmsg,"invalid json format");
			goto _process_error;
		}

		/* Array values can be strings/objects/arrays */
		a = json_value_get_array(vv);
		dprintf(dlevel,"array count: %d\n", a->count);
		if (f->nargs != 0 && a->count == 0) {
			strcpy(cp->errmsg,"invalid request (no parameters)");
			goto _process_error;
		}
		for(j=0; j < a->count; j++) {
			vvv = a->items[j];
			type = json_value_get_type(vvv);
			if (f->nargs == 0) {
				sprintf(cp->errmsg,"function %s takes 0 arguments but %d passed\n", f->name, (int)a->count);
				goto _process_error;
			} else if (f->nargs == 1) {
				char *str;

				/* Strings only */
				if (type != JSON_TYPE_STRING) {
					strcpy(cp->errmsg,"must be array of strings for nargs = 1");
					goto _process_error;
				}
				str = json_value_get_string(vvv);
				list_add(fargs,str,strlen(str)+1);
			} else if (f->nargs == 2) {
				json_object_t *oo;

				/* Objects */
				if (type != JSON_TYPE_OBJECT) {
					strcpy(cp->errmsg,"must be array of objects for nargs = 2");
					goto _process_error;
				}
				oo = json_value_get_object(vvv);
				nargs = 0;
				for(k=0; k < oo->count; k++) {
					args[nargs++] = oo->names[k];
					args[nargs++] = json_value_get_string(oo->values[k]);
				}
				args[nargs] = 0;
				list_add(fargs,args,sizeof(args));
			} else if (f->nargs > 2) {
				json_array_t *aa;
				int k;
				char *str;

				/* Arrays */
				if (type != JSON_TYPE_ARRAY) {
					strcpy(cp->errmsg,"must be array of arrays for nargs > 2");
					goto _process_error;
				}
				aa = json_value_get_array(vvv);
				/* Strings */
				nargs = 0;
				for(k=0; k < aa->count; k++) {
					if (json_value_get_type(aa->items[k]) != JSON_TYPE_STRING) {
						strcpy(cp->errmsg,"must be array of string arrays for nargs > 2");
						goto _process_error;
					}
					str = json_value_get_string(aa->items[k]);
					args[nargs++] = str;
				}
				args[nargs] = 0;
				list_add(fargs,args,sizeof(args));
			}
		}
		dprintf(dlevel,"fargs count: %d\n", list_count(fargs));
		if (f->func(f->ctx, fargs, cp->errmsg)) goto _process_error;
	}

	status = 0;
	strcpy(cp->errmsg,"success");

_process_error:
	json_destroy_value(v);
	return status;
}

#ifdef JS
JSPropertySpec *config_to_props(config_t *cp, char *name, JSPropertySpec *add) {
	JSPropertySpec *props,*pp,*app;
	config_section_t *section;
	config_property_t *p;
	int count;

	section = config_get_section(cp,name);
	if (!section) {
		sprintf(cp->errmsg,"unable to find config section: %s\n", name);
		return 0;
	}

	/* Get total count */
	count = list_count(section->items);
	if (add) for(app = add; app->name; app++) count++;
	/* Add 1 for termination */
	count++;
	dprintf(dlevel,"count: %d\n", count);
	props = calloc(count * sizeof(JSPropertySpec),1);
	if (!props) {
		sprintf(cp->errmsg,"calloc props(%d): %s\n", count, strerror(errno));
		return 0;
	}

	dprintf(dlevel,"adding config...\n");
	pp = props;
	list_reset(section->items);
	while((p = list_get_next(section->items)) != 0) {
		if (p->flags & CONFIG_FLAG_FILEONLY) continue;
		pp->name = p->name;
		pp->tinyid = p->id;
		pp->flags = JSPROP_ENUMERATE;
		if (p->flags & CONFIG_FLAG_READONLY) pp->flags |= JSPROP_READONLY;
		pp++;
	}

	/* Now add the addl props, if any */
	if (add) for(app = add; app->name; app++) *pp++ = *app;

	dprintf(dlevel+1,"dump:\n");
	for(pp = props; pp->name; pp++) 
		dprintf(dlevel+1,"prop: name: %s, id: %d, flags: %02x\n", pp->name, pp->tinyid, pp->flags);

	config_build_propmap(cp);

	return props;
}

#if 0
static JSBool jscallfunc(JSContext *cx, uintN argc, jsval *vp) {
	si_session_t *s;
	uint8_t data[8];
	int id,len,r;
	jsval *argv = vp + 2;
	JSObject *obj;
	JSClass *classp;
	char *args[32];
	int nargs;

	/* XXX no way to get func name and no way to get ctx */
	obj = JS_THIS_OBJECT(cx, vp);
	if (!obj) return JS_FALSE;

	/* Turn the args into array of char string ptrs */
	for(i=0; i < argc; i++) {
		str = JS_ValueToString(cx, argv[0]);
		if (!str) return JS_FALSE;
		bytes = JS_EncodeString(cx, str);
		if (!bytes) return JS_FALSE;
		args[i] = bytes;
	}
	for(i=0; i < argc; i++) JS_free(cx, args[i]);

}

JSFunctionSpec *config_to_funcs(config_t *cp, JSFunctionSpec *add) {
	JSFunctionSpec *funcs,*fp,*afp;
	config_function_t *f;
	int i,count;

	/* Get total count */
	count = list_count(cp->funcs);
	if (add) for(afp = add; afp->name; afp++) count++;
	/* Add 1 for termination */
	count++;
	printf("==> count: %d\n", count);
	funcs = calloc(count * sizeof(JSFunctionSpec),1);
	if (!funcs) {
		sprintf(cp->errmsg,"calloc funcs(%d): %s\n", count, strerror(errno));
		return 0;
	}

	dprintf(dlevel,"adding config...\n");
	fp = funcs;
	list_reset(cp->funcs);
	while((f = list_get_next(cp->funcs)) != 0) {
#if 0
#define JS_FN(name,fastcall,minargs,nargs,flags)                              \
    {name, (JSNative)(fastcall), nargs,                                       \
     (flags) | JSFUN_FAST_NATIVE | JSFUN_STUB_GSOPS,                          \
     (minargs) << 16}
    const char      *name;
    JSNative        call;
    uint8           nargs;
    uint8           flags;
    uint16          extra;
#endif
		/* Allocate mem for the func */
		fp->name = f->name;
		fp->call = jscallfunc;
		fp->nargs = f->nargs;
		fp->flags = JSFUN_FAST_NATIVE | JSFUN_STUB_GSOPS,
		fp->extra = 0;
		pp++;
	}

	/* Now add the addl funcs, if any */
	if (add) for(app = add; app->name; app++) *pp++ = *app;

	dprintf(dlevel+1,"dump:\n");
	for(fp = funcs; fp->name; fp++) 
		dprintf(dlevel+1,"func: name: %s, nargs: %d\n", fp->name, fp->nargs);

	return funcs;
}
#endif

JSBool config_jsgetprop(JSContext *cx, JSObject *obj, jsval id, jsval *rval, config_t *cp, JSPropertySpec *props) {
	int prop_id;
	config_property_t *p;

	if(JSVAL_IS_INT(id)) {
		prop_id = JSVAL_TO_INT(id);
		dprintf(dlevel,"prop_id: %d\n", prop_id);
		p = CONFIG_GETMAP(cp,prop_id);
		if (!p) p = config_get_propbyid(cp,prop_id);
		if (!p) {
			JS_ReportError(cx, "property not found");
			return JS_FALSE;
		}
		*rval = type_to_jsval(cx,p->type,p->dest,p->len);
	}
	return JS_TRUE;
}

JSBool config_jssetprop(JSContext *cx, JSObject *obj, jsval id, jsval *vp, config_t *cp, JSPropertySpec *props) {
	int prop_id;
	config_property_t *p;

	if(JSVAL_IS_INT(id)) {
		prop_id = JSVAL_TO_INT(id);
		dprintf(dlevel,"prop_id: %d\n", prop_id);
		p = CONFIG_GETMAP(cp,prop_id);
		if (!p) p = config_get_propbyid(cp,prop_id);
		if (!p) {
			JS_ReportError(cx, "property not found");
			return JS_FALSE;
		}
		jsval_to_type(p->type,p->dest,p->dsize,cx,*vp);
	}
	return JS_TRUE;
}

enum CONFIG_PROPERTY_PROPERTY_ID {
	CONFIG_PROPERTY_PROPERTY_ID_NAME,
	CONFIG_PROPERTY_PROPERTY_ID_DATA,
	CONFIG_PROPERTY_PROPERTY_ID_TYPE,
	CONFIG_PROPERTY_PROPERTY_ID_SIZE,
	CONFIG_PROPERTY_PROPERTY_ID_FLAGS,
	CONFIG_PROPERTY_PROPERTY_ID_SCOPE,
	CONFIG_PROPERTY_PROPERTY_ID_VALUES,
	CONFIG_PROPERTY_PROPERTY_ID_LABELS,
	CONFIG_PROPERTY_PROPERTY_ID_UNITS,
	CONFIG_PROPERTY_PROPERTY_ID_SCALE,
	CONFIG_PROPERTY_PROPERTY_ID_PRECISION,
};

static JSBool config_property_getprop(JSContext *cx, JSObject *obj, jsval id, jsval *rval) {
	config_property_t *p;
	int prop_id;

	p = JS_GetPrivate(cx,obj);
	dprintf(6,"prop: name: %s, id: %d, type: %d(%s)\n", p->name, p->id, p->type, typestr(p->type));
	dprintf(dlevel,"is_int: %d\n", JSVAL_IS_INT(id));
	if(JSVAL_IS_INT(id)) {
		prop_id = JSVAL_TO_INT(id);
		dprintf(dlevel,"prop_id: %d\n", prop_id);
		switch(prop_id) {
		case CONFIG_PROPERTY_PROPERTY_ID_NAME:
			*rval = STRING_TO_JSVAL(JS_NewStringCopyZ(cx,p->name));
			break;
		case CONFIG_PROPERTY_PROPERTY_ID_DATA:
			*rval = type_to_jsval(cx, p->type, p->dest, p->len);
			break;
		case CONFIG_PROPERTY_PROPERTY_ID_TYPE:
			*rval = STRING_TO_JSVAL(JS_NewStringCopyZ(cx,typestr(p->type)));
			break;
		case CONFIG_PROPERTY_PROPERTY_ID_SIZE:
			*rval = INT_TO_JSVAL(p->dsize);
			break;
		case CONFIG_PROPERTY_PROPERTY_ID_FLAGS:
			*rval = INT_TO_JSVAL(p->flags);
			break;
		case CONFIG_PROPERTY_PROPERTY_ID_SCOPE:
			*rval = STRING_TO_JSVAL(JS_NewStringCopyZ(cx,p->scope));
			break;
		case CONFIG_PROPERTY_PROPERTY_ID_VALUES:
			*rval = type_to_jsval(cx, p->type | DATA_TYPE_ARRAY, p->values, p->nvalues);
			break;
		case CONFIG_PROPERTY_PROPERTY_ID_LABELS:
			*rval = type_to_jsval(cx, DATA_TYPE_STRING_ARRAY, p->labels, p->nlabels);
			break;
		case CONFIG_PROPERTY_PROPERTY_ID_UNITS:
			*rval = STRING_TO_JSVAL(JS_NewStringCopyZ(cx,p->units));
			break;
		case CONFIG_PROPERTY_PROPERTY_ID_SCALE:
			JS_NewDoubleValue(cx, p->scale, rval);
			break;
		case CONFIG_PROPERTY_PROPERTY_ID_PRECISION:
			*rval = INT_TO_JSVAL(p->precision);
			break;
		default:
			JS_ReportError(cx, "property not found");
			return JS_FALSE;
		}
	}
	return JS_TRUE;
}

#if 0
static JSBool config_property_setprop(JSContext *cx, JSObject *obj, jsval id, jsval *rval) {
	config_property_t *p;
	int prop_id;

	p = JS_GetPrivate(cx,obj);
	dprintf(dlevel,"is_int: %dn", JSVAL_IS_INT(id));
	if(JSVAL_IS_INT(id)) {
		prop_id = JSVAL_TO_INT(id);
		dprintf(dlevel,"prop_id: %dn", prop_id);
		switch(prop_id) {
		case CONFIG_PROPERTY_PROPERTY_ID_NAME:
			jsval_to_type(DATA_TYPE_STRING,&s->name,sizeof(s->name)-1,cx,*vp);
			break;
		case CONFIG_PROPERTY_PROPERTY_ID_TYPE:
			jsval_to_type(DATA_TYPE_S32,&s->type,0,cx,*vp);
			break;
		case CONFIG_PROPERTY_PROPERTY_ID_DATA:
			p->len = jsval_to_type(p->type,p->data,p->dsize,cx,*vp);
			break;
		case CONFIG_PROPERTY_PROPERTY_ID_DSIZE:
			jsval_to_type(DATA_TYPE_S32,&s->dsize,0,cx,*vp);
			break;
		case CONFIG_PROPERTY_PROPERTY_ID_FLAGS:
			jsval_to_type(DATA_TYPE_U16,&s->flags,0,cx,*vp);
			break;
		case CONFIG_PROPERTY_PROPERTY_ID_SCOPE:
			jsval_to_type(DATA_TYPE_STRING,&s->scope,sizeof(s->scope)-1,cx,*vp);
			break;
		case CONFIG_PROPERTY_PROPERTY_ID_VALUES:
			p->nvalues = jsval_to_type(p->type | DATA_TYPE_ARRAY,p->values,p->dsize,*vp);
			break;
		case CONFIG_PROPERTY_PROPERTY_ID_LABELS:
			p->nlabels = jsval_to_type(p->type | DATA_TYPE_ARRAY,p->labels,p->dsize,*vp);
			break;
#if 0
		case CONFIG_PROPERTY_PROPERTY_ID_NVALUES:
			jsval_to_type(DATA_TYPE_S32,&s->nvalues,0,cx,*vp);
			break;
		case CONFIG_PROPERTY_PROPERTY_ID_NLABELS:
			jsval_to_type(DATA_TYPE_S32,&s->nlabels,0,cx,*vp);
			break;
#endif
		case CONFIG_PROPERTY_PROPERTY_ID_LABELS:
			jsval_to_type(DATA_TYPE_STRING,&s->labels,sizeof(s->labels)-1,cx,*vp);
			break;
		case CONFIG_PROPERTY_PROPERTY_ID_UNITS:
			jsval_to_type(DATA_TYPE_STRING,&s->units,sizeof(s->units)-1,cx,*vp);
			break;
		case CONFIG_PROPERTY_PROPERTY_ID_SCALE:
			jsval_to_type(DATA_TYPE_FLOAT,&s->scale,0,cx,*vp);
			break;
		case CONFIG_PROPERTY_PROPERTY_ID_PRECISION:
			jsval_to_type(DATA_TYPE_S32,&s->precision,0,cx,*vp);
			break;
		default:
			JS_ReportError(cx, "property not found");
			return JS_FALSE;
		}
	}
	return JS_TRUE;
}
#endif

static JSClass config_property_class = {
	"config_property",	/* Name */
	JSCLASS_HAS_PRIVATE,	/* Flags */
	JS_PropertyStub,	/* addProperty */
	JS_PropertyStub,	/* delProperty */
	config_property_getprop,	/* getProperty */
	JS_PropertyStub,	/* setProperty */
	JS_EnumerateStub,	/* enumerate */
	JS_ResolveStub,		/* resolve */
	JS_ConvertStub,		/* convert */
	JS_FinalizeStub,	/* finalize */
	JSCLASS_NO_OPTIONAL_MEMBERS
};

JSObject *JSConfigProperty(JSContext *cx, void *priv) {
	JSPropertySpec props[] = {
		{ "name",CONFIG_PROPERTY_PROPERTY_ID_NAME,JSPROP_ENUMERATE | JSPROP_READONLY },
		{ "data",CONFIG_PROPERTY_PROPERTY_ID_DATA,JSPROP_ENUMERATE | JSPROP_READONLY },
		{ "type",CONFIG_PROPERTY_PROPERTY_ID_TYPE,JSPROP_ENUMERATE | JSPROP_READONLY },
		{ "size",CONFIG_PROPERTY_PROPERTY_ID_SIZE,JSPROP_ENUMERATE | JSPROP_READONLY },
		{ "flags",CONFIG_PROPERTY_PROPERTY_ID_FLAGS,JSPROP_ENUMERATE | JSPROP_READONLY },
		{ "scope",CONFIG_PROPERTY_PROPERTY_ID_SCOPE,JSPROP_ENUMERATE | JSPROP_READONLY },
		{ "values",CONFIG_PROPERTY_PROPERTY_ID_VALUES,JSPROP_ENUMERATE | JSPROP_READONLY },
		{ "labels",CONFIG_PROPERTY_PROPERTY_ID_LABELS,JSPROP_ENUMERATE | JSPROP_READONLY },
		{ "units",CONFIG_PROPERTY_PROPERTY_ID_UNITS,JSPROP_ENUMERATE | JSPROP_READONLY },
		{ "scale",CONFIG_PROPERTY_PROPERTY_ID_SCALE,JSPROP_ENUMERATE | JSPROP_READONLY },
		{ "precision",CONFIG_PROPERTY_PROPERTY_ID_PRECISION,JSPROP_ENUMERATE | JSPROP_READONLY },
		{ 0 }
	};
	JSObject *obj;

	dprintf(5,"defining %s object\n",config_property_class.name);
	obj = JS_InitClass(cx, JS_GetGlobalObject(cx), 0, &config_property_class, 0, 0, props, 0, 0, 0);
	if (!obj) {
		JS_ReportError(cx,"unable to initialize %s class", config_property_class.name);
		return 0;
	}
	JS_SetPrivate(cx,obj,priv);
	dprintf(5,"done!\n");
	return obj;
}

enum CONFIG_PROPERTY_ID {
	CONFIG_PROPERTY_ID_SECTIONS,
	CONFIG_PROPERTY_ID_FILENAME,
	CONFIG_PROPERTY_ID_FILE_FORMAT,
	CONFIG_PROPERTY_ID_ERRMSG
};

static JSBool config_lgetprop(JSContext *cx, JSObject *obj, jsval id, jsval *rval) {
	config_t *cp;
	int prop_id;

	cp = JS_GetPrivate(cx,obj);
	if (!cp) return JS_FALSE;
	if(JSVAL_IS_INT(id)) {
		prop_id = JSVAL_TO_INT(id);
		dprintf(dlevel,"prop_id: %d\n", prop_id);
		switch(prop_id) {
		case CONFIG_PROPERTY_ID_SECTIONS:
			{
				config_section_t *s;
				config_property_t *p;
				JSObject *rows;
				jsval node;
				int i;

				rows = JS_NewArrayObject(cx, list_count(cp->sections), NULL);
				i = 0;
				list_reset(cp->sections);
				while( (s = list_next(cp->sections)) != 0) {
					if (s->flags & CONFIG_FLAG_NOID) continue;
					list_reset(s->items);
					while( (p = list_next(s->items)) != 0) {
						if (p->flags & CONFIG_FLAG_NOID) continue;
						node = OBJECT_TO_JSVAL(JSConfigProperty(cx,p));
						JS_SetElement(cx, rows, i++, &node);
					}
				}
				*rval = OBJECT_TO_JSVAL(rows);
			}
                        break;
		case CONFIG_PROPERTY_ID_FILENAME:
			*rval = STRING_TO_JSVAL(JS_NewStringCopyZ(cx,cp->filename));
			break;
		case CONFIG_PROPERTY_ID_FILE_FORMAT:
			*rval = INT_TO_JSVAL(cp->file_format);
			break;
		case CONFIG_PROPERTY_ID_ERRMSG:
			*rval = STRING_TO_JSVAL(JS_NewStringCopyZ(cx,cp->errmsg));
			break;
		default:
			JS_ReportError(cx, "property not found");
			return JS_FALSE;
		}
	}
	return JS_TRUE;
}

static JSBool config_lsetprop(JSContext *cx, JSObject *obj, jsval id, jsval *rval) {
	config_t *cp;
	int prop_id;

	cp = JS_GetPrivate(cx,obj);
	if (!cp) return JS_FALSE;
	dprintf(dlevel,"is_int: %d\n", JSVAL_IS_INT(id));
	if(JSVAL_IS_INT(id)) {
		prop_id = JSVAL_TO_INT(id);
		dprintf(dlevel,"prop_id: %d\n", prop_id);
		switch(prop_id) {
		case CONFIG_PROPERTY_ID_FILENAME:
			*rval = STRING_TO_JSVAL(JS_NewStringCopyZ(cx,cp->filename));
			break;
		case CONFIG_PROPERTY_ID_FILE_FORMAT:
			*rval = INT_TO_JSVAL(cp->file_format);
			break;
		default:
			JS_ReportError(cx, "property not found");
			return JS_FALSE;
		}
	}
	return JS_TRUE;
}

static JSClass config_class = {
	"config",		/* Name */
	JSCLASS_HAS_PRIVATE,	/* Flags */
	JS_PropertyStub,	/* addProperty */
	JS_PropertyStub,	/* delProperty */
	config_lgetprop,		/* getProperty */
	config_lsetprop,		/* setProperty */
	JS_EnumerateStub,	/* enumerate */
	JS_ResolveStub,		/* resolve */
	JS_ConvertStub,		/* convert */
	JS_FinalizeStub,	/* finalize */
	JSCLASS_NO_OPTIONAL_MEMBERS
};

static JSBool jsconfig_read(JSContext *cx, uintN argc, jsval *vp) {
	JSObject *obj;
	config_t *cp;
	jsval *argv = vp + 2;
	char *filename;
	int type;

	obj = JS_THIS_OBJECT(cx, vp);
	if (!obj) return JS_FALSE;
	cp = JS_GetPrivate(cx, obj);

	if (!JS_ConvertArguments(cx, argc, argv, "s / i", &filename, &type)) return JS_FALSE;

#if 0
	if (argc != 2) {
		JS_ReportError(cx,"config_read requires 2 arguments (filename: string[, format: string)\n");
		return JS_FALSE;
	}

	printf("isobj: %d\n", JSVAL_IS_OBJECT(argv[1]));
	if (!JSVAL_IS_OBJECT(argv[1])) {
		JS_ReportError(cx, "can_write: 2nd argument must be array\n");
		return JS_FALSE;
	}
	obj = JSVAL_TO_OBJECT(argv[1]);
	classp = OBJ_GET_CLASS(cx,obj);
	if (!classp) {
		JS_ReportError(cx, "can_write: 2nd argument must be array\n");
		return JS_FALSE;
	}
	printf("class: %s\n", classp->name);
	if (classp && strcmp(classp->name,"Array")) {
		JS_ReportError(cx, "can_write: 2nd argument must be array\n");
		return JS_FALSE;
	}
#endif
	config_read(cp, filename, type);

	return JS_FALSE;
}

static JSBool jsconfig_write(JSContext *cx, uintN argc, jsval *vp) {
	return JS_FALSE;
}

JSObject *JSConfig(JSContext *cx, void *priv) {
	JSPropertySpec config_props[] = {
		{ "sections", CONFIG_PROPERTY_ID_SECTIONS,  JSPROP_ENUMERATE | JSPROP_READONLY },
		{ "filename", CONFIG_PROPERTY_ID_FILENAME,  JSPROP_ENUMERATE },
		{ "file_format", CONFIG_PROPERTY_ID_FILE_FORMAT,  JSPROP_ENUMERATE },
		{ "errmsg", CONFIG_PROPERTY_ID_ERRMSG,  JSPROP_ENUMERATE | JSPROP_READONLY },
		{ 0 }
	};
	JSFunctionSpec config_funcs[] = {
		JS_FN("read",jsconfig_read,1,1,0),
		JS_FN("write",jsconfig_write,0,0,0),
		{ 0 }
	};
	JSConstantSpec config_consts[] = {
		JS_NUMCONST(CONFIG_FLAG_READONLY),
		JS_NUMCONST(CONFIG_FLAG_NOSAVE),
		JS_NUMCONST(CONFIG_FLAG_NOID),
		JS_NUMCONST(CONFIG_FLAG_FILEONLY),
		JS_NUMCONST(CONFIG_FLAG_VOLATILE),
		JS_NUMCONST(CONFIG_FILE_FORMAT_INI),
		JS_NUMCONST(CONFIG_FILE_FORMAT_JSON),
		JS_NUMCONST(CONFIG_FILE_FORMAT_CUSTOM),
		{ 0 }
	};
	JSObject *obj;
//	config_t *cp = priv;

	dprintf(5,"defining %s object\n",config_class.name);
	obj = JS_InitClass(cx, JS_GetGlobalObject(cx), 0, &config_class, 0, 0, config_props, config_funcs, 0, 0);
	if (!obj) {
		JS_ReportError(cx,"unable to initialize %s class", config_class.name);
		return 0;
	}
	/* Define global consts */
	if (!JS_DefineConstants(cx, JS_GetGlobalObject(cx), config_consts)) {
		JS_ReportError(cx,"unable to define constants for class: %s", config_class.name);
		return 0;
	}
	JS_SetPrivate(cx,obj,priv);
	dprintf(5,"done!\n");
	return obj;
}

int config_jsinit(JSEngine *e, config_t *cp) {
	return JS_EngineAddInitFunc(e, "config", JSConfig, cp);
}
#endif

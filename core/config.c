
#define DEBUG_CONFIG 1
#define dlevel 4

#ifdef DEBUG
#undef DEBUG
#endif
#if DEBUG_CONFIG
#define DEBUG 1
#endif
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

	dprintf(1,"name: %s\n", name);

	list_reset(cp->funcs);
	while((f = list_get_next(cp->funcs)) != 0) {
		if (strcasecmp(f->name,name) == 0) {
			dprintf(1,"found\n");
			return f;
		}
	}
	dprintf(1,"NOT found\n");
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

	dprintf(1,"section: %s, name: %s\n", s->name, name);

	list_reset(s->items);
	while((p = list_get_next(s->items)) != 0) {
		dprintf(7,"p->name: %s\n", p->name);
		if (strcasecmp(p->name,name)==0) {
			dprintf(1,"found\n");
			return p;
		}
	}
	dprintf(1,"NOT found\n");
	return 0;
}

config_property_t * config_section_add_property(config_t *cp, config_section_t *s, config_property_t *p, int flags) {
	config_property_t *pp,*rp;

	dprintf(1,"prop: name: %s, id: %d, flags: %02x, def: %p\n",p->name, p->id, p->flags, p->def);

	pp = config_section_get_property(s, p->name);
	dprintf(1,"p: %p, pp: %p\n", p, pp);
	/* Item alread added (via dup?) */
	if (pp == p) return 0;
	if (pp) {
		if (pp->flags & CONFIG_FLAG_FILEONLY) {
			free(pp->name);
			free(pp->dest);
		}
		list_delete(s->items,pp);
	}
	if (!solard_check_bit(flags,CONFIG_FLAG_NOID)) p->id = (p->flags & CONFIG_FLAG_NOID ? -1 : cp->id++);
	if (p->def && p->dest) conv_type(p->type, p->dest, p->len, DATA_TYPE_STRING, p->def, strlen(p->def));
	dprintf(1,"adding: %s\n", p->name);
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

	dprintf(1,"name: %s, flags: %02x\n", name, flags);

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
//	config_property_t *pp;
	config_section_t *section;

	if (!cp || !props) return;

	section = config_get_section(cp,section_name);
	if (!section) section = config_create_section(cp,section_name,flags);
	if (!section) return;
	for(p = props; p->name; p++) {
		config_section_add_property(cp, section, p, flags);
#if 0
		dprintf(1,"prop: name: %s, id: %d, flags: %02x\n",p->name, p->id, p->flags);
		if (!solard_check_bit(flags,CONFIG_FLAG_NOID)) p->id = (p->flags & CONFIG_FLAG_NOID ? -1 : cp->id++);
		dprintf(1,"p->name: %s\n", p->name);
		/* If default for prop specified, set it */
		if (p->def && p->dest) conv_type(p->type, p->dest, p->len, DATA_TYPE_STRING, p->def, strlen(p->def));
		pp = config_section_get_property(section, p->name);
		if (pp) {
			if (pp->flags & CONFIG_FLAG_FILEONLY) {
				free(pp->name);
				free(pp->dest);
			}
			list_delete(section->items,pp);
		}
		list_add(section->items, p, sizeof(*p));
#endif
	}
#if  0
	config_propdir_t *pd;
	for(pd = propdir; pd->name; pd++) {
		dprintf(1,"pd->name: %s\n", pd->name);
	}
#endif
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

	dprintf(1,"dumping...\n");

	list_reset(cp->sections);
	while((section = list_get_next(cp->sections)) != 0) {
		dprintf(1,"section: %s\n", section->name);
		list_reset(section->items);
		while((p = list_get_next(section->items)) != 0) {
			if (p->dest) conv_type(DATA_TYPE_STRING,value,sizeof(value)-1,p->type, p->dest, p->len);
			else *value = 0;
			dprintf(1,"  prop: name: %s, id: %d, type: %d(%s), value: %s\n", p->name, p->id, p->type, typestr(p->type), value);
		}
	}
}

config_property_t *config_get_property(config_section_t *section, char *name) {
	config_property_t *p;

	dprintf(1,"name: %s\n", name);

	list_reset(section->items);
	while((p = list_get_next(section->items)) != 0) {
		if (strcasecmp(p->name,name) == 0) {
			dprintf(1,"found\n");
			return p;
		}
	}
	dprintf(1,"NOT found\n");
	return 0;
}

static json_proctab_t *config_jsontab(config_t *cp, config_section_t *s) {
	json_proctab_t *tab,*tp;
	config_property_t *p;
	config_section_t *cs;
	list ds;
	int count,size;

	dprintf(1,"s->name: %s\n", s->name);

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
	dprintf(1,"count: %d\n", count);

	size = count * sizeof(*tab);
	dprintf(1,"size: %d\n", size);
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

	for(tp = tab; tp->field; tp++) dprintf(1,"tp: field: %s, ptr: %p, type: %d, cb: %p\n", tp->field, tp->ptr, tp->type, tp->cb);

	return tab;
}

int config_parse_json(config_t *cp, json_value_t *v) {
	json_object_t *o;
	config_section_t *s;
	json_proctab_t *tab;
	int i;

	dprintf(1,"type: %d (%s)\n", v->type, json_typestr(v->type));
	if (v->type != JSONObject) {
		strcpy(cp->errmsg,"invalid json format");
		return 1;
	}
	o = json_object(v);
	if (!o) {
		strcpy(cp->errmsg,"invalid json format");
		return 1;
	}
	dprintf(1,"object count: %d\n", o->count);
	for(i=0; i < o->count; i++) {
//		sname = o->names[i];
		dprintf(1,"section: %s\n", o->names[i]);
		s = config_get_section(cp, o->names[i]);
		if (!s) continue;
		tab = config_jsontab(cp, s);
		json_to_tab(tab, o->values[i]);
		free(tab);
	}

	return 0;
}

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
	dprintf(1,"count: %d\n", count);
	size = count * sizeof(*tab);
	dprintf(1,"size: %d\n", size);
	tab = calloc(size,1);
	dprintf(1,"tab: %p\n", tab);
	if (!tab) {
		sprintf(cp->errmsg,"config_cfgtab: malloc(%d): %s\n", size, strerror(errno));
		return 0;
	}

	tp = tab;
//	dprintf(1,"section count: %d\n", list_count(cp->sections));
	list_reset(cp->sections);
	while((s = list_get_next(cp->sections)) != 0) {
//		dprintf(1,"s->name: %s\n", s->name);
		list_reset(s->items);
		while((p = list_get_next(s->items)) != 0) {
//			dprintf(1,"prop: name: %s, type: %d(%s), flags: %02x\n", p->name, p->type, typestr(p->type), p->flags);
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

	for(tp = tab; tp->keyword; tp++) dprintf(1,"tab: section: %s, keyword: %s, type: %d, dest: %p, dlen: %d\n", 
		tp->section, tp->keyword, tp->type, tp->dest, tp->dlen);
	return tab;
}

int config_read_ini(config_t *cp, char *filename) {
	cfg_info_t *cfg;
	config_section_t *sec;
	cfg_section_t *cfgsec;
	cfg_item_t *item;
	config_property_t *p,newprop;

	dprintf(1,"filename: %s\n", filename);
	cfg = cfg_read(filename);
	dprintf(1,"cfg: %p\n", cfg);
	if (!cfg) {
		sprintf(cp->errmsg, "unable to read ini file");
		return 1;
	}
	list_reset(cfg->sections);
	while((cfgsec = list_get_next(cfg->sections)) != 0) {
		dprintf(1,"cfgsec->name: %s\n", cfgsec->name);
		sec = config_get_section(cp, cfgsec->name);
		if (!sec) sec = config_create_section(cp, cfgsec->name,CONFIG_FLAG_FILEONLY);
		list_reset(cfgsec->items);
		while((item = list_get_next(cfgsec->items)) != 0) {
			dprintf(1,"item->keyword: %s\n", item->keyword);
			p = config_section_get_property(sec,item->keyword);
			if (p) {
				conv_type(p->type,p->dest,p->len,DATA_TYPE_STRING,item->value,strlen(item->value));
			} else {
				memset(&newprop,0,sizeof(newprop));
				newprop.name = malloc(strlen(item->keyword)+1);
				strncpy(newprop.name,item->keyword,strlen(item->keyword)+1);
				newprop.type = DATA_TYPE_STRING;
				newprop.dest = malloc(strlen(item->value)+1);
				strcpy(newprop.dest,item->value);
				newprop.len = strlen(item->value)+1;
				newprop.flags |= CONFIG_FLAG_FILEONLY;
				dprintf(1,"adding...\n");
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

	dprintf(1,"filename: %s\n", filename);
	cp->cfg = cfg_read(filename);
	dprintf(1,"cfg: %p\n", cp->cfg);
	if (!cp->cfg) {
		sprintf(cp->errmsg, "unable to read ini file");
		return 1;
	}
	tab = config_cfgtab(cp);
	dprintf(1,"tab: %p\n", tab);
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
	dprintf(1,"filename: %s\n", filename);
	fd = open(filename,O_RDONLY);
	dprintf(1,"fd: %d\n", fd);
	if (fd < 0) return 0;

	/* Get file size */
	size = (fstat(fd, &sb) < 0 ?  size = 1048576 : sb.st_size);
	dprintf(1,"size: %d\n", size);

	/* Alloc buffer to hold it */
	buf = malloc(size);
	if (!buf) {
		sprintf(cp->errmsg, "config_read_json: malloc(%d): %s\n", size, strerror(errno));
		close(fd);
		return 1;
	}

	/* Read entire file into buffer */
	bytes = read(fd, buf, size);
	dprintf(1,"bytes: %d\n", bytes);
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
		json_destroy(v);
		return 1;
	}

	return 0;
}

int config_read(config_t *cp, char *filename, enum CONFIG_FILE_FORMAT format) {

	dprintf(1,"filename: %s\n", filename);
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
		dprintf(1,"s->flags: %02x\n", s->flags);
		if (s->flags & CONFIG_FLAG_NOSAVE) continue;
		list_reset(s->items);
		while((p = list_get_next(s->items)) != 0) {
			dprintf(1,"p->flags: %02x\n", p->flags);
			if (p->flags & CONFIG_FLAG_NOSAVE) continue;
			conv_type(DATA_TYPE_STRING,value,sizeof(value)-1,p->type,p->dest,p->len);
			if (!strlen(value)) continue;
			dprintf(1,"section: %s, item: %s, value(%d): %s\n", s->name, p->name, strlen(value), value);
			cfg_set_item(cfg, s->name, p->name, 0, value);
		}
	}
	if (cfg_write(cfg)) {
		sprintf(cp->errmsg, "unable to write ini file: %s",strerror(errno));
		return 1;
	}
	return 0;
}

int config_write_json(config_t *cp) {
	char data[65536];
	json_value_t *v,*o;
	config_section_t *section;
	json_proctab_t *tab;
	FILE *fp;
	int size,bytes;

	v = json_create_object();
	if (!v) {
		sprintf(cp->errmsg, "unable to create json object: %s",strerror(errno));
		return 1;
	}
	list_reset(cp->sections);
	while((section = list_get_next(cp->sections)) != 0) {
		dprintf(1,"section: %s\n", section->name);
		if (!list_count(section->items)) continue;
		tab = config_jsontab(cp,section);
		if (!tab) return 1;
		o = json_from_tab(tab);
		if (!o) {
			sprintf(cp->errmsg, "json_from_tab");
			return 1;
		}
		free(tab);
		json_add_object(v,section->name,o);
	}
	json_tostring(v, data, sizeof(data)-1, 1);

	fp = fopen(cp->filename, "w+");
	if (!fp) {
		sprintf(cp->errmsg, "config_write_json: fopen(%s): %s",cp->filename,strerror(errno));
		return 1;
	}
	size = strlen(data);
//	bytes = fwrite(data,1,size,fp);
	bytes = fprintf(fp,"%s\n",data);
	dprintf(1,"bytes: %d, size: %d\n", bytes, size);
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

	dprintf(1,"filename: %s\n", cp->filename);
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
		dprintf(1,"writer: %p, ctx: %p\n", cp->writer, cp->writer_ctx);
	}
	return cp->writer(cp->writer_ctx);
}

int config_set_filename(config_t *cp, char *filename, enum CONFIG_FILE_FORMAT format) {

	dprintf(1,"cp: %p, filename: %s, format: %d\n", cp, filename, format);
	if (!cp) return 1;

	if (format >= CONFIG_FILE_FORMAT_UNKNOWN && format < CONFIG_FILE_FORMAT_MAX) {
		if (format == CONFIG_FILE_FORMAT_UNKNOWN) {
			char *p;

			/* Try to determine format */
			p = strrchr(filename,'.');
			if (p && (strcasecmp(p,".json") == 0)) format = CONFIG_FILE_FORMAT_JSON;
			else format = CONFIG_FILE_FORMAT_INI;
			dprintf(1,"NEW format: %d\n", format);
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

	dprintf(1,"name: %s\n", name);
	list_reset(cp->sections);
	while((s = list_get_next(cp->sections)) != 0) {
		list_reset(s->items);
		while((p = list_get_next(s->items)) != 0) {
			dprintf(1,"p->name: %s\n", p->name);
			if (strcasecmp(p->name,name)==0) {
				dprintf(1,"found\n");
				return p;
			}
		}
	}
	dprintf(1,"NOT found\n");
	return 0;
}

config_property_t *config_get_propbyid(config_t *cp, int id) {
	config_section_t *s;
	config_property_t *p;

	dprintf(1,"checking map...\n");
	if (cp->map && id >= 0 && id < cp->map_maxid && cp->map[id]) {
		dprintf(1,"found: %s\n", cp->map[id]->name);
		return cp->map[id];
	}
	dprintf(1,"NOT found, checking lists\n");

	dprintf(1,"id: %d\n", id);
	list_reset(cp->sections);
	while((s = list_get_next(cp->sections)) != 0) {
		list_reset(s->items);
		while((p = list_get_next(s->items)) != 0) {
			dprintf(1,"p->id: %d\n", p->id);
			if (p->id == id) {
				dprintf(1,"found\n");
				return p;
			}
		}
	}
	dprintf(1,"NOT found\n");
	return 0;
}

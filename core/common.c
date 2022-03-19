
/*
Copyright (c) 2021, Stephen P. Shoecraft
All rights reserved.

This source code is licensed under the BSD-style license found in the
LICENSE file in the root directory of this source tree.
*/

#include <string.h>
#include <stdlib.h>
#include <dirent.h>
#include "common.h"
#include "config.h"
#include "mqtt.h"
#include "influx.h"
#ifdef __WIN32
#include <winsock2.h>
#endif

char SOLARD_BINDIR[SOLARD_PATH_MAX];
char SOLARD_ETCDIR[SOLARD_PATH_MAX];
char SOLARD_LIBDIR[SOLARD_PATH_MAX];
char SOLARD_LOGDIR[SOLARD_PATH_MAX];

#define CFG 1
#define dlevel 4

#if defined(__WIN32) && !defined(__WIN64)
#include <windows.h>
BOOL InitOnceExecuteOnce(
	PINIT_ONCE    InitOnce,
	PINIT_ONCE_FN InitFn,
	PVOID         Parameter,
	LPVOID        *Context
) {
	InitFn(InitOnce,Parameter,Context);
	return TRUE;
}
#endif


static int _getcfg(cfg_info_t *cfg, char *section_name, char *dest, char *what, int ow) {
	char name[256];
	char *p;
	int r;

	dprintf(dlevel,"cfg: %p, section_name: %s, dest: %s, what: %s, ow: %d\n", cfg, section_name, dest, what, ow);
	sprintf(name,"%sdir",what);
	dprintf(dlevel,"name: %s\n", name);

	r = 1;
#if CFG
	p = cfg_get_item(cfg,section_name,name);
	dprintf(dlevel,"%s p: %p\n", name, p);
	if (!p) {
		p = cfg_get_item(cfg,section_name,"prefix");
		dprintf(dlevel,"prefix p: %p\n", p);
		if (!p) goto _getcfg_err;
		if (ow) {
			dprintf(dlevel,"prefix: %s\n", p);
			sprintf(dest,"%s/%s",p,what);
		}
	} else {
		strncpy(dest,p,SOLARD_PATH_MAX-1);
	}
	dprintf(dlevel,"dest: %s\n", dest);
	r = 0;
_getcfg_err:
#endif
	dprintf(dlevel,"returning: %d\n", r);
	return r;
}

static void _getpath(cfg_info_t *cfg, char *section_name, char *home, char *dest, char *what, int ow) {

	dprintf(dlevel,"cfg: %p, home: %s, what: %s, ow: %d\n", cfg, home, what, ow);

	if (cfg && _getcfg(cfg,section_name,dest,what,1) == 0) return;
	if (!ow) return;

#ifdef __WIN32
//	strcpy(dest,"%ProgramFiles%\\SolarDirector");
	strcpy(dest,"c:\\sd");
	return;
#else
	/* Am I root? */
	if (getuid() == 0) {
		if (strcmp(what,"bin") == 0)
			strcpy(dest,"/usr/local/bin");
		else if (strcmp(what,"etc") == 0)
			strcpy(dest,"/usr/local/etc");
		else if (strcmp(what,"lib") == 0)
			strcpy(dest,"/usr/local/lib");
		else if (strcmp(what,"log") == 0)
			strcpy(dest,"/var/log");
		else {
			printf("error: init _getpath: no path set for: %s\n", what);
			dest = ".";
		}
		return;

	/* Not root */
	} else {
		sprintf(dest,"%s/%s",home,what);
		return;
	}
	printf("error: init: _getpath: no path set for: %s, using /tmp\n", what);
	dest = "/tmp";
#endif
}

static void _setp(char *name,char *value) {
	int len=strlen(value);
	if (len > 1 && value[len-1] == '/') value[len-1] = 0;
	fixpath(value,SOLARD_PATH_MAX-1);
	os_setenv(name,value,1);
}

static int solard_get_dirs(cfg_info_t *cfg, char *section_name, char *home, int ow) {

	dprintf(dlevel,"cfg: %p, section_name: %s, home: %s\n", cfg, section_name, home);

	_getpath(cfg,section_name,home,SOLARD_BINDIR,"bin",ow);
	_getpath(cfg,section_name,home,SOLARD_ETCDIR,"etc",ow);
	_getpath(cfg,section_name,home,SOLARD_LIBDIR,"lib",ow);
	_getpath(cfg,section_name,home,SOLARD_LOGDIR,"log",ow);

	_setp("SOLARD_BINDIR",SOLARD_BINDIR);
	_setp("SOLARD_ETCDIR",SOLARD_ETCDIR);
	_setp("SOLARD_LIBDIR",SOLARD_LIBDIR);
	_setp("SOLARD_LOGDIR",SOLARD_LOGDIR);
	dprintf(dlevel-1,"BINDIR: %s, ETCDIR: %s, LIBDIR: %s, LOGDIR: %s\n", SOLARD_BINDIR, SOLARD_ETCDIR, SOLARD_LIBDIR, SOLARD_LOGDIR);

	return 0;
}

int solard_common_init(int argc,char **argv,char *version,opt_proctab_t *add_opts,int start_opts) {
	/* std command-line opts */
	static char logfile[256];
	char home[246],configfile[256];
	cfg_info_t *cfg;
	static int append_flag;
	static int back_flag,verb_flag,vers_flag;
	static int help_flag,err_flag;
	static opt_proctab_t std_opts[] = {
		{ "-b|run in background",&back_flag,DATA_TYPE_LOGICAL,0,0,"no" },
#ifdef DEBUG
		{ "-d:#|set debugging level",&debug,DATA_TYPE_INT,0,0,"0" },
#endif
		{ "-e|redirect output to stderr",&err_flag,DATA_TYPE_LOGICAL,0,0,"N" },
		{ "-h|display program options",&help_flag,DATA_TYPE_LOGICAL,0,0,"N" },
		{ "-l:%|redirect output to logfile",&logfile,DATA_TYPE_STRING,sizeof(logfile)-1,0,"" },
		{ "-a|append to logfile",&append_flag,DATA_TYPE_LOGICAL,0,0,"N" },
		{ "-v|enable verbose output ",&verb_flag,DATA_TYPE_LOGICAL,0,0,"N" },
		{ "-V|program version ",&vers_flag,DATA_TYPE_LOGICAL,0,0,"N" },
		{ 0,0,0,0,0 }
	};
	opt_proctab_t *opts;
	char *file;
	volatile int error,log_opts;
	char *ident;
#ifdef __WIN32
	WSADATA wsaData;
	int iResult;
#endif

//	printf("initializng...\n");

	append_flag = back_flag = verb_flag = help_flag = err_flag = 0;

        /* Open the startup log */
	ident = "startup";
	log_opts = start_opts;
	if (log_opts & LOG_WX) {
		if (argv) ident = argv[argc+1];
		else log_opts &= ~LOG_WX;
	}
	log_open(ident,0,log_opts);

	dprintf(dlevel,"common_init: argc: %d, argv: %p, add_opts: %p, log_opts: %x\n", argc, argv, add_opts, log_opts);

#ifdef __WIN32
	dprintf(dlevel,"initializng winsock...\n");
	/* Initialize Winsock */
	iResult = WSAStartup(MAKEWORD(2,2), &wsaData);
	if (iResult != 0) {
		log_write(LOG_SYSERR,"WSAStartup");
		return 1;
	}
#endif

	/* If add_opts, add to std */
	opts = (add_opts ? opt_addopts(std_opts,add_opts) : std_opts);

	/* Init opts */
	dprintf(dlevel,"init'ing opts: %p\n",opts);
	opt_init(opts);

	/* Process the opts */
	dprintf(dlevel,"common_init: processing opts...\n");
	error = opt_process(argc,argv,opts);
	if (vers_flag) {
		printf("%s\n",version);
		exit(0);
	}

	/* If help flag, display usage and exit */
	dprintf(dlevel,"common_init: error: %d, help_flag: %d\n",error,help_flag);
	if (!error && help_flag) {
		opt_usage(argv[0],opts);
		error = 1;
	}

	/* If add_opts, free malloc'd opts */
	if (add_opts) mem_free(opts);
	if (error) return 1;

	/* Set the requested flags */
	log_opts &= ~(LOG_DEBUG|LOG_DEBUG2|LOG_DEBUG3|LOG_DEBUG4);
#ifdef DEBUG
	dprintf(dlevel,"debug: %d\n", debug);
	switch(debug) {
		case 4:
		default:
			log_opts |= LOG_DEBUG4;
			/* Fall-through */
		case 3:
			log_opts |= LOG_DEBUG3;
			/* Fall-through */
		case 2:
			log_opts |= LOG_DEBUG2;
			/* Fall-through */
		case 1:
			log_opts |= LOG_DEBUG;
			break;
	}
#endif
	dprintf(dlevel,"verb_flag: %d, err_flag: %d, append_flag: %d, log_opts: %x\n",verb_flag,err_flag,append_flag,log_opts);
	if (verb_flag) log_opts |= LOG_VERBOSE;
	if (err_flag) log_opts |= LOG_STDERR;
	if (append_flag) log_opts &= ~LOG_CREATE;
	else log_opts |= LOG_CREATE;

	/* If back_flag is set, become a daemon */
	if (back_flag) become_daemon();

	/* Re-open the startup log */
	log_open(ident, 0 ,log_opts);

	/* Log to a file? */
	dprintf(dlevel,"logfile: %s\n", logfile);
	if (strlen(logfile)) {
//		strcpy(logfile,os_fnparse(logfile,".log",0));
//		dprintf(dlevel,"logfile: %s\n", logfile);
//		log_lock_id = os_lock_file(logfile,0);
//		if (!log_lock_id) return 1;
		log_opts |= LOG_TIME;
		file = logfile;
		log_opts &= ~LOG_WX;
		ident = "logfile";
	} else {
		file = 0;
	}

	/* Re-open log */
	dprintf(dlevel,"file: %s\n", file);
	log_open(ident,file,log_opts);

	/* Get paths */
	*SOLARD_BINDIR = *SOLARD_ETCDIR = *SOLARD_LIBDIR = *SOLARD_LOGDIR = 0;

	/* Look for configfile in ~/.sd.conf, and /etc/sd.conf, /usr/local/etc/sd.conf */
	*configfile = 0;
	if (gethomedir(home,sizeof(home)-1) == 0) {
		char path[256];

		sprintf(path,"%s/.sd.conf",home);
		fixpath(path,sizeof(path)-1);
		if (access(path,0) == 0) strcpy(configfile,path);
	}
#ifndef __WIN32
	if (access("/etc/sd.conf",R_OK) == 0) strcpy(configfile,"/etc/sd.conf");
	if (access("/usr/local/etc/sd.conf",R_OK) == 0) strcpy(configfile,"/usr/local/etc/sd.conf");
#endif

	cfg = 0;
	dprintf(dlevel,"configfile: %s\n", configfile);
#if CFG
	if (strlen(configfile)) {
		cfg = cfg_read(configfile);
		if (!cfg) log_write(LOG_SYSERR,"cfg_read %s",configfile);
	}
#endif
	solard_get_dirs(cfg,"",home,1);

#if CFG
	if (cfg) cfg_destroy(cfg);
#endif

	return 0;
}

#if 0
int solard_common_config(cfg_info_t *cfg, char *section_name) {
	char home[256];

	dprintf(dlevel,"cfg: %p, section_name: %s\n", cfg, section_name);

	gethomedir(home,sizeof(home)-1);
	dprintf(dlevel,"home: %s\n",home);
	return solard_get_dirs(cfg,section_name,home,0);
}
#endif

int solard_common_startup(config_t **cp, char *sname, char *configfile,
			config_property_t *props, config_function_t *funcs,
			mqtt_session_t **m, mqtt_callback_t *getmsg, void *mctx,
			char *mqtt_info, mqtt_config_t *mc, int config_from_mqtt,
			influx_session_t **i, char *influx_info, influx_config_t *ic,
			JSEngine **js, int rtsize, int stksize, js_outputfunc_t *jsout)
{
	mqtt_config_t gmc;
	influx_config_t gic;
	int mqtt_init_done;

	/* Create MQTT session */
	*m = mqtt_new(getmsg, mctx);
	if (!*m) {
		log_syserror("unable to create MQTT session\n");
		return 1;
	}
	dprintf(dlevel,"m: %p\n", *m);

	/* Create InfluxDB session */
	*i = influx_new();
	if (!*i) {
		log_syserror("unable to create InfluxDB session\n");
		return 1;
	}
	dprintf(dlevel,"i: %p\n", *i);

	/* Init the config */
	*cp = config_init(sname, props, funcs);
	if (!*cp) return 0;
	common_add_props(*cp, sname);
	mqtt_add_props(*cp, &gmc, sname, mc);
	influx_add_props(*cp, &gic, sname, ic);

	mqtt_init_done = 0;
	dprintf(dlevel,"config_from_mqtt: %d, configfile: %s\n", config_from_mqtt, configfile);
	if (config_from_mqtt) {
		/* If mqtt info specified on command line, parse it */
		dprintf(dlevel,"mqtt_info: %s\n", mqtt_info);
		if (strlen(mqtt_info)) mqtt_parse_config(mc,mqtt_info);

		/* init mqtt */
		if (mqtt_init(*m, mc)) return 1;
		mqtt_init_done = 1;

		/* read the config */
//		if (config_from_mqtt(*cp, topic, m)) return 1;

	} else if (strlen(configfile)) {
		int fmt;
		char *p;

		/* Try to determine format */
		p = strrchr(configfile,'.');
		if (p && (strcasecmp(p,".json") == 0)) fmt = CONFIG_FILE_FORMAT_JSON;
		else fmt = CONFIG_FILE_FORMAT_INI;

		dprintf(dlevel,"reading configfile...\n");
		if (config_read(*cp,configfile,fmt)) {
			log_error(config_get_errmsg(*cp));
			return 1;
		}
	}
	config_dump(*cp);

	/* If MQTT not init, do it now */
	if (!mqtt_init_done) {
		dprintf(1,"mqtt_info: %s\n", mqtt_info);
		if (strlen(mqtt_info)) mqtt_parse_config(mc, mqtt_info);

		if (mqtt_init(*m, mc)) return 1;
		mqtt_init_done = 1;
	}

	/* Subscribe to our clientid */
	sprintf(mqtt_info,"%s/%s/%s",SOLARD_TOPIC_ROOT,SOLARD_TOPIC_CLIENTS,mc->clientid);
	mqtt_sub(*m,mqtt_info);

#ifdef JS
	/* Init js scripting */
	*js = JS_EngineInit(rtsize, stksize, jsout);
	common_jsinit(*js);
	types_jsinit(*js);
	config_jsinit(*js, *cp);
	mqtt_jsinit(*js, *m);
	influx_jsinit(*js, *i);
#endif

	return 0;
}

#ifdef JS
enum COMMON_PROPERTY_ID {
	COMMON_PROPERTY_ID_NONE,
	COMMON_PROPERTY_ID_BINDIR,
	COMMON_PROPERTY_ID_ETCDIR,
	COMMON_PROPERTY_ID_LIBDIR,
	COMMON_PROPERTY_ID_LOGDIR,
};

static JSBool common_getprop(JSContext *cx, JSObject *obj, jsval id, jsval *rval) {
	int prop_id;

	dprintf(dlevel,"is_int: %d\n", JSVAL_IS_INT(id));
	if(JSVAL_IS_INT(id)) {
		prop_id = JSVAL_TO_INT(id);
		dprintf(dlevel,"prop_id: %d\n", prop_id);
		switch(prop_id) {
		case COMMON_PROPERTY_ID_BINDIR:
			*rval = STRING_TO_JSVAL(JS_NewStringCopyZ(cx,SOLARD_BINDIR));
			break;
		case COMMON_PROPERTY_ID_ETCDIR:
			*rval = STRING_TO_JSVAL(JS_NewStringCopyZ(cx,SOLARD_ETCDIR));
			break;
		case COMMON_PROPERTY_ID_LIBDIR:
			*rval = STRING_TO_JSVAL(JS_NewStringCopyZ(cx,SOLARD_LIBDIR));
			break;
		case COMMON_PROPERTY_ID_LOGDIR:
			*rval = STRING_TO_JSVAL(JS_NewStringCopyZ(cx,SOLARD_LOGDIR));
			break;
		default:
			dprintf(1,"not found\n");
			JS_ReportError(cx, "property not found");
			return JS_FALSE;
			break;
		}
	}
	return JS_TRUE;
}

static JSBool common_setprop(JSContext *cx, JSObject *obj, jsval id, jsval *vp) {
	int prop_id;

	dprintf(dlevel,"is_int: %d\n", JSVAL_IS_INT(id));
	if(JSVAL_IS_INT(id)) {
		prop_id = JSVAL_TO_INT(id);
		dprintf(dlevel,"prop_id: %d\n", prop_id);
		switch(prop_id) {
		case COMMON_PROPERTY_ID_BINDIR:
			jsval_to_type(DATA_TYPE_STRING,SOLARD_BINDIR,SOLARD_PATH_MAX,cx,*vp);
			break;
		case COMMON_PROPERTY_ID_ETCDIR:
			jsval_to_type(DATA_TYPE_STRING,SOLARD_ETCDIR,SOLARD_PATH_MAX,cx,*vp);
			break;
		case COMMON_PROPERTY_ID_LIBDIR:
			jsval_to_type(DATA_TYPE_STRING,SOLARD_LIBDIR,SOLARD_PATH_MAX,cx,*vp);
			break;
		case COMMON_PROPERTY_ID_LOGDIR:
			jsval_to_type(DATA_TYPE_STRING,SOLARD_LOGDIR,SOLARD_PATH_MAX,cx,*vp);
			break;
		default:
			dprintf(1,"not found\n");
			JS_ReportError(cx, "property not found");
			return JS_FALSE;
			break;
		}
	}
	return JS_TRUE;
}


JSObject *JSCommon(JSContext *cx, void *priv) {
#define COMMON_FLAGS JSPROP_ENUMERATE | JSPROP_PERMANENT | JSPROP_EXPORTED
	JSPropertySpec common_props[] = {
		{ "SOLARD_BINDIR", COMMON_PROPERTY_ID_BINDIR, COMMON_FLAGS, common_getprop, &common_setprop },
		{ "SOLARD_ETCDIR", COMMON_PROPERTY_ID_ETCDIR, COMMON_FLAGS, common_getprop, common_setprop },
		{ "SOLARD_LIBDIR", COMMON_PROPERTY_ID_LIBDIR, COMMON_FLAGS, common_getprop, common_setprop },
		{ "SOLARD_LOGDIR", COMMON_PROPERTY_ID_LOGDIR, COMMON_FLAGS, common_getprop, common_setprop },
		{0}
	};
	JSConstantSpec common_consts[] = {
		JS_STRCONST(SOLARD_TOPIC_ROOT),
		JS_STRCONST(SOLARD_ROLE_CONTROLLER),
		JS_STRCONST(SOLARD_ROLE_STORAGE),
		JS_STRCONST(SOLARD_ROLE_BATTERY),
		JS_STRCONST(SOLARD_ROLE_INVERTER),
		JS_STRCONST(SOLARD_FUNC_STATUS),
		JS_STRCONST(SOLARD_FUNC_INFO),
		JS_STRCONST(SOLARD_FUNC_CONFIG),
		JS_STRCONST(SOLARD_FUNC_DATA),
		{0}
	};
	JSObject *obj = JS_GetGlobalObject(cx);

	dprintf(1,"Defining common properties...\n");
	if(!JS_DefineProperties(cx, obj, common_props)) {
		dprintf(1,"error defining common properties\n");
		return 0;
	}
	if(!JS_DefineConstants(cx, obj, common_consts)) {
		dprintf(1,"error defining common constants\n");
		return 0;
	}
	return obj;
}

void common_add_props(config_t *cp, char *name) {
	config_property_t common_props[] = {
		{ "bindir", DATA_TYPE_STRING, SOLARD_BINDIR, sizeof(SOLARD_BINDIR)-1, 0 },
		{ "etcdir", DATA_TYPE_STRING, SOLARD_ETCDIR, sizeof(SOLARD_ETCDIR)-1, 0 },
		{ "libdir", DATA_TYPE_STRING, SOLARD_LIBDIR, sizeof(SOLARD_LIBDIR)-1, 0 },
		{ "logdir", DATA_TYPE_STRING, SOLARD_LOGDIR, sizeof(SOLARD_LOGDIR)-1, 0 },
		{ "debug", DATA_TYPE_INT, &debug, 0, 0, 0,
			"range", 3, (int []){ 0, 99, 1 }, 1, (char *[]) { "debug level" }, 0, 1, 0 },
		{ 0 }
	};
	config_add_props(cp, name, common_props, 0);
}

int common_jsinit(JSEngine *e) {
	return JS_EngineAddInitFunc(e, "common", JSCommon, 0);
}
#endif

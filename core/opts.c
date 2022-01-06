
/*
Copyright (c) 2021, Stephen P. Shoecraft
All rights reserved.

This source code is licensed under the BSD-style license found in the
LICENSE file in the root directory of this source tree.
*/

#define DEBUG_OPTS 0

#ifdef DEBUG
#undef DEBUG
#endif
#define DEBUG DEBUG_OPTS

#define dlevel 7

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "opts.h"
#include "utils.h"
#include "debug.h"

#if defined(__WIN64) || defined(__arm__)
extern int opterr;
extern int optind;
extern int optopt;
extern char *optarg;
#else
int opterr = 1;		/* print out error messages */
int optind = 1;		/* index into parent argv vector */
int optopt = 0;		/* character checked for validity */
char *optarg;		/* argument associated with option */
#endif

#define	BADCH	(int)'?'
#define	BADARG	(int)':'
#define	EMSG	""

#define HAVE_CONV 1

#if !HAVE_CONV
#include <ctype.h>
char *typenames[] = { "Unknown","char","string","byte","short","int","long","quad","float","double","logical","date","list" };
int conv_type(int dtype,void *dest,int dlen,int stype,void *src,int slen) {
	char temp[1024];
	int *iptr;
	float *fptr;
	double *dptr;
	char *sptr;
	int len,r;

	dprintf(1,"stype: %d, src: %p, slen: %d, dtype: %d, dest: %p, dlen: %d",
		stype,src,slen,dtype,dest,dlen);

	r = 0;
	dprintf(1,"stype: %s, dtype: %s\n", typenames[stype],typenames[dtype]);
	switch(dtype) {
	case DATA_TYPE_INT:
		iptr = dest;
		*iptr = atoi(src);
		r = sizeof(int);
		break;
	case DATA_TYPE_FLOAT:
		fptr = dest;
		*fptr = atof(src);
		r = sizeof(float);
		break;
	case DATA_TYPE_DOUBLE:
		dptr = dest;
		*dptr = strtod(src,0);
		r = sizeof(double);
		break;
	case DATA_TYPE_LOGICAL:
		/* yes/no/true/false */
		{
			char temp[16];
			int i;

			iptr = dest;
			i = 0;
			for(sptr = src; *sptr; sptr++) temp[i++] = tolower(*sptr);
			temp[i] = 0;
			if (strcmp(temp,"yes") == 0 || strcmp(temp,"true") == 0)
				*iptr = 1;
			else
				*iptr = 0;
			r = sizeof(int);
		}
	case DATA_TYPE_STRING:
		switch(stype) {
		case DATA_TYPE_INT:
			iptr = src;
			sprintf(temp,"%d",*iptr);
			break;
		case DATA_TYPE_FLOAT:
			fptr = src;
			sprintf(temp,"%f",*fptr);
			break;
		case DATA_TYPE_DOUBLE:
			dptr = src;
			sprintf(temp,"%f",*dptr);
			break;
		case DATA_TYPE_LOGICAL:
			iptr = src;
			sprintf(temp,"%s",*iptr ? "yes" : "no");
			break;
		case DATA_TYPE_STRING:
			temp[0] = 0;
			len = slen > dlen ? dlen : slen;
			strncat(temp,src,len);
			break;
		default:
			dprintf(dlevel,"conv_type: unknown src type: %d\n", stype);
			temp[0] = 0;
			break;
		}
		dprintf(dlevel,"temp: %s\n",temp);
		strcpy((char *)dest,temp);
		r = strlen(temp);
		break;
	default:
		dprintf(dlevel,"conv_type: unknown dest type: %d\n", dtype);
		break;
	}
}
#endif

static void opt_addopt(list lp, opt_proctab_t *newopt) {
	opt_proctab_t *opt;
//	char k1,k2;
	char *p, kw1[32],kw2[32];

	p = strele(0,"|",newopt->keyword);
	if (p) strncpy(kw1,p,sizeof(kw1)-1);
	else strncpy(kw1,newopt->keyword,sizeof(kw1)-1);
	list_reset(lp);
	while((opt = list_get_next(lp)) != 0) {
		p = strele(0,"|",opt->keyword);
		if (p) strncpy(kw2,p,sizeof(kw2)-1);
		else strncpy(kw2,opt->keyword,sizeof(kw2)-1);
		dprintf(dlevel,"kw1: %s, kw2: %s\n", kw1, kw2);
		if (strncmp(kw1,kw2,2) == 0) {
			dprintf(dlevel,"found!\n");
			list_delete(lp,opt);
		}
	}
#if 0
	printf("k1 keyword: %s\n", newopt->keyword);
	if (newopt->keyword[0] == '-')
		k1 = newopt->keyword[1];
	else
		k1 = newopt->keyword[0];

	dprintf(dlevel,"k1: %c\n", k1);
	list_reset(lp);
	while((opt = list_get_next(lp)) != 0) {
		if (opt->keyword[0] == '-')
			k2 = opt->keyword[1];
		else
			k2 = opt->keyword[0];
	printf("k2 keyword: %s\n", opt->keyword);
		dprintf(dlevel,"k2: %c\n", k2);
		if (k1 == k2) {
			dprintf(dlevel,"found!\n");
			list_delete(lp,opt);
		}
	}
#endif
	dprintf(dlevel,"newopt->type: %d\n", newopt->type);
	if (!newopt->type) return;
	dprintf(dlevel,"adding\n");
	list_add(lp,newopt,sizeof(*newopt));
}

opt_proctab_t *opt_addopts(opt_proctab_t *std_opts,opt_proctab_t *add_opts) {
	opt_proctab_t *opts,*opt;
//	int scount,acount;
//	register int x,y;
	register int i;
	list lp;

	dprintf(dlevel,"std_opts: %p, add_opts: %p\n", std_opts, add_opts);

	if (std_opts && !add_opts) return std_opts;
	else if (!std_opts && add_opts) return add_opts;
	else if (!std_opts && !add_opts) return 0;

	lp = list_create();
	for(i=0; std_opts[i].keyword; i++) opt_addopt(lp,&std_opts[i]);
	for(i=0; add_opts[i].keyword; i++) opt_addopt(lp,&add_opts[i]);
//	for(i=0; std_opts[i].keyword; i++) list_add(lp, &std_opts[i], sizeof(opt_proctab_t));
//	for(i=0; add_opts[i].keyword; i++) list_add(lp, &add_opts[i], sizeof(opt_proctab_t));

	opts = mem_alloc(sizeof(*opt)*(list_count(lp)+1),1);
	if (!opts) {
		log_write(LOG_SYSERR,"opt_addopts: malloc");
		return 0;
	}
	dprintf(dlevel,"end list:\n");
	i = 0;
	list_reset(lp);
	while((opt = list_get_next(lp)) != 0) {
		dprintf(dlevel,"opt: keyword: %s\n", opt->keyword);
		opts[i++] = *opt;
	}
	list_destroy(lp);

	dprintf(dlevel,"returning: %p\n", opts);
	return opts;
#if 0

	scount = acount = 0;
	for(x=0; std_opts[x].keyword; x++) scount++;
	for(x=0; add_opts[x].keyword; x++) acount++;
	dprintf(dlevel,"scount: %d, acount: %d", scount,acount);
	opts = mem_alloc((scount + acount + 1) * sizeof(opt_proctab_t),1);
	if (!opts) {
		log_write(LOG_SYSERR,"opt_addopts: mem_alloc");
		return 0;
	}

	y = 0;
	/* !reqd */
	for(x=0; std_opts[x].keyword; x++) {
		if (std_opts[x].reqd) continue;
		dprintf(dlevel,"kw: %s, reqd: %d",
			std_opts[x].keyword,std_opts[x].reqd);
		memcpy(&opts[y++],&std_opts[x],sizeof(opt_proctab_t));
	}

	for(x=0; add_opts[x].keyword; x++) {
		if (add_opts[x].reqd) continue;
		dprintf(dlevel,"kw: %s, reqd: %d",
			add_opts[x].keyword,add_opts[x].reqd);
		memcpy(&opts[y++],&add_opts[x],sizeof(opt_proctab_t));
	}

	/* reqd */
	for(x=0; std_opts[x].keyword; x++) {
		if (!std_opts[x].reqd) continue;
		dprintf(dlevel,"kw: %s, reqd: %d",
			std_opts[x].keyword,std_opts[x].reqd);
		memcpy(&opts[y++],&std_opts[x],sizeof(opt_proctab_t));
	}

	for(x=0; add_opts[x].keyword; x++) {
		if (!add_opts[x].reqd) continue;
		dprintf(dlevel,"kw: %s, reqd: %d",
			add_opts[x].keyword,add_opts[x].reqd);
		memcpy(&opts[y++],&add_opts[x],sizeof(opt_proctab_t));
	}


	return opts;
#endif
}

/*
 * opt_getopt -- Parse argc/argv argument vector.
 */
int opt_getopt(int argc, char **argv, char *optstr) {
	static char *place = EMSG;		/* option letter processing */
	char *oli;				/* option letter list index */

	dprintf(dlevel,"optstr: %s", optstr);
	if (!*place) {				/* update scanning pointer */
		if (optind >= argc || *(place = argv[optind]) != '-') {
			place = EMSG;
			return (EOF);
		}
		if (place[1] && *++place == '-') {	/* found "--" */
			++optind;
			place = EMSG;
			return (EOF);
		}
	}

	/* option letter okay? */
	if ((optopt = (int)*place++) == (int)':' || !(oli = strchr(optstr, optopt))) {
		/*
		 * if the user didn't specify '-' as an option,
		 * assume it means EOF.
		 */
		if (optopt == (int)'-') return (EOF);
		if (!*place) ++optind;
		if (opterr && *optstr != ':') {
			(void)fprintf(stderr, "%s: illegal option -- %c\n", argv[0], optopt);
		}
		return (BADCH);
	}
	if (*++oli != ':') {			/* don't need argument */
		optarg = NULL;
		if (!*place) ++optind;
	} else {				/* need an argument */
		if (*place)			/* no white space */
			optarg = place;
		else if (argc <= ++optind) {	/* no arg */
			place = EMSG;
			if (*optstr == ':')
				return (BADARG);
			if (opterr)
				(void)fprintf(stderr, "%s: option requires an argument -- %c\n", argv[0], optopt);
			return (BADCH);
		} else {
			optarg = argv[optind];
			dprintf(dlevel,"argv[%d]: %s\n", optind, optarg);
		}
		place = EMSG;
		++optind;
	}
	return (optopt);			/* dump back option letter */
}

void opt_init(opt_proctab_t *opts) {
	register opt_proctab_t *opt;

	/* Init the options table */
	dprintf(dlevel,"initializing options table...\n");
	for(opt = opts; opt->keyword; opt++) {
		dprintf(dlevel,"opt: key: %s, type: %d(%s), len: %d, req: %d\n", opt->keyword, opt->type, typestr(opt->type), opt->len, opt->reqd);

#if 0
		/* If an option is not reqd, all opts after aren't either... */
		dprintf(dlevel,"keyword[0]: %c\n", *opt->keyword);
		if (*opt->keyword && *opt->keyword != '-') {
			if (i)
				opt->reqd = 0;
			else if (opt->reqd == 0)
				i = 1;
		}
#endif

		/* Set default value */
		dprintf(dlevel,"value: %p\n", opt->value);
		if (opt->value) conv_type(opt->type,opt->dest,opt->len,DATA_TYPE_STRING,opt->value,strlen(opt->value));
		opt->have = 0;
	}
	dprintf(dlevel,"done!\n");
	return;
}

int opt_process(int argc,char **argv,opt_proctab_t *opts) {
	char name[32],getoptstr[32];
	register opt_proctab_t *opt;
	register int i;

	name[0] = 0;
	if (argv && argv[0]) strncat(name,argv[0],sizeof(name)-1);

	/* Create the getopt string */
	dprintf(dlevel,"opt_process: creating getopt string...");

	getoptstr[0] = ':';
	i = 1;
	for(opt = opts; opt->keyword; opt++) {
		if (opt->keyword[0] == '-') {
			getoptstr[i++] = opt->keyword[1];
			if (opt->keyword[2] == ':')
				getoptstr[i++] = ':';
		}
	}
	getoptstr[i] = 0;

	/* Get the options */
	while( (i = opt_getopt(argc,argv,getoptstr)) != EOF) {
		if (i == ':') {
			log_write(LOG_ERROR,"the -%c option " "requires an argmument.", optopt);
			opt_usage(name,opts);
			return 1;
		} else if (i == '?') {
			fprintf(stderr,"error: invalid option: -%c\n",optopt);
			opt_usage(name,opts);
			return 1;
		}
		dprintf(dlevel,"opt_process: i: %c\n",i);
		for(opt = opts; opt->keyword; opt++) {
			if (opt->keyword[0] == '-' && opt->keyword[1] == i) {
				dprintf(dlevel,"opt_process: have keyword: %s",opt->keyword);
				opt_setopt(opt,optarg);
			}
		}
	}

	/* Now get the other arguments */
	dprintf(dlevel,"opt_process: getting other arguments...");
	argc--;
	for(opt = opts; opt->keyword; opt++) {
		if (opt->keyword[0] != '-') {
			dprintf(dlevel,"opt_process: " "op: %c, argc: %d, optind: %d", opt->keyword[0],argc,optind);
			/* If we don't have an arg & one's reqd, error */
			if (argc < optind) {
				if (opt->reqd) {
					fprintf(stderr, "error: insufficient arguments.\n");
					opt_usage(name,opts);
					return 1;
				}
			}
			else
				opt_setopt(opt,argv[optind++]);
		}
	}

	/* Check to make sure we have all the reqd args */
	dprintf(dlevel,"opt_process: checking for reqd args...");
	for(opt = opts; opt->keyword; opt++) {
		if (!opt->reqd) continue;
		dprintf(dlevel,"opt_process: " "keyword: %s, reqd: %d, have: %d", opt->keyword,opt->reqd,opt->have);
		if (!opt->have) {
			fprintf(stderr,"error: insufficient arguments.\n");
			opt_usage(name,opts);
			return 1;
		}
	}
	dprintf(dlevel,"opt_process: done!\n");
	return 0;
}

#include <string.h>

void opt_setopt(opt_proctab_t *opt,char *arg) {
	
	dprintf(dlevel,"opt: key: %s, type: %d(%s), len: %d, req: %d, arg: %s\n", opt->keyword, opt->type, typestr(opt->type), opt->len, opt->reqd, arg);

	if (!arg) {
		if (opt->keyword[0] == '-' && opt->type == DATA_TYPE_LOGICAL)
			arg = "True";
		else
			arg = "False";
	}
	dprintf(dlevel,"opt_setopt: keyword: %s, arg: %s", opt->keyword, arg);
	conv_type(opt->type,opt->dest,opt->len, DATA_TYPE_STRING,arg,strlen(arg));
	opt->have = 1;
	return;
}

#include <ctype.h>

char *opt_type(char opt) {
	register char *str;

	switch(toupper(opt)) {
		case ':':
			str = "string";
			break;
		case '#':
			str = "number";
			break;
		case '%':
			str = "filename";
			break;
		case '@':
			str = "hostname";
			break;
		case '/':
			str = "path";
			break;
		case 'P':
			str = "IP port #";
			break;
		case 'I':
#ifdef vms
			str = "database";
#else
			str = "user id";
#endif
			break;
		case 'T':
			str = "table";
			break;
		default:
			str = "unknown";
			break;
	}
	return str;
}

#include <string.h>
#include <ctype.h>

static int has_arg(char *s) {
	register char *p;

	for(p = s; *p; p++) {
		if (*p == ':')
			return 1;
	}
	return 0;
}

static char *get_type(char *s) {
	static char type[32];
	register char *p;
	int x;

	dprintf(dlevel,"get_type: s: %s\n", s);
	x=0;
	for(p = s+1; *p; p++) {
		if (!isalnum(*p))
			break;
		type[x++] = *p;
	}
	dprintf(dlevel,"get_type: x: %d\n",x);
	if (x)
		type[x] = 0;
	else
		strcpy(type,opt_type(*s));
	dprintf(dlevel,"get_type: type: %s\n",type);
	return type;
}

void opt_usage(char *name, opt_proctab_t *opts) {
	char temp[128],*ptr;
	register opt_proctab_t *opt;
	register int i;

#if 0
	fprintf(stderr,"%s, %s\n",descrip_string,version_string);
	fprintf(stderr,"%s\n",COPYRIGHT_NOTICE);
	fprintf(stderr,"%s\n",RIGHTS_NOTICE);
	fprintf(stderr,"\n");
#endif
//	ptr = (os_config.prog.dev ? os_config.prog.dev : os_config.prog.name);
	fprintf(stderr,"usage: %s ",name);

	/* Do the not-req opts w/o args */
	i = 0;
	for(opt = opts; opt->keyword; opt++) {
		if (opt->reqd) continue;
		if (opt->keyword[0] == '-' && !has_arg(opt->keyword)) {
			if (!i) {
				fprintf(stderr,"[-");
				i++;
			}
			fprintf(stderr,"%c",opt->keyword[1]);
		}
	}
	if (i) fprintf(stderr,"] ");

	/* Do the not-req opts with args */
	for(opt = opts; opt->keyword; opt++) {
		if (opt->reqd) continue;
		if (opt->keyword[0] == '-' && has_arg(opt->keyword))
			fprintf(stderr,"[-%c <%s>] ",opt->keyword[1],
				get_type(&opt->keyword[3]));
	}

	/* Do the req opts w/o args */
	i = 0;
	for(opt = opts; opt->keyword; opt++) {
		if (!opt->reqd) continue;
		if (opt->keyword[0] == '-' && !has_arg(opt->keyword)) {
			if (!i) {
				fprintf(stderr,"-");
				i++;
			}
			fprintf(stderr,"%c",opt->keyword[1]);
		}
	}

	/* Do the req opts with args */
	for(opt = opts; opt->keyword; opt++) {
		if (!opt->reqd) continue;
		if (opt->keyword[0] == '-' && has_arg(opt->keyword))
			fprintf(stderr,"-%c <%s> ",opt->keyword[1],
				get_type(&opt->keyword[3]));
	}

	/* Do the args */
	for(opt = opts; opt->keyword; opt++) {
		if (opt->keyword[0] == '-') continue;
		ptr = get_type(&opt->keyword[0]);
		if (strcmp(ptr,"unknown")==0) {
			strcpy(temp,opt->keyword);
			ptr = strchr(temp,'|');
			if (ptr) *ptr = 0;
		} else {
			strcpy(temp,ptr);
		}
		if (opt->reqd)
			fprintf(stderr,"<%s> ",temp);
		else
			fprintf(stderr,"[<%s>] ",temp);
	}
	fprintf(stderr,"\n");

	fprintf(stderr,"where:\n");
	fprintf(stderr,"\t[]\t\toptional arguments\n");
	fprintf(stderr,"\t<>\t\targument type\n");
	fprintf(stderr,"options:\n");
	for(opt = opts; opt->keyword; opt++) {
		if (opt->keyword[0] == '-') {
			fprintf(stderr,"\t-%c\t\t",opt->keyword[1]);
			i = 2;
		}
		else {
			ptr = get_type(&opt->keyword[0]);
			if (strcmp(ptr,"unknown")==0) {
				strcpy(temp,opt->keyword);
				ptr = strchr(temp,'|');
				if (ptr) *ptr = 0;
			} else {
				strcpy(temp,ptr);
			}
			fprintf(stderr,"\t<%s>\t",temp);
			if (strlen(temp)+2 < 8) fprintf(stderr,"\t");
			i = 1;
		}
		ptr = strchr(opt->keyword,'|');
		if (ptr) {
			ptr++;
			fprintf(stderr,"%s\n",ptr);
		}
		else {
			if (opt->keyword[i] == ':')
				fprintf(stderr,"a %s\n",
					get_type(&opt->keyword[i+1]));
			else
				fprintf(stderr,"is unknown.\n");
		}
	}	
	return;
}

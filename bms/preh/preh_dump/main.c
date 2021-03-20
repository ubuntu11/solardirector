
#include "mybmm.h"

int debug = 0;

#define IF "can1"

extern mybmm_module_t can_module;
extern mybmm_module_t preh_module;

int update_pack(mybmm_pack_t *pp) {
	int err;

	dprintf(1,"pack: name: %s, type: %s, transport: %s\n", pp->name, pp->type, pp->transport);
	if (pp->open(pp->handle)) {
		dprintf(1,"pack open error\n");
		return 1;
	}
	printf("Waiting 10s....\n");
	sleep(10);
	dprintf(5,"reading...\n");
	err = 0;
	if (pp->read(pp->handle)) {
		dprintf(1,"pack read error!\n");
		err = 1;
	}
	dprintf(5,"closing...\n");
	pp->close(pp->handle);
	return err;
}

int init_pack(mybmm_config_t *conf, mybmm_pack_t *pp, char *opts, mybmm_module_t *cp, mybmm_module_t *tp) {
	memset(pp,0,sizeof(*pp));
	strcpy(pp->type,"preh");
	strcpy(pp->transport,"can");
	sprintf(pp->target,"%s,500000",IF);
	strcpy(pp->opts,opts);
	pp->open = cp->open;
	pp->read = cp->read;
	pp->close = cp->close;
	dprintf(1,"calling preh->new...\n");
	pp->handle = cp->new(conf,pp,tp);
	return 0;
}

int main(int argc, char **argv) {
	mybmm_config_t conf;
	mybmm_pack_t pack;
	mybmm_module_t *cp,*tp;

	int opt,bytes,action,pretty,all,i;
	char *transport,*target,*label,*filename,*outfile,*p,*opts;

	while ((opt=getopt(argc, argv, "+acd:n:t:e:f:jJo:rwlh")) != -1) {
		switch (opt) {
		case 'd':
			debug = atoi(optarg);
			printf("debug: %d\n", debug);
			break;
		case 'h':
			printf("usage lol!\n");
			return 0;
			break;
		}
	}

	memset(&conf,0,sizeof(conf));
	conf.cfg = 0;

	tp = &can_module;
	if (tp->init) tp->init(&conf);
	cp = &preh_module;
	if (cp->init) cp->init(&conf);

	dprintf(1,"calling preh->new...\n");
	init_pack(&conf,&pack,"0,3", cp, tp);
	update_pack(&pack);
	return 0;
}


/*
Copyright (c) 2021, Stephen P. Shoecraft
All rights reserved.

This source code is licensed under the BSD-style license found in the
LICENSE file in the root directory of this source tree.
*/

#include "jk.h"

int jk_agent_init(jk_session_t *s, int argc, char **argv) {
	opt_proctab_t jk_opts[] = {
		/* Spec, dest, type len, reqd, default val, have */
		{ "-t::|transport,target,opts",&s->tpinfo,DATA_TYPE_STRING,sizeof(s->tpinfo)-1,0,"" },
		{ "-T::|override mqtt topic",&s->topic,DATA_TYPE_STRING,sizeof(s->topic)-1,0,"" },
		{ "-F|flatten arrays",&s->flatten,DATA_TYPE_BOOLEAN,0,0,"N" },
		OPTS_END
	};
	config_property_t jk_props[] = {
		{ "transport", DATA_TYPE_STRING, s->transport, sizeof(s->transport)-1, 0, 0 },
		{ "target", DATA_TYPE_STRING, s->target, sizeof(s->target)-1, 0, 0 },
		{ "topts", DATA_TYPE_STRING, s->topts, sizeof(s->topts)-1, 0, 0 },
		{ "topic", DATA_TYPE_STRING, s->topic, sizeof(s->topic)-1, 0, 0 },
		{ "flatten", DATA_TYPE_BOOLEAN, &s->flatten, 0, 0, 0 },
		{ "state", DATA_TYPE_INT, &s->state, 0, 0, CONFIG_FLAG_READONLY | CONFIG_FLAG_NOSAVE | CONFIG_FLAG_NOPUB },
		{0}
	};
	config_function_t jk_funcs[] = {
		{0}
	};

	s->ap = agent_init(argc,argv,jk_opts,&jk_driver,s,jk_props,jk_funcs);
	if (!s->ap) return 1;
	dprintf(1,"topic: %s(%d)\n", s->topic, strlen(s->topic));
	if (!strlen(s->topic)) agent_mktopic(s->topic,sizeof(s->topic)-1,s->ap,s->ap->instance_name,SOLARD_FUNC_DATA);

	/* Set battery name */
	strcpy(s->data.name,s->ap->instance_name);
	return 0;
}

void jk_config_add_jk_data(jk_session_t *s) {
	/* Only used by JS funcs */
	uint32_t flags = CONFIG_FLAG_NOSAVE | CONFIG_FLAG_NOPUB;
	config_property_t jk_data_props[] = {
		{ "name", DATA_TYPE_STRING, s->ap->instance_name, sizeof(s->ap->instance_name), 0, CONFIG_FLAG_READONLY | flags },
		{ "capacity", DATA_TYPE_FLOAT, &s->data.capacity, 0, 0, CONFIG_FLAG_READONLY | flags },
		{ "voltage", DATA_TYPE_FLOAT, &s->data.voltage, 0, 0, CONFIG_FLAG_READONLY | flags },
		{ "current", DATA_TYPE_FLOAT, &s->data.current, 0, 0, CONFIG_FLAG_READONLY | flags },
		{ "ntemps", DATA_TYPE_INT, &s->data.ntemps, 0, 0, CONFIG_FLAG_READONLY | flags },
		{ "temps", DATA_TYPE_FLOAT_ARRAY, &s->data.temps, JK_MAX_TEMPS, 0, CONFIG_FLAG_READONLY | flags },
		{ "ncells", DATA_TYPE_INT, &s->data.ncells, 0, 0, CONFIG_FLAG_READONLY | flags },
		{ "cellvolt", DATA_TYPE_FLOAT_ARRAY, &s->data.cellvolt, JK_MAX_CELLS, 0, CONFIG_FLAG_READONLY | flags },
		{ "cellres", DATA_TYPE_FLOAT_ARRAY, &s->data.cellres, JK_MAX_CELLS, 0, CONFIG_FLAG_READONLY | flags },
		{ "cell_min", DATA_TYPE_FLOAT, &s->data.cell_min, 0, 0, CONFIG_FLAG_READONLY | flags },
		{ "cell_max", DATA_TYPE_FLOAT, &s->data.cell_max, 0, 0, CONFIG_FLAG_READONLY | flags },
		{ "cell_diff", DATA_TYPE_FLOAT, &s->data.cell_diff, 0, 0, CONFIG_FLAG_READONLY | flags },
		{ "cell_avg", DATA_TYPE_FLOAT, &s->data.cell_avg, 0, 0, CONFIG_FLAG_READONLY | flags },
		{ "cell_total", DATA_TYPE_FLOAT, &s->data.cell_total, 0, 0, CONFIG_FLAG_READONLY | flags },
		{ "balancebits", DATA_TYPE_INT, &s->data.balancebits, 0, 0, CONFIG_FLAG_READONLY | flags },
		{0}
	};

	 /* Add info_props to config */
	config_add_props(s->ap->cp, "jk_data", jk_data_props, flags);
}

int jk_config(void *h, int req, ...) {
	jk_session_t *s = h;
	va_list va;
	void **vp;
	int r;

	r = 1;
	va_start(va,req);
	dprintf(1,"req: %d\n", req);
	switch(req) {
	case SOLARD_CONFIG_INIT:
	    {
		dprintf(1,"**** CONFIG INIT *******\n");

		/* 1st arg is AP */
		s->ap = va_arg(va,solard_agent_t *);
		dprintf(1,"ap: %p\n", s->ap);

		/* -t takes precedence over config */
		dprintf(1,"tpinfo: %s\n", s->tpinfo);
		if (strlen(s->tpinfo)) {
			*s->transport = *s->target = *s->topts = 0;
			strncat(s->transport,strele(0,",",s->tpinfo),sizeof(s->transport)-1);
			strncat(s->target,strele(1,",",s->tpinfo),sizeof(s->target)-1);
			strncat(s->topts,strele(2,",",s->tpinfo),sizeof(s->topts)-1);
		}
		/* XXX hax */
		if (!strlen(s->topts)) strcpy(s->topts,"ffe1");

		/* Init our transport */
		if (jk_tp_init(s)) return 1;

		/* Add our internal params to the config */
//		jk_config_add_parms(s);

#ifdef JS
		/* Init JS */
		jk_config_add_jk_data(s);
		r = jk_jsinit(s);
#endif
	    }
	    break;
	case SOLARD_CONFIG_GET_INFO:
		{
			json_value_t **vpp = va_arg(va,json_value_t **);
			dprintf(1,"vpp: %p\n", vpp);
			if (vpp) {
				*vpp = jk_get_info(s);
				if (*vpp) r = 0;
			}
		}
		break;
	case SOLARD_CONFIG_GET_DRIVER:
		dprintf(1,"GET_DRIVER called!\n");
		vp = va_arg(va,void **);
		if (vp) {
			*vp = s->tp;
			r = 0;
		}
		break;
	case SOLARD_CONFIG_GET_HANDLE:
		dprintf(1,"GET_HANDLE called!\n");
		vp = va_arg(va,void **);
		if (vp) {
			*vp = s->tp_handle;
			r = 0;
		}
		break;
	}
	dprintf(1,"returning: %d\n", r);
	va_end(va);
	return r;
}

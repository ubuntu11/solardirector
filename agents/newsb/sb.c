/*
Copyright (c) 2021, Stephen P. Shoecraft
All rights reserved.

This source code is licensed under the BSD-style license found in the
LICENSE file in the root directory of this source tree.
*/

#include "sb.h"

#define SB_LOGIN "dyn/login.json"
#define SB_LOGOUT "dyn/logout.json"
#define SB_GETVAL "dyn/getValues.json"
#define SB_GETPAR "dyn/getParamValues.json"
#define SB_LOGGER "dyn/getLogger.json"
#define SB_ALLVAL "dyn/getAllOnlValues.json"
#define SB_ALLPAR "dyn/getAllParamValues.json"
#define SB_SETPAR "dyn/setParamValues.json"

#if 0
# login: '/login.json',
# logout: '/logout.json',
# getTime: '/getTime.json',
# getValues: '/getValues.json',
# getAllOnlValues: '/getAllOnlValues.json',
# getAllParamValues: '/getAllParamValues.json',
# setParamValues: '/setParamValues.json',
# getWebSrvConf: '/getWebSrvConf.json',
# getEventsModal: '/getEvents.json',
# getEvents: '/getEvents.json',
# getLogger: '/getLogger.json',
# getBackup: '/getConfigFile.json',
# loginGridGuard: '/loginSR.json',
# sessionCheck: '/sessionCheck.json',
# setTime: '/setTime.json',
# backupUpdate: 'input_file_backup.htm',
# fwUpdate: 'input_file_update.htm',
# SslCertUpdate: 'input_file_ssl.htm'
#endif

static int getsmaobjectpath(char *dest, int destsize, sma_object_t *o) {
	char path[256], *tagstr, *s, *p;
	int i;

	tagstr = getsmastring(o->tag);
	if (!tagstr) return 1;
	p = path;
	for(i=0; i < 8; i++) {
		if (!o->taghier[i]) break;
		s = getsmastring(o->taghier[i]);
		if (!s) continue;
		p += sprintf(p,"(%d)%s->",o->taghier[i],s);
	}
	p += sprintf(p,"(%d)%s",o->tag,tagstr);
	dprintf(5,"path: %s\n",path);
	strncpy(dest,path,destsize);
	return 0;
}

static void _getval(sb_session_t *s, sb_value_t *vp,  json_value_t *rv) {
	json_value_t *v;
	json_object_t *o;
	json_array_t *a;
	int type;
	register int i;
	register char *p;

	type = json_value_get_type(rv);
	dprintf(1,"type: %d (%s)\n", type, json_typestr(type));
	switch(type) {
	case JSON_TYPE_NULL:
		vp->type = DATA_TYPE_NULL;
		break;
	case JSON_TYPE_NUMBER:
		vp->type = DATA_TYPE_DOUBLE;
		vp->d = json_value_get_number(rv);
		dprintf(1,"vp->d(1): %f, mult: %f\n", vp->d, vp->o->mult);
		if (float_equals(vp->o->mult,0.0)) vp->o->mult = 1.0;
		vp->d *= vp->o->mult;
		dprintf(1,"vp->d(2): %f\n", vp->d);
		break;
	case JSON_TYPE_STRING:
		vp->type = DATA_TYPE_STRING;
		p = json_value_get_string(rv);
		vp->s = malloc(strlen(p)+1);
		if (!vp->s) {
			log_syserror("_getval: malloc(%d)\n", strlen(p)+1);
			return;
		}
		strcpy(vp->s,p);
		break;
	case JSON_TYPE_ARRAY:
		vp->type = DATA_TYPE_STRING_LIST;
		a = json_value_get_array(rv);
		dprintf(1,"array count: %d\n", a->count);
		vp->l = list_create();
		for(i=0; i < a->count; i++) {
			if (json_value_get_type(a->items[i]) != JSON_TYPE_OBJECT) return;
			o = json_value_get_object(a->items[i]);
			dprintf(1,"object count: %d\n", o->count);
			if (o->count < 1) return;
			/* The only option we support here is "tag" (number) */
			v = json_object_dotget_value(o,"tag");
			if (!v) return;
			p = getsmastring((int)json_value_get_number(v));
			if (!p) return;
			list_add(vp->l,p,strlen(p)+1);
		}
		break;
	default:
		log_error("_getval: unhandled type: %d(%s)\n", type, json_typestr(type));
		return;
		break;
	}
}

static int getvals(sb_session_t *s,json_object_t *o) {
	sb_value_t newval;
	json_value_t *v;
	json_object_t *oo;
	json_array_t *a;
	int i,j,type;

	dprintf(1,"count: %d\n", o->count);
	for(i=0; i < o->count; i++) {
		v = o->values[i];
		type = json_value_get_type(v);
		dprintf(1,"names[%d]: %s, type: %d (%s)\n", i, o->names[i], type, json_typestr(type));
		if (type != JSON_TYPE_OBJECT) continue;

		oo = json_value_get_object(v);
		if (oo->count < 1) continue;

		v = oo->values[0];
		if (json_value_get_type(v) != JSON_TYPE_ARRAY) continue;
		a = json_value_get_array(v);
		dprintf(1,"array count: %d\n", a->count);
		for(j=0; j < a->count; j++) {
			if (json_value_get_type(a->items[j]) != JSON_TYPE_OBJECT) continue;
			v = json_object_dotget_value(json_value_get_object(a->items[j]),"val");
			dprintf(1,"v: %p\n", v);
			if (!v) continue;
			memset(&newval,0,sizeof(newval));
			newval.o = getsmaobject(o->names[i]);
			if (!newval.o) continue;
			if (a->count > 1) sprintf(newval.w," [%c]",'A' + j);
			else *newval.w = 0;
			dprintf(1,"w: %s\n", newval.w);
			_getval(s,&newval,v);
			list_add(s->data,&newval,sizeof(newval));
		}
	}
	return 0;
}

static size_t wrfunc(void *ptr, size_t size, size_t nmemb, void *ctx) {
	sb_session_t *s = ctx;
	json_value_t *v,*vv;
	json_object_t *o,*oo;
	int i,j,type;
	bool bval;

	v = json_parse(ptr);
	if (!v) goto done;
#if 0
	{"result":{"sid":"qdgNwugbjD0bktwQ"}}
	{"result":{"019D-xxxxxA37":{"6400_00260100":{"1":[{"val":1535047}]}}}}
	{"result":{"isLogin":false}}
#endif
	dprintf(1,"type: %s\n", json_typestr(json_value_get_type(v)));
	if (json_value_get_type(v) != JSON_TYPE_OBJECT) goto done;
	o = json_value_get_object(v);
	for(i=0; i < o->count; i++) {
		dprintf(1,"names[%d]: %s\n", i, o->names[i]);
		if (strcmp(o->names[i],"result") == 0) {
			v = o->values[i];
			dprintf(1,"values[%d] type: %s\n", i, json_typestr(json_value_get_type(v)));
			if (json_value_get_type(v) != JSON_TYPE_OBJECT) continue;
			oo = json_value_get_object(v);
			for(j=0; j < oo->count; j++) {
				vv = oo->values[j];
				type = json_value_get_type(vv);
				dprintf(1,"names[%d]: %s, type: %d(%s)\n", j, oo->names[j], type, json_typestr(type));
				if (strcmp(oo->names[j],"sid") == 0) {
					if (type != JSON_TYPE_STRING) continue;
					strncpy(s->session_id,json_value_get_string(vv),sizeof(s->session_id)-1);
				} else if (strcmp(oo->names[i],"isLogin") == 0) {
					bval = json_value_get_boolean(oo->values[i]);
					printf("isLogin: %d\n", bval);
//					if (s->isLogin) s->isLogin = 0;
//					*s->sid = 0;
				} else if (type == JSON_TYPE_OBJECT) {
					getvals(s,json_value_get_object(vv));
				}
			}
		} else if (strcmp(o->names[i],"err") == 0) {
			oo = json_value_get_object(v);
			if (oo->count < 1) goto done;
			if (json_value_get_type(oo->values[0]) == JSON_TYPE_NUMBER) {
				s->errcode = json_value_get_number(oo->values[0]);
				dprintf(1,"errcode: %d\n", s->errcode);
			}
		}
	}

done:
	return size*nmemb;
}

int sb_request(sb_session_t *s, char *func, char *fields) {
	char url[128],*p;
	CURLcode res;

	dprintf(1,"func: %s, fields: %s\n", func, fields);

	p = url;
	if (strncmp(s->endpoint,"http",4) != 0) p += sprintf(p,"https://");
	p += sprintf(p,"%s/%s",s->endpoint,func);
	if (*s->session_id) p += sprintf(p,"?sid=%s",s->session_id);
	dprintf(1,"url: %s\n", url);
	curl_easy_setopt(s->curl, CURLOPT_URL, url);
	if (fields) curl_easy_setopt(s->curl, CURLOPT_POSTFIELDS, fields);
	else curl_easy_setopt(s->curl, CURLOPT_POSTFIELDS, "{}");
	res = curl_easy_perform(s->curl);
	if (res != CURLE_OK) {
		log_error("sb_request failed: %s\n", curl_easy_strerror(res));
		sb_driver.close(s);
		return 1;
	}
	return 0;
}

void *sb_new(void *conf, void *driver, void *driver_handle) {
	sb_session_t *s;
	struct curl_slist *hs = curl_slist_append(0, "Content-Type: application/json");

	s = calloc(1,sizeof(*s));
	if (!s) {
		log_syserror("sb_new: calloc");
		return 0;
	}
	s->ap = conf;
	s->curl = curl_easy_init();
	if (!s->curl) {
		log_syserror("sb_new: curl_init");
		free(s);
		return 0;
	}
//	curl_easy_setopt(s->curl, CURLOPT_VERBOSE, 1L);
	curl_easy_setopt(s->curl, CURLOPT_SSL_VERIFYPEER, 0L);
	curl_easy_setopt(s->curl, CURLOPT_HTTPHEADER, hs);
	curl_easy_setopt(s->curl, CURLOPT_WRITEFUNCTION, wrfunc);
	curl_easy_setopt(s->curl, CURLOPT_WRITEDATA, s);

	return s;
}

int sb_open(void *handle) {
	sb_session_t *s = handle;

	if (!s) return 1;

	dprintf(3,"open: %d\n", solard_check_state(s,SB_STATE_OPEN));
	if (solard_check_state(s,SB_STATE_OPEN)) return 0;

	/* Create the login info */
	if (!s->login_fields) {
		json_object_t *o;

		o = json_create_object();
		json_object_set_string(o,"right","usr");
		json_object_set_string(o,"pass",s->password);
		s->login_fields = json_dumps(json_object_get_value(o),0);
		if (!s->login_fields) {
			log_syserror("sb_open: create login fields: %s\n", strerror(errno));
			return 1;
		}
		json_destroy_object(o);
	}

	*s->session_id = 0;
	if (sb_request(s,SB_LOGIN,s->login_fields)) return 1;
	if (!*s->session_id) {
		log_error("sb_open: error getting session_id .. bad password?\n");
		return 1;
	}

	dprintf(1,"session_id: %s\n", s->session_id);
	solard_set_state(s,SB_STATE_OPEN);
	return 0;
}

int sb_close(void *handle) {
	sb_session_t *s = handle;

	if (!s) return 1;
	dprintf(1,"open: %d\n", solard_check_state(s,SB_STATE_OPEN));
	if (!solard_check_state(s,SB_STATE_OPEN)) return 0;

	sb_request(s,SB_LOGOUT,"{}");
	curl_easy_cleanup(s->curl);
	s->curl = 0;
	return 0;
}

static int sb_read(void *handle, void *buf, int buflen) {
	sb_session_t *s = handle;

	/* Open if not already open */
        if (sb_driver.open(s)) return 1;

	if (!s->read_fields) {
		json_object_t *o;
		json_array_t *a;

		o = json_create_object();
		a = json_create_array();
		json_object_set_array(o,"destDev",a);
		s->read_fields = json_dumps(json_object_get_value(o),0);
		if (!s->read_fields) {
			log_syserror("mkfields: json_dumps");
			return 1;
		}
		dprintf(1,"fields: %s\n", s->read_fields);
	}


	s->data = list_create();
	if (sb_request(s,SB_ALLVAL,s->read_fields)) {
		list_destroy(s->data);
		return 1;
	}
	list_destroy(s->data);
	return 0;
}

solard_driver_t sb_driver = {
	SOLARD_DRIVER_AGENT,
	"sb",
	sb_new,				/* New */
	0,				/* Destroy */
	sb_open,			/* Open */
	sb_close,			/* Close */
	sb_read,			/* Read */
	0,				/* Write */
	sb_config			/* Config */
};

/*
Copyright (c) 2021, Stephen P. Shoecraft
All rights reserved.

This source code is licensed under the BSD-style license found in the
LICENSE file in the root directory of this source tree.
*/

#include "sb.h"
#include "sma_strings.h"
#include "sma_objects.h"

#define TESTING 0

#define URL_LOGIN "dyn/login.json"
#define URL_LOGOUT "dyn/logout.json"
#define URL_VALUES "dyn/getValues.json"
#define URL_LOGGER "dyn/getLogger.json"
#define URL_SESCHK "dyn/sessionCheck.json"
#define URL_ALLVAL "dyn/getAllOnlValues.json"

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

static char *getsmastring(int tag) {
	sma_string_t *s;
//	char tagstr[16];

	dprintf(1,"tag: %d\n", tag);
//	sprintf(tagstr,"%d",tag);
//	dprintf(1,"tagstr: %s\n", tagstr);

	for(s=sma_strings; s->tag; s++) {
//		if (strcmp(s->tag,tag) == 0) {
		if (s->tag == tag) {
			dprintf(1,"found\n");
			return s->string;
		}
	}
	printf("TAG NOT FOUND: %d\n", tag);
	dprintf(1,"NOT found\n");
	return 0;
}


static sma_object_t *getsmaobject(char *id) {
	sma_object_t *o;

	dprintf(1,"id: %s\n", id);

	for(o=sma_objects; o->id; o++) {
		if (strcmp(o->id,id) == 0) {
			dprintf(1,"found\n");
			return o;
		}
	}
	printf("OBJECT NOT FOUND: %s\n", id);
	dprintf(1,"NOT found\n");
	return 0;
}

static int getsmaobjectpath(char *dest, int destsize, char *id) {
	sma_object_t *o;
	char path[256], *tagstr, *s, *p;
	int i;

	o = getsmaobject(id);
	if (!o) return 1;
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
	dprintf(1,"path: %s\n",path);
	strncpy(dest,path,destsize);
	return 0;
}

static void _getval(char *id, char *w, json_value_t *rv) {
	struct sbval val;
	json_value_t *v;
	json_object_t *o;
	json_array_t *a;
	int type;
	double dval;
	char *p,*s;
	char path[256],value[1024];
	register int i;

	dprintf(1,"id: %s\n", id);
	if (getsmaobjectpath(path,sizeof(path)-1,id)) return;

	memset(&val,0,sizeof(val));
	val.values = list_create();
	strncpy(val.id,id,sizeof(val.id)-1);
	type = json_value_get_type(rv);
	dprintf(1,"type: %d (%s)\n", type, json_typestr(type));
	switch(type) {
	case JSON_TYPE_NULL:
		val.type = DATA_TYPE_NULL;
		strcpy(value,"null");
		break;
	case JSON_TYPE_NUMBER:
		val.type = DATA_TYPE_DOUBLE;
		dval = json_value_get_number(rv);
		sprintf(value,"%f",dval);
		break;
	case JSON_TYPE_STRING:
		val.type = DATA_TYPE_STRING;
		p = json_value_get_string(rv);
		strncpy(value,p,sizeof(value)-1);
		break;
	case JSON_TYPE_ARRAY:
		a = json_value_get_array(rv);
		dprintf(1,"array count: %d\n", a->count);
		s = value;
		for(i=0; i < a->count; i++) {
			if (json_value_get_type(a->items[i]) != JSON_TYPE_OBJECT) return;
			o = json_value_get_object(a->items[i]);
			dprintf(0,"object count: %d\n", o->count);
			if (o->count < 1) return;
			/* The only option we support here is "tag" (number) */
			v = json_object_dotget_value(o,"tag");
			if (!v) return;
			dval = json_value_get_number(v);
			dprintf(1,"dval: %f\n", dval);
			p = getsmastring((int)dval);
			if (!p) return;
			printf("value[%d]: %s\n", i, p);
			if (i) s += sprintf(s,",");
			s += sprintf(s,"%s",p);
		}
		break;
	default:
		dprintf(0,"unhandled type: %d(%s)\n", type, json_typestr(type));
		return;
		break;
	}
	printf("(%s)%s%s: %s\n", id, path, w, value);
}

static int getvals(json_object_t *o) {
	json_value_t *v;
	json_object_t *oo;
	json_array_t *a;
	int i,j,type;
	char w[5];

	dprintf(0,"count: %d\n", o->count);
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
			if (a->count > 1) sprintf(w," [%c]",'A' + j);
			else *w = 0;
			_getval(o->names[i],w,v);
		}
	}
	return 0;
}

static size_t wrfunc(void *ptr, size_t size, size_t nmemb, void *ctx) {
	json_value_t *v,*vv;
	json_object_t *o,*oo;
	char *p;
	int i,j,type;
	bool bval;

	printf("ptr: %s\n", (char *)ptr);
	v = json_parse(ptr);
	if (!v) goto done;
#if 0
	{"result":{"sid":"qdgNwugbjD0bktwQ"}}
	{"result":{"019D-xxxxxA37":{"6400_00260100":{"1":[{"val":1535047}]}}}}
	{"result":{"isLogin":false}}
#endif
	printf("type: %s\n", json_typestr(json_value_get_type(v)));
	if (json_value_get_type(v) != JSON_TYPE_OBJECT) goto done;
	o = json_value_get_object(v);
	for(i=0; i < o->count; i++) {
		printf("names[%d]: %s\n", i, o->names[i]);
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
					p = json_value_get_string(vv);
					if (p) printf("==> sid: %s\n", p);
				} else if (strcmp(oo->names[i],"isLogin") == 0) {
					bval = json_value_get_boolean(oo->values[i]);
					printf("isLogin: %d\n", bval);
//					if (s->isLogin) s->isLogin = 0;
//					*s->sid = 0;
				} else if (type == JSON_TYPE_OBJECT) {
					getvals(json_value_get_object(vv));
				}
			}
		}
	}
	p = json_object_dotget_string(o,"result.sid");
	if (p) {
		strncpy((char *)ctx,p,63);
		goto done;
	}

done:
	return size*nmemb;
}

void *sb_new(void *conf, void *driver, void *driver_handle) {
	sb_session_t *s;

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

	return s;
}

int sb_open(void *handle) {
	sb_session_t *s = handle;
	int r;

	dprintf(3,"s: %p\n", s);
	if (!s) return 1;
	dprintf(3,"open: %d\n", solard_check_state(s,SB_STATE_OPEN));

	r = 0;
	if (!solard_check_state(s,SB_STATE_OPEN)) {
		struct curl_slist *hs = curl_slist_append(0, "Content-Type: application/json");
		CURLcode res;
		char url[128],*j;
		json_object_t *o;

		curl_easy_setopt(s->curl, CURLOPT_VERBOSE, 1L);
		curl_easy_setopt(s->curl, CURLOPT_SSL_VERIFYPEER, 0L);
		curl_easy_setopt(s->curl, CURLOPT_HTTPHEADER, hs);
		sprintf(url,"https://%s/%s",s->endpoint,URL_LOGIN);
		printf("url: %s\n", url);
		curl_easy_setopt(s->curl, CURLOPT_URL, url);
		o = json_create_object();
		json_object_set_string(o,"right","usr");
		json_object_set_string(o,"pass",s->password);
		j = json_dumps(json_object_get_value(o),0);
		json_destroy_object(o);
		printf("j: %s\n", j);
		curl_easy_setopt(s->curl, CURLOPT_POSTFIELDS, j);
		curl_easy_setopt(s->curl, CURLOPT_WRITEFUNCTION, wrfunc);
		*s->session_id = 0;
		curl_easy_setopt(s->curl, CURLOPT_WRITEDATA, s->session_id);
		curl_easy_setopt(s->curl, CURLOPT_HTTPHEADER, hs);
	 
		/* Perform the request, res will get the return code */
		res = curl_easy_perform(s->curl);
		if(res != CURLE_OK) fprintf(stderr, "curl_easy_perform() failed: %s\n", curl_easy_strerror(res));
		free(j);
		if (!*s->session_id) {
			log_error("sb_open: error getting session_id\n");
			sprintf(s->errmsg,"error getting session_id\n");
			r = 1;
		} else {
			dprintf(0,"session_id: %s\n", s->session_id);
			solard_set_state(s,SB_STATE_OPEN);
		}
	}
	dprintf(3,"returning: %d\n", r);
	return r;
}

int sb_close(void *handle) {
	sb_session_t *s = handle;
	char url[128];
	CURLcode res;
	int r;

	dprintf(3,"s: %p\n", s);
	if (!s) return 1;
	dprintf(3,"open: %d\n", solard_check_state(s,SB_STATE_OPEN));

	r = 0;
	sprintf(url,"https://%s/%s?sid=%s",s->endpoint,URL_LOGOUT,s->session_id);
	dprintf(1,"url: %s\n", url);
	curl_easy_setopt(s->curl, CURLOPT_URL, url);
	curl_easy_setopt(s->curl, CURLOPT_POSTFIELDS, "{}");
	res = curl_easy_perform(s->curl);
	if (res != CURLE_OK) {
		log_error("sb_close: logout failed: %s\n", curl_easy_strerror(res));
		r = 1;
	}
	curl_easy_cleanup(s->curl);
	s->curl = 0;

	dprintf(3,"returning: %d\n", r);
	return r;
}

static int mkfields(sb_session_t *s) {
	json_object_t *o;
	json_array_t *a;

	o = json_create_object();
	a = json_create_array();
	json_object_set_array(o,"destDev",a);
	s->fields = json_dumps(json_object_get_value(o),0);
	if (!s->fields) {
		log_syserror("mkfields: json_dumps");
		return 1;
	}
	dprintf(0,"fields: %s\n", s->fields);
	return 0;
}

static int sb_read(void *handle, void *buf, int buflen) {
	sb_session_t *s = handle;
	char url[128];
	CURLcode res;

	/* Open if not already open */
        if (sb_driver.open(s)) return 1;

	dprintf(1,"reading...\n");
	sprintf(url,"https://%s/%s?sid=%s",s->endpoint,"dyn/getAllParamValues.json",s->session_id);
	printf("url: %s\n", url);
	curl_easy_setopt(s->curl, CURLOPT_URL, url);
	if (!s->fields && mkfields(s)) return 1;
	curl_easy_setopt(s->curl, CURLOPT_POSTFIELDS, s->fields);
	res = curl_easy_perform(s->curl);
	if (res != CURLE_OK) {
		log_error("sb_read failed: %s\n", curl_easy_strerror(res));
		sb_driver.close(s);
		return 1;
	}
//	solard_clear_state(s->ap,SOLARD_AGENT_RUNNING);
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

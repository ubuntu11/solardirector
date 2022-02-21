/*
Copyright (c) 2021, Stephen P. Shoecraft
All rights reserved.

This source code is licensed under the BSD-style license found in the
LICENSE file in the root directory of this source tree.
*/

#include "sb.h"

#define dlevel 1

#define SB_INIT_BUFSIZE 1024

char *gettag(sb_session_t *s, json_value_t *rv) {
	json_object_t *o;
	json_value_t *v;
	int tag;
	char *p;

	o = json_value_get_object(rv);
	dprintf(dlevel,"object count: %d\n", o->count);
	if (o->count < 1) return 0;
	/* The only option we support here is "tag" (number) */
	v = json_object_dotget_value(o,"tag");
	dprintf(dlevel,"v: %p\n", v);
	if (!v) return 0;
	tag = json_value_get_number(v);
	dprintf(dlevel,"tag: %d\n", tag);
	p = sb_get_string(s, tag);
	if (!p) p = "";
	return p;
}

static void getval(sb_session_t *s, sb_result_t *res, json_value_t *rv) {
	sb_value_t newval;
	json_array_t *a;
	int type;
	register int i;
	register char *p;

	dprintf(dlevel,"id: %s\n", res->obj->key);

	memset(&newval,0,sizeof(newval));

	type = json_value_get_type(rv);
	dprintf(dlevel,"type: %d (%s)\n", type, json_typestr(type));
	switch(type) {
	case JSON_TYPE_NULL:
		newval.type = DATA_TYPE_NULL;
		break;
	case JSON_TYPE_NUMBER:
	    {
		double d;

		newval.type = DATA_TYPE_DOUBLE;
		newval.data = malloc(sizeof(double));
		if (!newval.data) {
			log_syserror("getval: malloc(%d)",sizeof(double));
			return;
		}
		d = json_value_get_number(rv);
		dprintf(dlevel,"newval.d(1): %f, mult: %f\n", d, res->obj->mult);
		if (!float_equals(res->obj->mult,0.0)) d *= res->obj->mult;
		dprintf(dlevel,"newval.d(2): %f\n", d);
		*((double *)newval.data) = d;
	    }
	    break;
	case JSON_TYPE_STRING:
		newval.type = DATA_TYPE_STRING;
		p = json_value_get_string(rv);
		newval.len = strlen(p);
		newval.data = malloc(newval.len+1);
		if (!newval.data) {
			log_syserror("_getval: malloc(%d)\n", newval.len+1);
			return;
		}
		strcpy((char *)newval.data,p);
		break;
	case JSON_TYPE_ARRAY:
		a = json_value_get_array(rv);
		dprintf(dlevel,"array count: %d\n", a->count);
		if (a->count == 1 && json_value_get_type(a->items[0]) == JSON_TYPE_OBJECT) {
			newval.type = DATA_TYPE_STRING;
			p = gettag(s,a->items[0]);
			dprintf(dlevel,"p: %s\n", p);
			if (!p) return;
			newval.len = strlen(p);
			newval.data = malloc(newval.len+1);
			if (!newval.data) {
				log_syserror("_getval: malloc(%d)\n", newval.len+1);
				return;
			}
			strcpy((char *)newval.data,p);
		} else {
			newval.type = DATA_TYPE_STRING_LIST;
			newval.data = list_create();
			for(i=0; i < a->count; i++) {
				type = json_value_get_type(a->items[i]);
				dprintf(dlevel,"type: %s\n", json_typestr(type));
				switch(type) {
				case JSON_TYPE_NUMBER:
					dprintf(dlevel,"num: %f\n", json_value_get_number(a->items[i]));
					break;
				case JSON_TYPE_OBJECT:
					p = gettag(s,a->items[i]);
					dprintf(dlevel,"adding: %s\n", p);
					list_add((list)newval.data,p,strlen(p)+1);
					break;
				default:
					dprintf(dlevel,"unhandled type: %d (%s)\n", type, json_typestr(type));
					return;
				}
			}
		}
		break;
	default:
		log_error("_getval: unhandled type: %d(%s)\n", type, json_typestr(type));
		return;
		break;
	}
	list_add(res->values,&newval,sizeof(newval));
}

static void getsel(sb_session_t *s, sb_result_t *res, json_array_t *a) {
	int i;
	char *p;

	dprintf(dlevel,"count: %d\n", a->count);
	for(i=0; i < a->count; i++) {
		p = sb_get_string(s,json_value_get_number(a->items[i]));
		if (!p) continue;
		dprintf(dlevel,"select: %s\n", p);
		list_add(res->selects,p,strlen(p)+1);
	}
}

static int getobject(sb_session_t *s,json_object_t *o) {
	sb_result_t newres;
	json_value_t *v;
	json_object_t *oo;
	json_array_t *a;
	int i,j,k,type;

/*
      "6800_08822000": {
        "1": [
          {
            "validVals": [
              9439,
              9440,
              9441,
              9442,
              9443,
              9444
            ],
            "val": [
              {
                "tag": 9442
              }
            ]
          }
        ]
      },
*/
	dprintf(dlevel,"count: %d\n", o->count);
	for(i=0; i < o->count; i++) {
		/* ID */
		v = o->values[i];
		type = json_value_get_type(v);
		dprintf(dlevel,"o->names[%d]: %s, type: %d (%s)\n", i, o->names[i], type, json_typestr(type));
		if (type != JSON_TYPE_OBJECT) continue;

		/* Create a new result */
		memset(&newres,0,sizeof(newres));
		newres.obj = sb_get_object(s,o->names[i]);
		if (!newres.obj) continue;
		newres.selects = list_create();
		newres.values = list_create();

		/* "1" - array of 1 object */
		oo = json_value_get_object(v);
		if (oo->count < 1) continue;
		if (json_value_get_type(oo->values[0]) != JSON_TYPE_ARRAY) continue;

		a = json_value_get_array(oo->values[0]);
		dprintf(dlevel,"array count: %d\n", a->count);
		for(j=0; j < a->count; j++) {
			type = json_value_get_type(a->items[j]);
			dprintf(dlevel,"type: %s\n", json_typestr(type));
			switch(type) {
			case JSON_TYPE_NUMBER:
				dprintf(dlevel,"num: %f\n", json_value_get_number(a->items[j]));
				break;
			case JSON_TYPE_OBJECT:
				oo = json_value_get_object(a->items[j]);
				for(k=0; k < oo->count; k++) {
					type = json_value_get_type(oo->values[k]);
					dprintf(dlevel,"oo->names[%d]: %s, type: %d (%s)\n", k, oo->names[k], type, json_typestr(type));
					if (strcmp(oo->names[k],"val") == 0) {
						getval(s,&newres,oo->values[k]);
					} else if (strcmp(oo->names[k],"validVals") == 0 && type == JSON_TYPE_ARRAY) {
						getsel(s,&newres,json_value_get_array(oo->values[k]));
					} else if (strcmp(oo->names[k],"low") == 0 && type == JSON_TYPE_NUMBER) {
						newres.low = json_value_get_number(oo->values[k]);
					} else if (strcmp(oo->names[k],"high") == 0 && type == JSON_TYPE_NUMBER) {
						newres.high = json_value_get_number(oo->values[k]);
					}
				}
			}
		}
		list_add(s->results,&newres,sizeof(newres));
	}
	return 0;
}

static int getresults(sb_session_t *s, json_object_t *o) {
	json_value_t *v,*vv;
	json_object_t *oo;
	int i,j,type;
	bool bval;

	for(i=0; i < o->count; i++) {
		dprintf(dlevel,"names[%d]: %s\n", i, o->names[i]);
		if (strcmp(o->names[i],"result") == 0) {
			v = o->values[i];
			dprintf(dlevel,"values[%d] type: %s\n", i, json_typestr(json_value_get_type(v)));
			if (json_value_get_type(v) != JSON_TYPE_OBJECT) continue;
			oo = json_value_get_object(v);
			for(j=0; j < oo->count; j++) {
				vv = oo->values[j];
				type = json_value_get_type(vv);
				dprintf(dlevel,"names[%d]: %s, type: %d(%s)\n", j, oo->names[j], type, json_typestr(type));
				if (strcmp(oo->names[j],"sid") == 0) {
					if (type != JSON_TYPE_STRING) continue;
					strncpy(s->session_id,json_value_get_string(vv),sizeof(s->session_id)-1);
				} else if (strcmp(oo->names[i],"isLogin") == 0) {
					json_to_type(DATA_TYPE_BOOLEAN,&bval,sizeof(bval),oo->values[i]);
//					bval = json_value_get_boolean(oo->values[i]);
					dprintf(dlevel,"isLogin: %d\n", bval);
					if (!bval) *s->session_id = 0;
				} else if (type == JSON_TYPE_OBJECT) {
					getobject(s,json_value_get_object(vv));
				}
			}
		} else if (strcmp(o->names[i],"err") == 0) {
			oo = json_value_get_object(v);
			if (oo->count < 1) return 1;
			if (json_value_get_type(oo->values[0]) == JSON_TYPE_NUMBER) {
				s->errcode = json_value_get_number(oo->values[0]);
				dprintf(dlevel,"errcode: %d\n", s->errcode);
			}
		}
	}

	return 0;
}

static size_t wrfunc(void *ptr, size_t size, size_t nmemb, void *ctx) {
	sb_session_t *s = ctx;
	int bytes,newidx;

//	printf("%s\n",(char *)ptr);

	bytes = size*nmemb;
//	dprintf(dlevel,"bytes: %d, bufidx: %d, bufsize: %d\n", bytes, s->bufidx, s->bufsize);
	newidx = s->bufidx + bytes;
//	dprintf(dlevel,"newidx: %d\n", newidx);
	if (newidx > s->bufsize) {
		char *newbuf;

		newbuf = realloc(s->buffer,newidx);
		if (!newbuf) {
			log_syserror("wrfunc: realloc(buffer,%d)",newidx);
			return 0;
		}
		s->buffer = newbuf;
		s->bufsize = newidx;
	}
	strcpy(s->buffer + s->bufidx,ptr);
	s->bufidx = newidx;

	return bytes;
}

int sb_request(sb_session_t *s, char *func, char *fields) {
	char url[128],*p;
	CURLcode res;

	dprintf(dlevel,"func: %s, fields: %s\n", func, fields);
	dprintf(dlevel,"endpoint: %s\n", s->endpoint);
	if (!*s->endpoint) return 1;

	sb_destroy_results(s);
	s->results = list_create();
	s->bufidx = 0;
	if (s->data) {
		json_destroy_value(s->data);
		s->data = 0;
	}

	p = url;
	if (strncmp(s->endpoint,"http",4) != 0) p += sprintf(p,"https://");
	p += sprintf(p,"%s/%s",s->endpoint,func);
	if (*s->session_id) p += sprintf(p,"?sid=%s",s->session_id);
	dprintf(dlevel,"url: %s\n", url);
	curl_easy_setopt(s->curl, CURLOPT_URL, url);
	if (fields) curl_easy_setopt(s->curl, CURLOPT_POSTFIELDS, fields);
	else curl_easy_setopt(s->curl, CURLOPT_POSTFIELDS, "{}");
	res = curl_easy_perform(s->curl);
	if (res != CURLE_OK) {
		log_error("sb_request failed: %s\n", curl_easy_strerror(res));
		sb_driver.close(s);
		return 1;
	}
#if 0
	if (s->bufidx) {
		register char *p,*p2;
		p2 = s->buffer;
		for(p = s->buffer; *p; p++) {
			if (*p == '\n' || *p == '\r') continue;
			else if (*p < 0 || *p > 127) continue;
			*p2++ = *p;
		}
	}
#endif

	/* Parse the buffer */
	dprintf(1,"bufidx: %d\n", s->bufidx);
	s->data = json_parse(s->buffer);
	dprintf(dlevel,"data: %p\n", s->data);
	/* May not be json data? */
	if (!s->data) return 0;

	/* If it's results, process it */
	dprintf(dlevel,"type: %s\n", json_typestr(json_value_get_type(s->data)));
	if (json_value_get_type(s->data) == JSON_TYPE_OBJECT) {
		json_object_t *o;
		
		o = json_value_get_object(s->data);
		dprintf(dlevel,"count: %d, names[0]: %s\n", o->count, o->names[0]);
		if (o->count == 1 && strcmp(o->names[0],"result") == 0)
			return getresults(s,json_value_get_object(s->data));
	}

	return 0;
}

void *sb_new(void *driver, void *driver_handle) {
	sb_session_t *s;
	struct curl_slist *hs = curl_slist_append(0, "Content-Type: application/json");

	s = calloc(1,sizeof(*s));
	if (!s) {
		log_syserror("sb_new: calloc");
		return 0;
	}
	s->curl = curl_easy_init();
	if (!s->curl) {
		log_syserror("sb_new: curl_init");
		free(s);
		return 0;
	}
	s->buffer = malloc(SB_INIT_BUFSIZE);
	if (!s->buffer) {
		log_syserror("sb_new: malloc(%d)",SB_INIT_BUFSIZE);
		curl_easy_cleanup(s->curl);
		free(s);
		return 0;
	}
	s->bufsize = SB_INIT_BUFSIZE;
	s->bufidx = 0;
	s->results = list_create();
//	curl_easy_setopt(s->curl, CURLOPT_VERBOSE, 1L);
	curl_easy_setopt(s->curl, CURLOPT_SSL_VERIFYPEER, 0L);
	curl_easy_setopt(s->curl, CURLOPT_HTTPHEADER, hs);
	curl_easy_setopt(s->curl, CURLOPT_WRITEFUNCTION, wrfunc);
	curl_easy_setopt(s->curl, CURLOPT_WRITEDATA, s);

	return s;
}

static int sb_destroy(void *handle) {
	sb_session_t *s = handle;

	list_destroy(s->results);
	free(s->buffer);
	curl_easy_cleanup(s->curl);
	free(s);
	return 0;
}

int sb_open(void *handle) {
	sb_session_t *s = handle;

	if (!s) return 1;

	dprintf(dlevel,"open: %d\n", solard_check_state(s,SB_STATE_OPEN));
	if (solard_check_state(s,SB_STATE_OPEN)) return 0;

	/* Create the login info */
	if (!s->login_fields) {
		json_object_t *o;

		o = json_create_object();
		json_object_set_string(o,"right",s->user);
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

	dprintf(dlevel,"session_id: %s\n", s->session_id);
	solard_set_state(s,SB_STATE_OPEN);
	s->connected = 1;
	return 0;
}

int sb_close(void *handle) {
	sb_session_t *s = handle;

	if (!s) return 1;
	dprintf(dlevel,"open: %d\n", solard_check_state(s,SB_STATE_OPEN));
	if (!solard_check_state(s,SB_STATE_OPEN)) return 0;

	sb_request(s,SB_LOGOUT,"{}");
	s->connected = 0;
	return 0;
}

static int sb_read(void *handle, uint32_t *control, void *buf, int buflen) {
	sb_session_t *s = handle;

	/* Open if not already open */
        if (sb_driver.open(s)) return 1;

	if (!s->read_fields) s->read_fields = sb_mkfields(0,0);

	if (sb_request(s,SB_ALLVAL,s->read_fields)) return 1;
	return 0;
}

solard_driver_t sb_driver = {
	"sb",
	sb_new,				/* New */
	sb_destroy,			/* Destroy */
	sb_open,			/* Open */
	sb_close,			/* Close */
	sb_read,			/* Read */
	0,				/* Write */
	sb_config			/* Config */
};

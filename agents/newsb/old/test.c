
#include "common.h"
#include <curl/curl.h>
#include "sma_strings.h"
#include "sma_objects.h"
 
#define SB_ID_SIZE 32
#define SB_SID_SIZE 32

struct sbval {
	char id[SB_ID_SIZE];
	int type;
	list values;
};
typedef struct sbval sbval_t;

struct sb_session {
	CURL *curl;
	char sid[SB_SID_SIZE];
	list values;
};

char *ip = "192.168.1.153";
//char *ip = "192.168.1.175";
//char *ip = "192.168.1.182";
char *password = "Bgt56yhn!";

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
	char tagstr[16];

	dprintf(1,"tag: %d\n", tag);
	sprintf(tagstr,"%d",tag);
	dprintf(1,"tagstr: %s\n", tagstr);

	for(s=sma_strings; s->tag; s++) {
		if (strcmp(s->tag,tagstr) == 0) {
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

#if 0
static int getobjectname(char *dest, int destsize, char *id) {
	sma_object_t *o;
	char temp[16], *idstr, *p;

	o = getsmaobject(id);
	if (!o) return 1;
	p = getsmastring(o->tag);
	if (!p) return 1;
	strncpy(dest,p,destsize);
	return 0;
}
#endif

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

void _getval(char *id, char *w, json_value_t *rv) {
	struct sbval val;
	json_value_t *v;
	json_object_t *o;
	json_array_t *a;
	int type;
	double dval;
	char *p;
	char path[256],value[64];

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
		if (a->count != 1) {
			dprintf(0,"====> COUNT: %d\n", a->count);
			exit(0);
		}
		if (a->count < 1) return;
		if (json_value_get_type(a->items[0]) != JSON_TYPE_OBJECT) return;
		o = json_value_get_object(a->items[0]);
		dprintf(1,"object count: %d\n", o->count);
		if (o->count < 1) return;
		/* The only option we support here is "tag" (number) */
		v = json_object_dotget_value(o,"tag");
		if (!v) return;
		dval = json_value_get_number(v);
		dprintf(1,"dval: %f\n", dval);
		p = getsmastring((int)dval);
		if (!p) return;
		strncpy(value,p,sizeof(value)-1);
		break;
	default:
		dprintf(0,"unhandled type: %d(%s)\n", type, json_typestr(type));
		return;
		break;
	}
	printf("(%s)%s%s: %s\n", id, path, w, value);
}

int getvals(json_object_t *o) {
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

size_t wrfunc(void *ptr, size_t size, size_t nmemb, void *ctx) {
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

int main(void) {
	CURL *curl;
	CURLcode res;
	json_object_t *o;
	json_array_t *a;
	char *j;
	char session_id[64];
	struct curl_slist *hs=NULL;
	char url[128];
 
#if 0
curl -H "Content-Type: application/json, Accept-Charset: UTF-8" -X POST -d '{"pass" : "password", "right" : "usr"}' http://pv/dyn/login.json

You get a session ID: {"result":{"sid":"NHIsYKZaBmBa7KJG"}}

curl -H "Content-Type: application/json, Accept-Charset: UTF-8" -X POST -d '{"destDev":[],"keys":["6400_00260100","6400_00262200","6100_40263F00"]}' http://pv/dyn/getValues.json?sid=NHIsYKZaBmBa7KJG

{"result":{"0156-76BCFF**":{"6400_00260100":{"1":[{"val":12020721}]},"6400_00262200":{"1":[{"val":17570}]},"6100_40263F00":{"1":[{"val":831}]}}}}
        params = {
            "data": json.dumps(payload),
            "headers": {"content-type": "application/json"},
            "params": {"sid": self.sma_sid} if self.sma_sid else None,
        }
#endif

	hs = curl_slist_append(hs, "Content-Type: application/json");

	/* In windows, this will init the winsock stuff */
	curl_global_init(CURL_GLOBAL_ALL);
 
	curl = curl_easy_init();
	if (curl) {
		curl_easy_setopt(curl, CURLOPT_VERBOSE, 1L);
		curl_easy_setopt(curl, CURLOPT_SSL_VERIFYPEER, 0L);
		curl_easy_setopt(curl, CURLOPT_HTTPHEADER, hs);
		sprintf(url,"https://%s/%s",ip,URL_LOGIN);
		printf("url: %s\n", url);
		curl_easy_setopt(curl, CURLOPT_URL, url);
		o = json_create_object();
		json_object_set_string(o,"right","usr");
		json_object_set_string(o,"pass",password);
		j = json_dumps(json_object_get_value(o),0);
		json_destroy_object(o);
		printf("j: %s\n", j);
		curl_easy_setopt(curl, CURLOPT_POSTFIELDS, j);
		curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, wrfunc);
		*session_id = 0;
		curl_easy_setopt(curl, CURLOPT_WRITEDATA, session_id);
		curl_easy_setopt(curl, CURLOPT_HTTPHEADER, hs);
	 
		/* Perform the request, res will get the return code */
		res = curl_easy_perform(curl);
		if(res != CURLE_OK) fprintf(stderr, "curl_easy_perform() failed: %s\n", curl_easy_strerror(res));
		free(j);
		if (!*session_id) {
			printf("error getting session_id\n");
			return 1;
		}
		printf("session_id: %s\n", session_id);

//		sprintf(url,"https://%s/%s?sid=%s",ip,URL_VALUES,session_id);
//https://192.168.1.175/dyn/getAllOnlValues.json?sid=qNiEitggWL5gRuAk
		sprintf(url,"https://%s/%s?sid=%s",ip,"dyn/getAllOnlValues.json",session_id);
		printf("url: %s\n", url);
		curl_easy_setopt(curl, CURLOPT_URL, url);
#if 1
//		curl_easy_setopt(curl, CURLOPT_POSTFIELDS, "{\"destDev\":[],\"keys\":[\"6400_00260100\"]}");
		o = json_create_object();
		a = json_create_array();
		json_object_set_array(o,"destDev",a);
		j = json_dumps(json_object_get_value(o),0);
		json_destroy_object(o);
		printf("==> j: %s\n", j);
		curl_easy_setopt(curl, CURLOPT_POSTFIELDS, j);
#else
		curl_easy_setopt(curl, CURLOPT_POSTFIELDS, "{\"destDev\":[]}");
#endif
		res = curl_easy_perform(curl);
		if (res != CURLE_OK) fprintf(stderr, "curl_easy_perform() failed: %s\n", curl_easy_strerror(res));
		free(j);

		sprintf(url,"https://%s/%s?sid=%s",ip,URL_LOGOUT,session_id);
		printf("url: %s\n", url);
		curl_easy_setopt(curl, CURLOPT_URL, url);
		curl_easy_setopt(curl, CURLOPT_POSTFIELDS, "{}");
		res = curl_easy_perform(curl);
		if (res != CURLE_OK) fprintf(stderr, "curl_easy_perform() failed: %s\n", curl_easy_strerror(res));

		curl_easy_cleanup(curl);
	}

	curl_global_cleanup();
	return 0;
}

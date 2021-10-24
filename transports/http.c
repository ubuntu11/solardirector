
#ifndef __WIN32
/*
Copyright (c) 2021, Stephen P. Shoecraft
All rights reserved.

This source code is licensed under the BSD-style license found in the
LICENSE file in the root directory of this source tree.
*/

#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <fcntl.h>
#include <ctype.h>
#include <netdb.h>
#include <curl/curl.h>
#include "transports.h"

#define DEFAULT_PORT 23

struct http_session {
	int fd;
	char target[SOLARD_TARGET_LEN+1];
	char username[32];
	char password[64];
	int port;
	CURL *curl;
	CURLcode res;
};
typedef struct http_session http_session_t;

static int curl_init = 0;

static void *http_new(void *conf, void *target, void *topts) {
	http_session_t *s;
	char temp[128],*p;
	int i;

	if (!curl_init) {
		/* In windows, this will init the winsock stuff */
		curl_global_init(CURL_GLOBAL_ALL);
		curl_init = 1;
	}

	s = calloc(1,sizeof(*s));
	if (!s) {
		perror("http_new: malloc");
		return 0;
	}
	s->fd = -1;

	p = strchr((char *)target,':');
	if (p) *p = 0;
	strncat(s->target,(char *)target,SOLARD_TARGET_LEN);
	if (p) {
		p++;
		s->port = atoi(p);
	}
	if (!s->port) s->port = DEFAULT_PORT;
	dprintf(5,"target: %s, port: %d\n", s->target, s->port);

	dprintf(5,"target: %s, topts: %s\n", s->target, (char *)topts);
	for(i=0; i < 5; i++) {
		strcpy(temp,strele(i,",",(char *)topts));
		trim(temp);
		dprintf(1,"temp: %s\n", temp);
		if (!strlen(temp)) break;
		if (strncmp(temp,"user=",5)==0)
			strcpy(s->username,strele(1,"=",temp));
		else if (strncmp(temp,"username=",9)==0)
			strcpy(s->username,strele(1,"=",temp));
		else if (strncmp(temp,"pass=",5)==0)
			strcpy(s->password,strele(1,"=",temp));
		else if (strncmp(temp,"password=",9)==0)
			strcpy(s->password,strele(1,"=",temp));
	}
	dprintf(1,"username: %s, password: %s\n", s->username, s->password);

	/* get a curl handle */
	s->curl = curl_easy_init();
	if(!s->curl) {
		free(s);
		return 0;
	}
	curl_easy_setopt(s->curl, CURLOPT_URL, s->target);

	return s;
}

static int http_open(void *handle) {
	return 0;
}

static int http_read(void *handle, void *buf, int buflen) {
	http_session_t *s = handle;
//	int retcode = 0;
//	CURLcode res = CURLE_FAILED_INIT;
	char errbuf[CURL_ERROR_SIZE] = { 0, };
	struct curl_slist *headers = NULL;
	char agent[1024] = { 0, };
	char *url = "none";

//	https://192.168.1.5:443/dyn/login.json
#if 0
                                'User-Agent': 'Mozilla/5.0 (Windows NT 6.0; WOW64; rv:24.0) Gecko/20100101 Firefox/24.0',
                                'Accept': 'application/json, text/plain, */*',
                                'Accept-Language': 'fr,fr-FR;q=0.8,en-US;q=0.5,en;q=0.3',
                                'Accept-Encoding': 'gzip, deflate',
                                'Referer': self.__url + '/',
                                'Content-Type': 'application/json',
                                'Content-Length': str(len(params)),
#endif


	snprintf(agent, sizeof(agent), "libcurl/%s", curl_version_info(CURLVERSION_NOW)->version);
	agent[sizeof(agent) - 1] = 0;
	dprintf(1,"agent: %s\n", agent);
	curl_easy_setopt(s->curl, CURLOPT_USERAGENT, agent);

	headers = curl_slist_append(headers, "Expect:");
	headers = curl_slist_append(headers, "Content-Type: application/json");
	curl_easy_setopt(s->curl, CURLOPT_HTTPHEADER, headers);

//	curl_easy_setopt(s->curl, CURLOPT_POSTFIELDS, json);
//	curl_easy_setopt(s->curl, CURLOPT_POSTFIELDSIZE, -1L);

	/* This is a test server, it fakes a reply as if the json object were created */
	curl_easy_setopt(s->curl, CURLOPT_URL, url);

	curl_easy_setopt(s->curl, CURLOPT_VERBOSE, 1L);
	curl_easy_setopt(s->curl, CURLOPT_ERRORBUFFER, errbuf);

	return 1;
}

static int http_close(void *handle) {
	http_session_t *s = handle;

	/* Is it open */
	if (s->fd >= 0) {
		dprintf(5,"closing...\n");
		close(s->fd);
		s->fd = -1;
	}
	return 0;
}

static int http_config(void *h, int func, ...) {
	va_list ap;
	int r;

	r = 1;
	va_start(ap,func);
	switch(func) {
	default:
		dprintf(1,"error: unhandled func: %d\n", func);
		break;
	}
	return r;
}

solard_driver_t http_driver = {
	SOLARD_DRIVER_TRANSPORT,
	"ip",
	http_new,
	http_open,
	http_close,
	http_read,
	0,
	http_config
};
#endif

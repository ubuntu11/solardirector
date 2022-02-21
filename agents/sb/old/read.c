
/*
Copyright (c) 2021, Stephen P. Shoecraft
All rights reserved.

This source code is licensed under the BSD-style license found in the
LICENSE file in the root directory of this source tree.
*/

#include "sb.h"

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

int sb_read(void *handle, void *buf, int buflen) {
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
//	curl_easy_setopt(s->curl, CURLOPT_POSTFIELDS, "{\"destDev\":[]}");
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

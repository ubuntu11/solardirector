
#include "sb.h"

char *sb_agent_version_string = "1.0-TEST";

char *spot_order[] = {
	/* Status */
	/* Status -> Operation */
	"6100_00416600",
	"6180_08214800",
	"6180_08412800",
	"6180_08416400",
	"6180_08416500",
	/* Status -> Operation -> Device status */
	"6100_00411E00",
	"6100_00411F00",
	"6100_00412000",
};

void dispval(sb_session_t *s, sb_result_t *res, sb_value_t *vp, char *w) {
	char path[256], value[512], *unit;
	char out[1024], *op;

	if (sb_get_object_path(s,path,sizeof(path)-1,res->obj)) return;
	unit = (res->obj->unit ? res->obj->unit : "");
//	strcat(path," -> ");
//	strcat(path,res->obj->label);
	op = out;
	op += sprintf(op,"[%s] %s%s: ", res->obj->key, path, w);
	conv_type(DATA_TYPE_STRING,value,sizeof(value)-1,vp->type,vp->data,vp->len);
	op += sprintf(op, "%s %s", value, unit);
	printf("%s\n", out);
}

void dispres(sb_session_t *s, sb_result_t *res) {
	sb_value_t *vp;
	int count,i;
	char w[5];

	count = list_count(res->values);
	dprintf(5,"result: key: %s, low: %d, high: %d, selects: %d, values: %d\n", res->obj->key, res->low, res->high, list_count(res->selects), count);
	i = 0;
	while((vp = list_get_next(res->values)) != 0) {
		if (count > 1) sprintf(w," [%c]",'A' + i++);
		else *w = 0;
		dispval(s,res,vp,w);
	}
}

void display(sb_session_t *s, char **keys) {
	register char **kp;
	sb_result_t *rp;

	for(kp = keys; *kp; kp++) {
//		printf("kp: %s\n", *kp);
		list_reset(s->results);
		while((rp = list_get_next(s->results)) != 0) {
//			printf("id: %s\n", rp->obj->key);
			if (strcmp(rp->obj->key,*kp) == 0) {
//				printf("match!\n");
				dispres(s,rp);
				list_delete(s->results,rp);
				break;
			}
		}
	}
	printf("not found:\n");
	list_reset(s->results);
	while((rp = list_get_next(s->results)) != 0) printf("%s\n",rp->obj->key);
}

int main(int argc, char **argv) {
	sb_session_t *s;
	char *fields;
	sb_result_t *rp;
        char *args[] = { "sb", "-d", "0", "-c", "sbtest.conf" };
        argc = sizeof(args)/sizeof(char *);
        argv = args;

	s = sb_driver.new(0,0);
	if (!s) return 1;

	if (sb_agent_init(s,argc,args)) return 1;

	if (sb_driver.open(s)) return 1;
	sb_get_info(s);

//	fields = sb_mkfields(instant_keys,instant_keys_count);
	fields = sb_mkfields(0,0);
	printf("fields: %s\n", fields);
//	if (sb_request(s,SB_ALLPAR,"{\"destDev\":[]}")) return 1;
	if (sb_request(s,SB_ALLVAL,"{\"destDev\":[]}")) return 1;
//	if (sb_request(s,SB_GETVAL,fields)) return 1;
	dprintf(0,"result count: %d\n", list_count(s->results));
//	display(s,instant_keys);
	list_reset(s->results);
	while((rp = list_get_next(s->results)) != 0) dispres(s,rp);
	sb_driver.close(s);
	return 0;
}

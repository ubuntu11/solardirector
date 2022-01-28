
#include "sb.c"
#include "config.c"

int main(void) {
	sb_session_t *s;
	char path[256], value[1024], *unit, *sp, *p;
	sb_value_t *vp;
	int i;

	s = sb_driver.new(0,0,0);
	if (!s) return 1;

debug = 1;
//	if (count > 1) sprintf(w," [%c]",'A' + i++);
//	else *w = 0;

        strcpy(s->endpoint,"192.168.1.153");
//        strcpy(s->endpoint,"192.168.1.175");
        strcpy(s->password,"Bgt56yhn!");

	if (sb_driver.open(s)) return 1;
	s->data = list_create();
	if (sb_request(s,SB_ALLVAL,"{\"destDev\":[]}")) {
		list_destroy(s->data);
		return 1;
	}
	list_reset(s->data);
	while((vp = list_get_next(s->data)) != 0) {
		dprintf(2,"w: %s\n", vp->w);
		if (getsmaobjectpath(path,sizeof(path)-1,vp->o)) continue;
		unit = (vp->o->unit ? getsmastring(vp->o->unit) : "");
		switch(vp->type) {
		case DATA_TYPE_NULL:
			*value = 0;
			printf("(%s)%s%s: %s %s\n", vp->o->id, path, vp->w, value, unit);
			break;
		case DATA_TYPE_DOUBLE:
			sprintf(value,"%f",vp->d);
			printf("(%s)%s%s: %s %s\n", vp->o->id, path, vp->w, value, unit);
			break;
		case DATA_TYPE_STRING:
			strncpy(value,vp->s,sizeof(value)-1);
			printf("(%s)%s%s: %s %s\n", vp->o->id, path, vp->w, value, unit);
			break;
		case DATA_TYPE_STRING_LIST:
			i = 0;
			sp = value;
			list_reset(vp->l);
			while((p = list_get_next(vp->l)) != 0) {
				if (i++) sp += sprintf(sp,",");
				sp += sprintf(sp,"%s",p);
			}
			printf("(%s)%s%s: %s %s\n", vp->o->id, path, vp->w, value, unit);
			break;
		}
	}
	sb_driver.close(s);
	return 0;
}

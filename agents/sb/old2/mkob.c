
#include <string.h>
#include <stdio.h>
#include "sma_objects.c"
#include "sma_strings.c"

int _intcmp(void *i1, void *i2) {
//	int *ip1 = i1, *ip2 = i2;
//	int n1 = *ip1, n2 = *ip2;

	if (*(int *)i1 < *(int *)i2)
		return -1;
	else if (*(int *)i1 > *(int *)i2)
		return 1;
	else
		return 0;
}

void lookfor(int *hier, int depth) {
	sma_object_t *o;
	char out[1024], *p;
	register int i;
	int found;

#if 0
	p = out;
	p += sprintf(p,"[ ");
	for(i=0; i < depth; i++) {
		if (i) p += sprintf(p,",");
		p += sprintf(p,"%d",hier[i]);
	}
	p += sprintf(p," ]");
	printf("looking for: %s\n", out);
#endif
	bindump("hier",&hier,sizeof(int)*8);
	for(o=sma_objects; o->id; o++) {
		found = 1;
		if (hier[0] == 834 && hier[1] == 67) bindump("o->hier",o->hier,sizeof(o->hier));
		for(i = 0; i < 8; i++) {
//			if (hier[0] == 834 && hier[1] == 67) printf("hier[%d]: %d\n", i, hier[i]);
			if (!o->hier[i]) break;
			if (o->hier[i] != hier[i]) {
				found = 0;
				break;
			}
		}
		if (found) {
			printf("found: %s\n", o->id);
			p = out;
			p += sprintf(p,"[ ");
			for(i=0; i < depth; i++) {
				if (i) p += sprintf(p,",");
				p += sprintf(p,"%d",hier[i]);
			}
			p += sprintf(p," ]");
			printf("hier: %s\n", out);
		}
	}
}

int main(void) {
	sma_object_t *o, *oo;
	list *ltmp;
	int *tp, tag,found;
	int *ti;
	int i,j,max_depth;
	int *counts, **tags, *indexes;

	i = 0;
	for(o=sma_objects; o->id; o++) i++;
	printf("i: %d\n", i);
	return 0;

	/* Get max depth */
	max_depth = 0;
	for(o=sma_objects; o->id; o++) {
		for(i=0; i < 8; i++) {
			if (!o->hier[i]) break;
		}
		if (i > max_depth) max_depth = i;
	}
	printf("max_depth: %d\n", max_depth);
	ltmp = malloc(max_depth*sizeof(list));
	counts = malloc(max_depth*sizeof(int));
	tags = malloc(max_depth*sizeof(int *));
	indexes = malloc(max_depth*sizeof(int));
	for(i=0; i < max_depth; i++) {
		ltmp[i] = list_create();
		for(o=sma_objects; o->id; o++) {
			if (!o->hier[i]) continue;
			tag = o->hier[i];
			found = 0;
			list_reset(ltmp[i]);
			while((tp = list_get_next(ltmp[i])) != 0) {
				if (*tp == tag) {
					found = 1;
					break;
				}
			}
			if (!found) list_add(ltmp[i],&tag,sizeof(tag));
		}
	}
	/* Get the counts for each level */
	printf("counts:\n");
	for(i=0; i < max_depth; i++) {
		counts[i] = list_count(ltmp[i]);
		printf("tags[%d]: %d\n", i, counts[i]);
		tags[i] = malloc(counts[i]*sizeof(int));
	}
	/* Sort the levels */
	for(i=0; i < max_depth; i++) list_sort(ltmp[i],_intcmp,0);
	/* Fill the arrays */
	for(i=0; i < max_depth; i++) {
		indexes[i] = 0;
		list_reset(ltmp[i]);
		while((tp = list_get_next(ltmp[i])) != 0) {
			if (!*tp) continue;
			tags[i][indexes[i]++] = *tp;
		}
	}
	{
		int a,b,c,d,e;
		int hier[8];
		for(a=0; a < counts[0]; a++) {
			memset(&hier,0,sizeof(hier));
			hier[0] = tags[0][a];
			lookfor(hier,1);
			for(b=0; b < counts[1]; b++) {
				hier[1] = tags[1][b];
				lookfor(hier,2);
				for(c=0; c < counts[2]; c++) {
					hier[2] = tags[2][c];
					lookfor(hier,3);
					for(d=0; d < counts[3]; d++) {
						hier[3] = tags[3][d];
						lookfor(hier,4);
						for(e=0; e < counts[4]; e++) {
							hier[4] = tags[4][e];
							lookfor(hier,5);
						}
					}
				}
			}
		}
	}
#if 0
		for(i=0; i < max_depth; i++) indexes[i] = 0;
		while(1) {
			for(i=0; i < max_depth; i++) {
				printf("tag[%d][%d]: %d\n", i, indexes[i], tags[i][indexes[i]]);
				tag = tags[i][indexes[i]++];
				if (indexes[i] == 
			}
		}
	
		for(j=0; j < counts[i]; j++) {
			for(o=sma_objects; o->id; o++) {
				if (tags[i][indexes[i]] = o->hier[i];
//			p += sprintf(p,"%s", getsmastring(tags[i][j]));
		}
#endif
	return 0;
}

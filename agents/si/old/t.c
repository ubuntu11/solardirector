#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <time.h>

#define SI_MAX_BA 10

float ba[SI_MAX_BA];
int bastart,baend;

void doit(float a) {
	float amps;
	int i;

	printf("a: %f, baend: %d, bastart: %d\n", a, baend, bastart);
	ba[baend++] = a;
	if (baend >= SI_MAX_BA) baend = 0;
	if (baend == bastart) {
		amps = 0;
		i=bastart+1;
		while(1) {
			if (i >= SI_MAX_BA) i = 0;
			if (i == baend) break;
			amps += ba[i];
//			printf("amps(%d): %f\n", i, amps);
			i++;
		}
		printf("amps: %f, avg: %f\n", amps, amps / SI_MAX_BA);
		bastart++;
		if (bastart >= SI_MAX_BA) bastart = 0;
	}
//	for(i=0; i < SI_MAX_BA; i++) printf("ba[%d]: %f\n", i, ba[i]);
}

int main(void) {
	int i;
	memset(&ba,0,sizeof(ba));
	bastart = baend = 0;
	for(i=0; i < 20; i++) doit((float)rand()/(float)(RAND_MAX/50.0));
}

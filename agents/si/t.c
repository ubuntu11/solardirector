#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <time.h>

#define SI_MAX_BA 10

float ba[SI_MAX_BA];
int baidx,bafull;

void doit(float a) {
	float amps;
	int i,count;

//	printf("a: %f, baidx: %d, bafull: %d\n", a, baidx, bafull);
	ba[baidx++] = a;
	if (baidx == SI_MAX_BA) {
		baidx = 0;
		bafull = 1;
	}
	if (bafull) {
		amps = 0;
		count = 0;
		for(i=0; i < SI_MAX_BA; i++) {
//			printf("ba[%d]: %f\n", i, ba[i]);
			amps += ba[i];
		}
		printf("amps: %f, count: %d, avg: %f\n", amps, SI_MAX_BA, amps / SI_MAX_BA);
	}
}

int main(void) {
	int i;
	memset(&ba,0,sizeof(ba));
	baidx = bafull = 0;
	srand(time(0));
	for(i=0; i < 1000; i++) doit((float)rand()/(float)(RAND_MAX/50.0));
}

/* Example using log10 by TechOnTheNet.com */

#include <stdio.h>
#include <math.h>

int main(int argc, const char * argv[])
{
	double v,l;

	for(v=100.0; v >= 0.0; v -= 1) {
		if (v >= 60)
			l = log10(v/10);
		else
			l = log10((v/2)/10);
		printf("v: %f, l: %f, p: %f\n", v, l, l*100);
	}

    return 0;
}

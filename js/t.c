#include <limits.h>
#include <stdio.h>

int main()
{
     printf("%d bits\n", (int)(CHAR_BIT * sizeof(void *)));
     return 0;
}

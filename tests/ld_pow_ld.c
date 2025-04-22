#include <stdio.h>
#include <math.h>

int main(void) {
	long double x = 0.75L;  /* Must be in range: [-1.0, +1.0) */
	long double result;

	/* f2xm1: 2^x - 1 */
	result = powl(2.0L, x) - 1.0L;

	printf("f2xm1(%Lf) = %Lf\n", x, result);
	return 0;
}

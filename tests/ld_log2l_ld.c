#include <stdio.h>
#include <math.h>

int main(void) {
	long double st1 = 10.1L;
	long double st0 = 2000.1L;
	long double result;

	/* fyl2x: ST(1) * log2(ST(0)) */
	result = st1 * log2l(st0);

	printf("fyl2x(%Lf, %Lf) = %Lf\n", st1, st0, result);
	return 0;
}

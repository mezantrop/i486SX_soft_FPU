#include <stdio.h>
#include <math.h>

int main(void) {
	long double st1 = 10.1L;
	long double st0 = 1.5L;
	long double result;

	result = st1 * log2l(st0);

	printf("st1 = %Lf\n", st1);
	printf("st0 = %Lf\n", st0);
	printf("log2(st0) = %Lf\n", log2l(st0));
	printf("fyl2x(st1, st0) = %Lf\n", result);

	return 0;
}
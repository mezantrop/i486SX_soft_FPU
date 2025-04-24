#include <stdio.h>
#include <math.h>

int main(void) {
	long double x = 2000.1L;
	long double r = log2l(x);

	printf("log2l(%Lf) = %Lf\n", x, r);
	return 0;
}

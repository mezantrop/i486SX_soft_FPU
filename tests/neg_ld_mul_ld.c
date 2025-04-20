#include <stdio.h>

int main(void) {
	long double n1 = -2000.1;
	long double n2 = 10.1;
	long double result = 0;

	result = n1 * n2;

	printf("%Lf\n", result);
	return 0;
}

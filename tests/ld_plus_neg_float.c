#include <stdio.h>
#include <stdlib.h>

/*
long double to_type(long double value, char t) {
	switch (t) {
		case 'c': return (char)value;
		case 's': return (short)value;
		case 'i': return (int)value;
		case 'l': return (long)value;
		case 'f': return (float)value;
		case 'd': return (double)value;
		case 'L': return (long double)value;
		default:  return 0;
	}
}
*/

int main(void) {
	long double n1 = 4.000002L;
	long double n2 = -1.000001L;
	long double result = 0;

	result = (long double)n1 + (float)n2;

	printf("%Lf\n", result);
	return 0;
}

// EXPECTED RESULT: 3.0000010000

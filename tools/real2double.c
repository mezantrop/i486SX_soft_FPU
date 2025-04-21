#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>

typedef union {
	double d;
	struct {
		uint32_t low;   // младшие 32 бита
		uint32_t high;  // старшие 32 бита
	} parts;
} ieee_double_union;

int main(int argc, char *argv[]) {
	if (argc != 2) {
		fprintf(stderr, "Usage: %s <decimal_float>\n", argv[0]);
		return 1;
	}

	double val = strtod(argv[1], NULL);

	ieee_double_union u;
	u.d = val;

	printf("double: 0x%08X 0x%08X\n", u.parts.high, u.parts.low);

	return 0;
}

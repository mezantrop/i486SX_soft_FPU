#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>

typedef union {
	long double ld;
	struct {
		uint64_t fraction : 63;
		uint64_t integer_bit : 1;
		uint64_t exponent : 15;
		uint64_t sign : 1;
	} parts __attribute__((packed));
} long_double_union;

void print_long_double(uint16_t exponent, uint64_t fraction) {
	long_double_union ld_union;

	ld_union.parts.sign = exponent >> 15;
	ld_union.parts.exponent = exponent & 0x7FFF;
	ld_union.parts.fraction = fraction;
	ld_union.parts.integer_bit = 1;

	printf("long double value: %.19Lf\n", ld_union.ld);
}

int main(int argc, char *argv[]) {
	if (argc != 3) {
		fprintf(stderr, "Usage: %s <exponent (in hex)> <fraction (in hex)>\n", argv[0]);
		return 1;
	}

	uint16_t exponent = (uint16_t)strtol(argv[1], NULL, 16);
	uint64_t fraction = strtoull(argv[2], NULL, 16);

	print_long_double(exponent, fraction);

	return 0;
}

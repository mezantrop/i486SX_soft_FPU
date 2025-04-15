#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>

typedef struct {
	uint32_t a; // low bits
	uint32_t b; // high bits
} long_real;

typedef struct {
	uint32_t a;        // lower significand bits
	uint32_t b;        // upper significand bits
	uint16_t exponent; // 15-bit exponent + sign
} temp_real;

void long_to_temp(const long_real *a, temp_real *b) {
	if (!a->a && !(a->b & 0x7FFFFFFF)) {
		b->a = b->b = 0;
		b->exponent = (a->b & 0x80000000) ? 0x8000 : 0;
		return;
	}

	int unbiased_exp = (a->b >> 20) & 0x7FF;
	int exp = unbiased_exp - 1023 + 16383;

	if (a->b & 0x80000000)
		exp |= 0x8000;

	b->exponent = (uint16_t)exp;

	b->b = 0x80000000 | ((a->b & 0x000FFFFF) << 11) | (a->a >> 21);
	b->a = a->a << 11;
}

int main(int argc, char *argv[]) {
	if (argc != 3) {
		fprintf(stderr, "Usage: %s <high32> <low32>\n", argv[0]);
		return 1;
	}

	long_real input;
	temp_real output;

	input.b = (uint32_t)strtoul(argv[1], NULL, 0);
	input.a = (uint32_t)strtoul(argv[2], NULL, 0);

	long_to_temp(&input, &output);

	printf("exponent: 0x%04X significand: 0x%08X %08X\n",
		   output.exponent, output.b, output.a);

	return 0;
}

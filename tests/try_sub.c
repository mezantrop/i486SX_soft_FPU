#include <stdio.h>
#include <string.h>
#include <stdint.h>

typedef unsigned long u_long;

static int try_sub(u_long *a, u_long *b);

static int try_sub(u_long *a, u_long *b)
{
	unsigned char ok;

	__asm__ volatile (
		"movl 0(%2), %%eax\n\t"
		"subl %%eax, 0(%1)\n\t"
		"movl 4(%2), %%eax\n\t"
		"sbbl %%eax, 4(%1)\n\t"
		"movl 8(%2), %%eax\n\t"
		"sbbl %%eax, 8(%1)\n\t"
		"movl 12(%2), %%eax\n\t"
		"sbbl %%eax, 12(%1)\n\t"
		"setae %0"
		: "=q" (ok)
		: "r" (a), "r" (b)
		: "eax", "memory"
	);

	return ok;
}

int main() {
	u_long a[4] __attribute__((aligned(16))) = {0, 0, 0, 0xa0000000};
	u_long b[4] __attribute__((aligned(16))) = {0, 0, 0, 0xe0000000};
	u_long tmp[4] __attribute__((aligned(16)));

	memcpy(tmp, a, sizeof(tmp));
	int res = try_sub(b, tmp);
	printf("res = %d, tmp = %08lx %08lx %08lx %08lx\n", res, tmp[0], tmp[1], tmp[2], tmp[3]);
	return 0;
}

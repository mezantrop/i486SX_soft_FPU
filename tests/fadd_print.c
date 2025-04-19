#include <stdio.h>

float num1 = 3.5;  /* Define 3.5 */
float num2 = 2.5;  /* Define 2.5 */

int main() {
	float result;

	/* Inline assembly to perform the floating-point addition */
	__asm__ (
		"flds %1			\n\t"	/* Load num1 (3.5) into ST(0) */
		"fadd %2			\n\t"	/* Add num2 (2.5) to ST(0) */
		"fstps %0			\n\t"	/* Store result in 'result' (memory), popping the value off the stack */
		: "=m" (result)				/* Output operand */
		: "m" (num1), "m" (num2)	/* Input operands: memory locations of num1 and num2 */
	);

	printf("Result: %.6f\n", result);
	return 0;
}

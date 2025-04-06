#include <stdio.h>
#include <stdlib.h>
#include <err.h>

#define DEBUG	0

void dbgprintf(const char *fmt, ...) {
	#if DEBUG
		va_list args;

		printf("DEBUG: ");
		va_start(args, fmt);
		vprintf(fmt, args);
		va_end(args);
	#endif
}

long double to_type(long double value, char t) {
	switch (t) {
		case 'c': dbgprintf("T: [%c] V: [%c]\n", t, (char)value); return (char)value;
		case 's': dbgprintf("T: [%c] V: [%hd]\n", t, (short)value); return (short)value;
		case 'i': dbgprintf("T: [%c] V: [%d]\n", t, (int)value); return (int)value;
		case 'l': dbgprintf("T: [%c] V: [%ld]\n", t, (long)value); return (long)value;
		case 'f': dbgprintf("T: [%c] V: [%f]\n", t, (float)value); return (float)value;
		case 'd': dbgprintf("T: [%c] V: [%f]\n", t, (double)value); return (double)value;
		case 'L': dbgprintf("T: [%c] V: [%Lf]\n", t, (long double)value); return (long double)value;
		default:  dbgprintf("T: DEFAULT\n"); return 0;
	}
}


/* ------------------------------------------------------------------------- */
int main(int argc, char *argv[]) {
	long double n1, n2, rt;
	char op = 0;						/* Operation: + - * / ... */


	if (argc != 6) {
		dprintf(2, "usage:\n\
	fcalc <number> <type> <'operation'> <number> <type>\n\n\
	number:\n\
		a string representing a numder suitable for strtold()\n\
	type:\n\
		integer:\n\
			c - char\n\
			s - short\n\
			i - int\n\
			l - long\n\
		real:\n\
			f - float\n\
			d - double\n\
			L - long double\n\
	operation:\n\
		+ - * /\n");
		exit(1);
	}

	n1 = strtold(argv[1], NULL);
	n2 = strtold(argv[4], NULL);

	dbgprintf("N1: [%Lf] T1: [%c] OP: [%c] N2: [%Lf] T2: [%c]\n",
		n1, argv[2][0], argv[3][0], n2, argv[5][0]);

	switch (argv[3][0]) {
		case '+':
			rt = to_type(n1, argv[2][0]) + to_type(n2, argv[5][0]);
		break;

		case '-':
			rt = to_type(n1, argv[2][0]) - to_type(n2, argv[5][0]);
		break;

		case '*':
			rt = to_type(n1, argv[2][0]) * to_type(n2, argv[5][0]);
		break;

		case '/':
			rt = to_type(n1, argv[2][0]) / to_type(n2, argv[5][0]);
		break;

		default:
			printf("Operation is not implemented yet\n");
			op = 0;
	}

	printf("%Lf\n", rt);
	return 0;
}

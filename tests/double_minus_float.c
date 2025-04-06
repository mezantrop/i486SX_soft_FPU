#include <stdio.h>
#include <stdlib.h>

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

int main(void) {
    long double n1 = 5.0000005L;
    long double n2 = 3.00000025L;
    long double result = 0;

    result = to_type(n1, 'd') - to_type(n2, 'f');

    printf("%Lf\n", result);
    return 0;
}

// EXPECTED RESULT: 1.9999997600

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
    long double n1 = -2.000004L;
    long double n2 = 4L;
    long double result = 0;

    result = to_type(n1, 'd') * to_type(n2, 'l');

    printf("%Lf\n", result);
    return 0;
}

// EXPECTED RESULT: -8.0000160000

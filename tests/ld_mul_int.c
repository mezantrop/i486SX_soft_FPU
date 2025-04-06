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
    long double n1 = 1.5000003L;
    long double n2 = 3L;
    long double result = 0;

    result = to_type(n1, 'L') * to_type(n2, 'i');

    printf("%Lf\n", result);
    return 0;
}

// EXPECTED RESULT: 4.5000009000

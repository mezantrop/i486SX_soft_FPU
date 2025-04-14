#include <stdio.h>

int main(void) {
    long double n1 = 1.5000003L;
    long double n2 = 3L;
    long double result = 0;

    result = (long double)n1 * (int)n2;

    printf("%Lf\n", result);
    return 0;
}

// EXPECTED RESULT: 4.5000009000

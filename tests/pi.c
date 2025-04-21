#include <stdio.h>

double calculate_pi(int terms) {
    double pi = 0.0;
    for (int i = 0; i < terms; i++) {
        double term = (i % 2 == 0 ? 1.0 : -1.0) / (2.0 * i + 1);
        pi += term;
    }
    return pi * 4;
}

int main() {
    int terms = 1000000;
    double pi = calculate_pi(terms);
    printf("Pi: %.15f\n", pi);
    return 0;
}

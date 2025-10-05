// The Computer Language Benchmarks Game
// https://salsa.debian.org/benchmarksgame-team/benchmarksgame/
//
// Translated from Jeremy Zerfas's Python program.
// Which was in turn translated from Mr Ledrug's C program.
// This C version uses the GMP library.

#include <stdio.h>
#include <stdlib.h>
#include <gmp.h>

// Global GMP integer variables
mpz_t acc, den, num, tmp1, tmp2;

// Function declarations
void next_Term(unsigned int k);
int extract_Digit(unsigned int nth);
void eliminate_Digit(int d);


void next_Term(unsigned int k) {
    unsigned int k2 = k * 2 + 1;
    mpz_addmul_ui(acc, num, 2);
    mpz_mul_ui(acc, acc, k2);
    mpz_mul_ui(den, den, k2);
    mpz_mul_ui(num, num, k);
}

int extract_Digit(unsigned int nth) {
    mpz_mul_ui(tmp1, num, nth);
    mpz_add(tmp2, tmp1, acc);
    mpz_tdiv_q(tmp1, tmp2, den);

    return mpz_get_ui(tmp1);
}

void eliminate_Digit(int d) {
    mpz_submul_ui(acc, den, d);
    mpz_mul_ui(acc, acc, 10);
    mpz_mul_ui(num, num, 10);
}

int main(int argc, char *argv[]) {
    if (argc != 2) {
        fprintf(stderr, "Usage: %s <number_of_digits>\n", argv[0]);
        return 1;
    }
    int n = atoi(argv[1]);
    if (n <= 0) {
        return 0;
    }

    // Initialize GMP variables
    mpz_init(acc);
    mpz_init_set_ui(den, 1);
    mpz_init_set_ui(num, 1);
    mpz_init(tmp1);
    mpz_init(tmp2);

    int i = 0;
    unsigned int k = 0;
    while (i < n) {
        k++;
        next_Term(k);

        if (mpz_cmp(num, acc) > 0) {
            continue;
        }

        int d = extract_Digit(3);
        if (d != extract_Digit(4)) {
            continue;
        }

        putchar(48 + d);
        i++;
        if (i % 10 == 0) {
            printf("\t:%d\n", i);
        }

        // *** THIS IS THE MISSING LINE THAT FIXES THE BUG ***
        eliminate_Digit(d);
    }

    // Clean up GMP variables
    mpz_clear(acc);
    mpz_clear(den);
    mpz_clear(num);
    mpz_clear(tmp1);
    mpz_clear(tmp2);

    return 0;
}

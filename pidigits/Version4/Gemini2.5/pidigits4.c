// The Computer Language Benchmarks Game
// https://salsa.debian.org/benchmarksgame-team/benchmarksgame/
//
// Translated from Python by Gemini
// Based on the Python program by Isaac Gouy and Jeremy Zerfas

#include <stdio.h>
#include <stdlib.h>
#include <gmp.h>

// Global big integer variables, similar to the Python script
mpz_t tmp1, tmp2, acc, den, num;

// Extracts a digit. Corresponds to extract_Digit() in Python.
static int extract_digit(unsigned int nth) {
    // tmp1 = num * nth
    mpz_mul_ui(tmp1, num, nth);
    // tmp2 = tmp1 + acc
    mpz_add(tmp2, tmp1, acc);
    // tmp1 = tmp2 / den (integer division)
    mpz_tdiv_q(tmp1, tmp2, den);

    return mpz_get_ui(tmp1);
}

// Applies the next term in the series. Corresponds to next_Term() in Python.
static void next_term(unsigned int k) {
    unsigned int k2 = k * 2 + 1;

    // acc = acc + num * 2
    mpz_addmul_ui(acc, num, 2);
    // acc = acc * k2
    mpz_mul_ui(acc, acc, k2);
    // den = den * k2
    mpz_mul_ui(den, den, k2);
    // num = num * k
    mpz_mul_ui(num, num, k);
}

// Consumes a digit. Corresponds to eliminate_Digit() in Python.
static void eliminate_digit(unsigned int d) {
    // acc = (acc - den * d) * 10
    mpz_submul_ui(acc, den, d);
    mpz_mul_ui(acc, acc, 10);
    // num = num * 10
    mpz_mul_ui(num, num, 10);
}


int main(int argc, char **argv) {
    if (argc != 2) {
        fprintf(stderr, "Usage: %s <number_of_digits>\n", argv[0]);
        return 1;
    }
    const int n = atoi(argv[1]);
    if (n <= 0) {
        return 0;
    }

    // Initialize all the GMP big integer variables
    mpz_init(tmp1);
    mpz_init(tmp2);
    mpz_init_set_ui(acc, 0);
    mpz_init_set_ui(den, 1);
    mpz_init_set_ui(num, 1);

    int i = 0;
    unsigned int k = 0;

    while (i < n) {
        k++;
        next_term(k);

        // if num > acc, continue
        if (mpz_cmp(num, acc) > 0) {
            continue;
        }

        const int d = extract_digit(3);
        if (d != extract_digit(4)) {
            continue;
        }

        putchar('0' + d);
        i++;
        if (i % 10 == 0) {
            printf("\t:%d\n", i);
        }

        eliminate_digit(d);
    }
    
    // Pad the final line with spaces if it's not a full 10 digits
    int remaining = n % 10;
    if (remaining != 0) {
        for (int j = 0; j < (10 - remaining); j++) {
            putchar(' ');
        }
        printf("\t:%d\n", n);
    }


    // Free the memory used by the GMP variables
    mpz_clear(tmp1);
    mpz_clear(tmp2);
    mpz_clear(acc);
    mpz_clear(den);
    mpz_clear(num);

    return 0;
}

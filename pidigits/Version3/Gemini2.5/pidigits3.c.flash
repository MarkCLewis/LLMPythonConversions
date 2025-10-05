// The Computer Language Benchmarks Game
// https://salsa.debian.org/benchmarksgame-team/benchmarksgame/
//
// Translated from Jeremy Zerfas's Python program.

#include <stdio.h>
#include <stdlib.h>
#include <gmp.h>

// Declare GMP integers as global variables, similar to the Python script.
mpz_t tmp1, tmp2, acc, den, num;

// Extracts the nth digit.
unsigned long int extract_digit(unsigned long int nth) {
    // Corresponds to:
    // GMP.__gmpz_mul_ui(byref(tmp1), byref(num), c_ulong(nth))
    // GMP.__gmpz_add(byref(tmp2), byref(tmp1), byref(acc))
    // GMP.__gmpz_tdiv_q(byref(tmp1), byref(tmp2), byref(den))
    mpz_mul_ui(tmp1, num, nth);
    mpz_add(tmp2, tmp1, acc);
    mpz_tdiv_q(tmp1, tmp2, den);

    // GMP.__gmpz_get_ui(byref(tmp1))
    return mpz_get_ui(tmp1);
}

// Eliminates a digit from the accumulator.
void eliminate_digit(unsigned long int d) {
    // Corresponds to:
    // GMP.__gmpz_submul_ui(byref(acc), byref(den), c_ulong(d))
    // GMP.__gmpz_mul_ui(byref(acc), byref(acc), c_ulong(10))
    // GMP.__gmpz_mul_ui(byref(num), byref(num), c_ulong(10))
    mpz_submul_ui(acc, den, d);
    mpz_mul_ui(acc, acc, 10);
    mpz_mul_ui(num, num, 10);
}

// Advances the series to the next term.
void next_term(unsigned long int k) {
    unsigned long int k2 = k * 2 + 1;
    // Corresponds to:
    // GMP.__gmpz_addmul_ui(byref(acc), byref(num), c_ulong(2))
    // GMP.__gmpz_mul_ui(byref(acc), byref(acc), c_ulong(k2))
    // GMP.__gmpz_mul_ui(byref(den), byref(den), c_ulong(k2))
    // GMP.__gmpz_mul_ui(byref(num), byref(num), c_ulong(k))
    mpz_addmul_ui(acc, num, 2);
    mpz_mul_ui(acc, acc, k2);
    mpz_mul_ui(den, den, k2);
    mpz_mul_ui(num, num, k);
}

int main(int argc, char **argv) {
    if (argc < 2) {
        printf("Usage: %s <number_of_digits>\n", argv[0]);
        return 1;
    }
    int n = atoi(argv[1]);

    // Initialize GMP integers. In C, this must be done before use.
    // mpz_init_set_ui is equivalent to __gmpz_init_set_ui.
    mpz_init_set_ui(tmp1, 0);
    mpz_init_set_ui(tmp2, 0);
    mpz_init_set_ui(acc, 0);
    mpz_init_set_ui(den, 1);
    mpz_init_set_ui(num, 1);

    int i = 0;
    int k = 0;
    while (i < n) {
        k++;
        next_term(k);

        // mpz_cmp is the direct equivalent of __gmpz_cmp
        if (mpz_cmp(num, acc) > 0) {
            continue;
        }

        unsigned long int d = extract_digit(3);
        if (d != extract_digit(4)) {
            continue;
        }

        // putchar(48 + d) is the C equivalent of print(chr(48+d), end="")
        putchar(48 + d);
        i++;
        if (i % 10 == 0) {
            printf("\t:%d\n", i);
        }
        eliminate_digit(d);
    }

    // Free the memory used by the GMP integers.
    // This is good practice in C.
    mpz_clear(tmp1);
    mpz_clear(tmp2);
    mpz_clear(acc);
    mpz_clear(den);
    mpz_clear(num);

    return 0;
}

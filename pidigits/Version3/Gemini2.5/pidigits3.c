// The Computer Language Benchmarks Game
// https://salsa.debian.org/benchmarksgame-team/benchmarksgame/
//
// Translated from Jeremy Zerfas's Python program.
//
// To compile, you need the GMP library installed.
// On Debian/Ubuntu: sudo apt-get install libgmp3-dev
// On RedHat/CentOS: sudo yum install gmp-devel
// On macOS (Homebrew): brew install gmp
//
// Compile with:
// gcc -O2 -pipe -o pidigits pidigits.c -lgmp

#include <stdio.h>
#include <stdlib.h>
#include <gmp.h>

// Global GMP integer variables
mpz_t tmp1, tmp2, acc, den, num;

/**
 * @brief Extracts the nth digit.
 * Corresponds to the calculation: floor((num * nth + acc) / den)
 * @param nth The multiplier for the numerator.
 * @return The extracted digit as an unsigned long.
 */
static unsigned long extract_digit(unsigned long nth) {
    // tmp1 = num * nth
    mpz_mul_ui(tmp1, num, nth);
    // tmp2 = tmp1 + acc
    mpz_add(tmp2, tmp1, acc);
    // tmp1 = tmp2 / den (integer division)
    mpz_tdiv_q(tmp1, tmp2, den);
    
    return mpz_get_ui(tmp1);
}

/**
 * @brief Eliminates a digit `d` from the state variables to prepare for the next.
 * @param d The digit to eliminate.
 */
static void eliminate_digit(unsigned long d) {
    // acc = (acc - den * d) * 10
    mpz_submul_ui(acc, den, d);
    mpz_mul_ui(acc, acc, 10);
    // num = num * 10
    mpz_mul_ui(num, num, 10);
}

/**
 * @brief Advances the calculation to the next term `k`.
 * This implements the core of the spigot algorithm for Pi.
 * @param k The current term number.
 */
static void next_term(unsigned long k) {
    unsigned long k2 = k * 2 + 1;

    // acc = acc + num * 2
    mpz_addmul_ui(acc, num, 2);
    // acc = acc * k2
    mpz_mul_ui(acc, acc, k2);
    // den = den * k2
    mpz_mul_ui(den, den, k2);
    // num = num * k
    mpz_mul_ui(num, num, k);
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

    // Initialize GMP variables
    mpz_init(tmp1);
    mpz_init(tmp2);
    mpz_init_set_ui(acc, 0); // acc = 0
    mpz_init_set_ui(den, 1); // den = 1
    mpz_init_set_ui(num, 1); // num = 1

    int i = 0;
    unsigned long k = 0;

    while (i < n) {
        k++;
        next_term(k);

        // A digit is ready to be extracted only when num > acc.
        // We check if the next two digits are the same to ensure stability.
        if (mpz_cmp(num, acc) > 0) {
            continue;
        }

        unsigned long d = extract_digit(3);
        if (d != extract_digit(4)) {
            continue;
        }

        // We have a stable digit, print it.
        printf("%lu", d);
        fflush(stdout);

        i++;
        if (i % 10 == 0) {
            printf("\t:%d\n", i);
        }

        eliminate_digit(d);
    }
    
    // Add a newline if the last line wasn't a full 10-digit line.
    if (n % 10 != 0) {
        // Pad with spaces for alignment
        for (int j = 0; j < (10 - (n % 10)); j++) {
            printf(" ");
        }
        printf("\t:%d\n", n);
    }


    // Free the memory used by the GMP variables
    mpz_clears(tmp1, tmp2, acc, den, num, NULL);

    return 0;
}

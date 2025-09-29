/*
 * The Computer Language Benchmarks Game
 * https://salsa.debian.org/benchmarksgame-team/benchmarksgame/
 * 
 * Translated from the Python program by Claude
 * Based on Mr Ledrug's C program via Jeremy Zerfas's Python adaptation
 */

#include <stdio.h>
#include <stdlib.h>
#include <gmp.h>

// Initialize GMP variables
mpz_t tmp1, tmp2, acc, den, num;

// Extract a digit from the current state
int extract_digit(unsigned int nth) {
    // Joggling between tmp1 and tmp2, so GMP won't have to use temp buffers
    mpz_mul_ui(tmp1, num, nth);
    mpz_add(tmp2, tmp1, acc);
    mpz_tdiv_q(tmp1, tmp2, den);
    
    return mpz_get_ui(tmp1);
}

// Remove a digit from the current state
void eliminate_digit(int d) {
    mpz_submul_ui(acc, den, d);
    mpz_mul_ui(acc, acc, 10);
    mpz_mul_ui(num, num, 10);
}

// Calculate the next term in the series
void next_term(int k) {
    int k2 = k * 2 + 1;
    
    mpz_addmul_ui(acc, num, 2);
    mpz_mul_ui(acc, acc, k2);
    mpz_mul_ui(den, den, k2);
    mpz_mul_ui(num, num, k);
}

int main(int argc, char *argv[]) {
    if (argc != 2) {
        fprintf(stderr, "Usage: %s <number of digits>\n", argv[0]);
        return 1;
    }
    
    int n = atoi(argv[1]);
    if (n <= 0) {
        fprintf(stderr, "Number of digits must be positive\n");
        return 1;
    }
    
    // Initialize GMP variables
    mpz_init_set_ui(tmp1, 0);
    mpz_init_set_ui(tmp2, 0);
    mpz_init_set_ui(acc, 0);
    mpz_init_set_ui(den, 1);
    mpz_init_set_ui(num, 1);
    
    int i = 0;
    int k = 0;
    
    // Main loop to calculate digits of pi
    while (i < n) {
        k++;
        next_term(k);
        
        if (mpz_cmp(num, acc) > 0) {
            continue;
        }
        
        int d = extract_digit(3);
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
    
    // Add a newline if we didn't end on a multiple of 10
    if (n % 10 != 0) {
        printf("%*s\t:%d\n", 10 - (n % 10), "", n);
    }
    
    // Free GMP resources
    mpz_clear(tmp1);
    mpz_clear(tmp2);
    mpz_clear(acc);
    mpz_clear(den);
    mpz_clear(num);
    
    return 0;
}
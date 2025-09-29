/*
 * The Computer Language Benchmarks Game
 * https://salsa.debian.org/benchmarksgame-team/benchmarksgame/
 * 
 * Converted from Python to C by Claude
 */

#include <stdio.h>
#include <stdlib.h>
#include <gmp.h>

// Global GMP variables
mpz_t tmp1, tmp2, acc, den, num;

// Extract a digit
unsigned int extract_digit(unsigned int nth) {
    // Calculate tmp1 = num * nth
    mpz_mul_ui(tmp1, num, nth);
    
    // Calculate tmp2 = tmp1 + acc
    mpz_add(tmp2, tmp1, acc);
    
    // Calculate tmp1 = tmp2 / den
    mpz_tdiv_q(tmp1, tmp2, den);
    
    // Return the result as an unsigned int
    return mpz_get_ui(tmp1);
}

// Eliminate a digit
void eliminate_digit(unsigned int d) {
    // Calculate acc = acc - den * d
    mpz_submul_ui(acc, den, d);
    
    // Calculate acc = acc * 10
    mpz_mul_ui(acc, acc, 10);
    
    // Calculate num = num * 10
    mpz_mul_ui(num, num, 10);
}

// Calculate the next term
void next_term(unsigned int k) {
    unsigned int k2 = k * 2 + 1;
    
    // Calculate acc = acc + num * 2
    mpz_addmul_ui(acc, num, 2);
    
    // Calculate acc = acc * k2
    mpz_mul_ui(acc, acc, k2);
    
    // Calculate den = den * k2
    mpz_mul_ui(den, den, k2);
    
    // Calculate num = num * k
    mpz_mul_ui(num, num, k);
}

int main(int argc, char *argv[]) {
    // Check command-line arguments
    if (argc != 2) {
        fprintf(stderr, "Usage: %s <number of digits>\n", argv[0]);
        return 1;
    }
    
    // Parse the number of digits
    int n = atoi(argv[1]);
    if (n <= 0) {
        fprintf(stderr, "The number of digits must be positive\n");
        return 1;
    }
    
    // Initialize GMP variables
    mpz_init(tmp1);
    mpz_init(tmp2);
    mpz_init(acc);
    mpz_init(den);
    mpz_init(num);
    
    // Set initial values
    mpz_set_ui(acc, 0);
    mpz_set_ui(den, 1);
    mpz_set_ui(num, 1);
    
    // Main loop to calculate digits of pi
    int i = 0;
    unsigned int k = 0;
    while (i < n) {
        k++;
        next_term(k);
        
        // Check if num > acc
        if (mpz_cmp(num, acc) > 0) {
            continue;
        }
        
        // Extract and compare digits
        unsigned int d = extract_digit(3);
        if (d != extract_digit(4)) {
            continue;
        }
        
        // Print the digit
        putchar('0' + d);
        i++;
        
        // Print tab and counter every 10 digits
        if (i % 10 == 0) {
            printf("\t:%d\n", i);
        }
        
        // Eliminate the digit
        eliminate_digit(d);
    }
    
    // Add a final newline if needed
    if (n % 10 != 0) {
        // Print spaces to align the tab
        for (int j = 0; j < (10 - (n % 10)); j++) {
            putchar(' ');
        }
        printf("\t:%d\n", n);
    }
    
    // Clean up GMP variables
    mpz_clear(tmp1);
    mpz_clear(tmp2);
    mpz_clear(acc);
    mpz_clear(den);
    mpz_clear(num);
    
    return 0;
}
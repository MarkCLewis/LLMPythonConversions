// pidigits.c - based on GMP library
#include <stdio.h>
#include <gmp.h>

int extract_digit(mpz_t acc, mpz_t den, mpz_t num, int nth) {
    mpz_t tmp1, tmp2, result;
    mpz_inits(tmp1, tmp2, result, NULL);

    mpz_mul_ui(tmp1, num, nth);
    mpz_add(tmp2, tmp1, acc);
    mpz_tdiv_q(result, tmp2, den);

    int digit = mpz_get_ui(result);

    mpz_clears(tmp1, tmp2, result, NULL);
    return digit;
}

void eliminate_digit(mpz_t acc, mpz_t den, mpz_t num, int d) {
    mpz_submul_ui(acc, den, d);
    mpz_mul_ui(acc, acc, 10);
    mpz_mul_ui(num, num, 10);
}

void next_term(mpz_t acc, mpz_t den, mpz_t num, int k) {
    unsigned long k2 = 2 * k + 1;
    mpz_addmul_ui(acc, num, 2);
    mpz_mul_ui(acc, acc, k2);
    mpz_mul_ui(den, den, k2);
    mpz_mul_ui(num, num, k);
}

int main(int argc, char **argv) {
    if (argc < 2) {
        printf("Usage: %s <num_digits>\n", argv[0]);
        return 1;
    }

    int n = atoi(argv[1]);

    mpz_t acc, den, num;
    mpz_inits(acc, den, num, NULL);
    mpz_set_ui(acc, 0);
    mpz_set_ui(den, 1);
    mpz_set_ui(num, 1);

    int i = 0, k = 0;
    while (i < n) {
        k++;
        next_term(acc, den, num, k);

        if (mpz_cmp(num, acc) > 0)
            continue;

        int d = extract_digit(acc, den, num, 3);
        if (d != extract_digit(acc, den, num, 4))
            continue;

        printf("%c", '0' + d);
        i++;
        if (i % 10 == 0) {
            printf("\t:%d\n", i);
        }
        eliminate_digit(acc, den, num, d);
    }

    mpz_clears(acc, den, num, NULL);
    return 0;
}

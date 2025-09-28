// pidigits.c - Translated from pidigits4.py (no GMP required)
#include <stdio.h>
#include <stdlib.h>

#if defined(__GNUC__) || defined(__clang__)
    typedef __int128 int128_t;
#else
    #error "128-bit integer (__int128) support is required for this program."
#endif

int128_t tmp1, tmp2, acc, den, num;

int extract_digit(int nth) {
    tmp1 = num * nth;
    tmp2 = tmp1 + acc;
    tmp1 = tmp2 / den;
    return (int)tmp1;
}

void eliminate_digit(int d) {
    acc = (acc - den * d) * 10;
    num = num * 10;
}

void next_term(int k) {
    int k2 = k * 2 + 1;
    acc = (acc + num * 2) * k2;
    den = den * k2;
    num = num * k;
}

void print_digit(int d, int i) {
    putchar('0' + d);
    if (i % 10 == 0) {
        printf("\t:%d\n", i);
    }
}

int main(int argc, char **argv) {
    if (argc < 2) {
        fprintf(stderr, "Usage: %s <num_digits>\n", argv[0]);
        return 1;
    }

    int n = atoi(argv[1]);

    tmp1 = tmp2 = 0;
    acc = 0;
    den = 1;
    num = 1;

    int i = 0, k = 0;
    while (i < n) {
        k++;
        next_term(k);

        if (num > acc) continue;

        int d = extract_digit(3);
        if (d != extract_digit(4)) continue;

        print_digit(d, ++i);
        eliminate_digit(d);
    }

    return 0;
}

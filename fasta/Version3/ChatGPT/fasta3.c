// The Computer Language Benchmarks Game
// https://salsa.debian.org/benchmarksgame-team/benchmarksgame/
//
// Translated from Python version by ChatGPT
//
// Compile with:
//   gcc -O3 -march=native -o fasta fasta3.c
//
// Run with:
//   ./fasta 25000000 > out.txt

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>

#define WIDTH 60

// Input sequences
const char* alu =
  "GGCCGGGCGCGGTGGCTCACGCCTGTAATCCCAGCACTTTGG"
  "GAGGCCGAGGCGGGCGGATCACCTGAGGTCAGGAGTTCGAGA"
  "CCAGCCTGGCCAACATGGTGAAACCCCGTCTCTACTAAAAAT"
  "ACAAAAATTAGCCGGGCGTGGTGGCGCGCGCCTGTAATCCCA"
  "GCTACTCGGGAGGCTGAGGCAGGAGAATCGCTTGAACCCGGG"
  "AGGCGGAGGTTGCAGTGAGCCGAGATCGCGCCACTGCACTCC"
  "AGCCTGGGCGACAGAGCGAGACTCCGTCTCAAAAA";

typedef struct {
    char c;
    double p;
} Prob;

Prob iub[] = {
    {'a', 0.27}, {'c', 0.12}, {'g', 0.12}, {'t', 0.27},
    {'B', 0.02}, {'D', 0.02}, {'H', 0.02}, {'K', 0.02},
    {'M', 0.02}, {'N', 0.02}, {'R', 0.02}, {'S', 0.02},
    {'V', 0.02}, {'W', 0.02}, {'Y', 0.02}
};
int iub_len = 15;

Prob homosapiens[] = {
    {'a', 0.3029549426680},
    {'c', 0.1979883004921},
    {'g', 0.1975473066391},
    {'t', 0.3015094502008}
};
int homo_len = 4;

// Precompute cumulative probability
void make_cumulative(Prob* table, int len, double* probs, char* chars) {
    double sum = 0.0;
    for (int i = 0; i < len; i++) {
        sum += table[i].p;
        probs[i] = sum;
        chars[i] = table[i].c;
    }
}

// Linear congruential generator
static double gen_random(double* seed) {
    *seed = fmod(*seed * 3877.0 + 29573.0, 139968.0);
    return *seed / 139968.0;
}

void repeat_fasta(const char* src, int n) {
    int len = strlen(src);
    int pos = 0;
    while (n > 0) {
        int line_len = n < WIDTH ? n : WIDTH;
        for (int i = 0; i < line_len; i++) {
            putchar(src[pos]);
            pos++;
            if (pos >= len) pos = 0;
        }
        putchar('\n');
        n -= line_len;
    }
}

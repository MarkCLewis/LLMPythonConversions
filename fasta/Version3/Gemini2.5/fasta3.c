#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>

#define LINE_LENGTH 60
#define IM 139968.0
#define IA 3877.0
#define IC 29573.0

// The ALU sequence
const char *alu =
    "GGCCGGGCGCGGTGGCTCACGCCTGTAATCCCAGCACTTTGG"
    "GAGGCCGAGGCGGGCGGATCACCTGAGGTCAGGAGTTCGAGA"
    "CCAGCCTGGCCAACATGGTGAAACCCCGTCTCTACTAAAAAT"
    "ACAAAAATTAGCCGGGCGTGGTGGCGCGCGCCTGTAATCCCA"
    "GCTACTCGGGAGGCTGAGGCAGGAGAATCGCTTGAACCCGGG"
    "AGGCGGAGGTTGCAGTGAGCCGAGATCGCGCCACTGCACTCC"
    "AGCCTGGGCGACAGAGCGAGACTCCGTCTCAAAAA";

// Struct to hold character-probability pairs
typedef struct {
    char c;
    double p;
} amino_acid;

// IUB ambiguity codes and probabilities
amino_acid iub[] = {
    {'a', 0.27}, {'c', 0.12}, {'g', 0.12}, {'t', 0.27},
    {'B', 0.02}, {'D', 0.02}, {'H', 0.02}, {'K', 0.02},
    {'M', 0.02}, {'N', 0.02}, {'R', 0.02}, {'S', 0.02},
    {'V', 0.02}, {'W', 0.02}, {'Y', 0.02}
};

// Homo sapiens frequencies
amino_acid homosapiens[] = {
    {'a', 0.3029549426680},
    {'c', 0.1979883004921},
    {'g', 0.1975473066391},
    {'t', 0.3015094502008}
};

// Creates cumulative probability tables for faster lookups
void make_cumulative(const amino_acid* table, int size, double* P, char* C) {
    double prob = 0.0;
    for (int i = 0; i < size; ++i) {
        prob += table[i].p;
        P[i] = prob;
        C[i] = table[i].c;
    }
    // Ensure the last probability is 1.0 for safety with floating point math
    P[size - 1] = 1.0;
}

// Repeats the source string `src` until `n` characters have been printed
void repeat_fasta(const char* src, int n) {
    int src_len = strlen(src);
    int pos = 0;
    char line_buffer[LINE_LENGTH + 1];
    line_buffer[LINE_LENGTH] = '\n';

    int full_lines = n / LINE_LENGTH;
    for (int i = 0; i < full_lines; ++i) {
        for (int j = 0; j < LINE_LENGTH; ++j) {
            line_buffer[j] = src[pos];
            pos = (pos + 1) % src_len;
        }
        fwrite(line_buffer, 1, LINE_LENGTH + 1, stdout);
    }

    int remaining_chars = n % LINE_LENGTH;
    if (remaining_chars > 0) {
        for (int i = 0; i < remaining_chars; ++i) {
            line_buffer[i] = src[pos];
            pos = (pos + 1) % src_len;
        }
        line_buffer[remaining_chars] = '\n';
        fwrite(line_buffer, 1, remaining_chars + 1, stdout);
    }
}

// Generates and prints a random sequence based on the provided probability table
double random_fasta(const amino_acid* table, int size, int n, double seed) {
    double P[size];
    char C[size];
    make_cumulative(table, size, P, C);

    char line_buffer[LINE_LENGTH + 1];
    line_buffer[LINE_LENGTH] = '\n';

    int full_lines = n / LINE_LENGTH;
    for (int i = 0; i < full_lines; ++i) {
        for (int j = 0; j < LINE_LENGTH; ++j) {
            seed = fmod(seed * IA + IC, IM);
            double r = seed / IM;
            
            // Find character by checking against cumulative probabilities
            // This is equivalent to Python's bisect.bisect
            char next_char;
            for (int k = 0; k < size; ++k) {
                if (r < P[k]) {
                    next_char = C[k];
                    break;
                }
            }
            line_buffer[j] = next_char;
        }
        fwrite(line_buffer, 1, LINE_LENGTH + 1, stdout);
    }

    int remaining_chars = n % LINE_LENGTH;
    if (remaining_chars > 0) {
        for (int i = 0; i < remaining_chars; ++i) {
            seed = fmod(seed * IA + IC, IM);
            double r = seed / IM;
            
            char next_char;
            for (int k = 0; k < size; ++k) {
                if (r < P[k]) {
                    next_char = C[k];
                    break;
                }
            }
            line_buffer[i] = next_char;
        }
        line_buffer[remaining_chars] = '\n';
        fwrite(line_buffer, 1, remaining_chars + 1, stdout);
    }
    
    return seed;
}

int main(int argc, char **argv) {
    if (argc < 2) {
        fprintf(stderr, "Usage: %s <n>\n", argv[0]);
        return 1;
    }
    int n = atoi(argv[1]);

    fputs(">ONE Homo sapiens alu\n", stdout);
    repeat_fasta(alu, n * 2);

    fputs(">TWO IUB ambiguity codes\n", stdout);
    // The initial seed is 42.0, as in the Python script
    double seed = 42.0; 
    seed = random_fasta(iub, sizeof(iub)/sizeof(amino_acid), n * 3, seed);

    fputs(">THREE Homo sapiens frequency\n", stdout);
    random_fasta(homosapiens, sizeof(homosapiens)/sizeof(amino_acid), n * 5, seed);

    return 0;
}

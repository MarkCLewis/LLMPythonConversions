/*
 * The Computer Language Benchmarks Game
 * https://salsa.debian.org/benchmarksgame-team/benchmarksgame/
 *
 * Converted from Python to C by Claude
 * Original Python by Ian Osgood, Heinrich Acker, Justin Peel, and Christopher Sean Forgeron
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define LINE_WIDTH 60
#define IM 139968.0

// ALU sequence
const char *alu =
   "GGCCGGGCGCGGTGGCTCACGCCTGTAATCCCAGCACTTTGG"
   "GAGGCCGAGGCGGGCGGATCACCTGAGGTCAGGAGTTCGAGA"
   "CCAGCCTGGCCAACATGGTGAAACCCCGTCTCTACTAAAAAT"
   "ACAAAAATTAGCCGGGCGTGGTGGCGCGCGCCTGTAATCCCA"
   "GCTACTCGGGAGGCTGAGGCAGGAGAATCGCTTGAACCCGGG"
   "AGGCGGAGGTTGCAGTGAGCCGAGATCGCGCCACTGCACTCC"
   "AGCCTGGGCGACAGAGCGAGACTCCGTCTCAAAAA";

// Frequency tables
typedef struct {
    char c;
    double p;
} Frequency;

Frequency iub[] = {
    {'a', 0.27}, {'c', 0.12}, {'g', 0.12}, {'t', 0.27},
    {'B', 0.02}, {'D', 0.02}, {'H', 0.02}, {'K', 0.02},
    {'M', 0.02}, {'N', 0.02}, {'R', 0.02}, {'S', 0.02},
    {'V', 0.02}, {'W', 0.02}, {'Y', 0.02}
};

Frequency homosapiens[] = {
    {'a', 0.3029549426680},
    {'c', 0.1979883004921},
    {'g', 0.1975473066391},
    {'t', 0.3015094502008}
};

// Convert frequency table to cumulative probabilities
typedef struct {
    double prob;
    char c;
} CumulativeFreq;

void make_cumulative(Frequency *table, int table_size, CumulativeFreq *result) {
    double prob = 0.0;
    for (int i = 0; i < table_size; i++) {
        prob += table[i].p;
        result[i].prob = prob;
        result[i].c = table[i].c;
    }
}

// Binary search to find character based on probability
char lookup_char(CumulativeFreq *table, int table_size, double random) {
    int low = 0;
    int high = table_size - 1;
    
    while (low <= high) {
        int mid = (low + high) / 2;
        if (random < table[mid].prob)
            high = mid - 1;
        else
            low = mid + 1;
    }
    
    return table[low].c;
}

// Generate a repeated sequence
void repeat_fasta(const char *src, int src_len, int n) {
    char buffer[LINE_WIDTH + 1];
    buffer[LINE_WIDTH] = '\n';
    
    int pos = 0;
    int remaining = n;
    
    while (remaining > 0) {
        int line_len = remaining < LINE_WIDTH ? remaining : LINE_WIDTH;
        for (int i = 0; i < line_len; i++) {
            buffer[i] = src[(pos + i) % src_len];
        }
        fwrite(buffer, 1, line_len + (line_len == LINE_WIDTH ? 1 : 0), stdout);
        if (line_len < LINE_WIDTH) {
            putchar('\n');
        }
        
        pos = (pos + line_len) % src_len;
        remaining -= line_len;
    }
}

// Generate a random sequence
double random_fasta(CumulativeFreq *table, int table_size, int n, double seed) {
    char buffer[LINE_WIDTH + 1];
    buffer[LINE_WIDTH] = '\n';
    
    int remaining = n;
    
    while (remaining > 0) {
        int line_len = remaining < LINE_WIDTH ? remaining : LINE_WIDTH;
        
        for (int i = 0; i < line_len; i++) {
            seed = (seed * 3877.0 + 29573.0) % IM;
            buffer[i] = lookup_char(table, table_size, seed / IM);
        }
        
        fwrite(buffer, 1, line_len + (line_len == LINE_WIDTH ? 1 : 0), stdout);
        if (line_len < LINE_WIDTH) {
            putchar('\n');
        }
        
        remaining -= line_len;
    }
    
    return seed;
}

int main(int argc, char *argv[]) {
    if (argc != 2) {
        fprintf(stderr, "Usage: %s <number>\n", argv[0]);
        return 1;
    }
    
    int n = atoi(argv[1]);
    
    // Calculate the size of the frequency tables
    int iub_size = sizeof(iub) / sizeof(iub[0]);
    int homosapiens_size = sizeof(homosapiens) / sizeof(homosapiens[0]);
    
    // Convert the frequency tables to cumulative probabilities
    CumulativeFreq iub_cumul[iub_size];
    CumulativeFreq homosapiens_cumul[homosapiens_size];
    
    make_cumulative(iub, iub_size, iub_cumul);
    make_cumulative(homosapiens, homosapiens_size, homosapiens_cumul);
    
    // Generate the sequences
    printf(">ONE Homo sapiens alu\n");
    repeat_fasta(alu, strlen(alu), n * 2);
    
    printf(">TWO IUB ambiguity codes\n");
    double seed = 42.0;
    seed = random_fasta(iub_cumul, iub_size, n * 3, seed);
    
    printf(">THREE Homo sapiens frequency\n");
    random_fasta(homosapiens_cumul, homosapiens_size, n * 5, seed);
    
    return 0;
}
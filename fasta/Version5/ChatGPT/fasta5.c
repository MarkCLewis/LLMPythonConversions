// fasta.c - FASTA benchmark in C using OpenMP
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <omp.h>

#define IM 139968
#define IA 3877
#define IC 29573

typedef struct {
    const char *name;
    const char *sequence;
} Sequence;

typedef struct {
    char c;
    double p;
} Nucleotide;

static const char *ALU =
    "GGCCGGGCGCGGTGGCTCACGCCTGTAATCCCAGCACTTTGGGAGGCCGAGGCGGGCGGA"
    "TCACCTGAGGTCAGGAGTTCGAGACCAGCCTGGCCAACATGGTGAAACCCCGTCTCTACT"
    "AAAAATACAAAAATTAGCCGGGCGTGGTGGCGCGCGCCTGTAATCCCAGCTACTCGGGAG"
    "GCTGAGGCAGGAGAATCGCTTGAACCCGGGAGGCGGAGGTTGCAGTGAGCCGAGATCGCG"
    "CCACTGCACTCCAGCCTGGGCGACAGAGCGAGACTCCGTCTCAAAAA";

static Nucleotide IUB[] = {
    {'a', 0.27}, {'c', 0.12}, {'g', 0.12}, {'t', 0.27},
    {'B', 0.02}, {'D', 0.02}, {'H', 0.02}, {'K', 0.02},
    {'M', 0.02}, {'N', 0.02}, {'R', 0.02}, {'S', 0.02},
    {'V', 0.02}, {'W', 0.02}, {'Y', 0.02},
};

static Nucleotide HomoSapiens[] = {
    {'a', 0.3029549426680},
    {'c', 0.1979883004921},
    {'g', 0.1975473066391},
    {'t', 0.3015094502008}
};

void make_cumulative(Nucleotide *nucl, int len) {
    double p = 0.0;
    for (int i = 0; i < len; i++) {
        p += nucl[i].p;
        nucl[i].p = p;
    }
}

char select_random(Nucleotide *nucl, int len, int *seed) {
    double r = (double)(*seed = (*seed * IA + IC) % IM) / IM;
    for (int i = 0; i < len; i++) {
        if (r < nucl[i].p)
            return nucl[i].c;
    }
    return nucl[len - 1].c;
}

void repeat_fasta(const char *header, const char *sequence, int n) {
    printf("%s", header);
    int len = strlen(sequence);
    for (int i = 0; i < n; i += 60) {
        int line_len = (n - i < 60) ? n - i : 60;
        for (int j = 0; j < line_len; ++j) {
            putchar(sequence[(i + j) % len]);
        }
        putchar('\n');
    }
}

void random_fasta(const char *header, Nucleotide *nucl, int len, int n, int seed) {
    printf("%s", header);
    make_cumulative(nucl, len);

    #pragma omp parallel
    {
        int thread_seed = seed + omp_get_thread_num() * 97;

        #pragma omp for schedule(static)
        for (int i = 0; i < n; i += 60) {
            char line[61];
            int line_len = (n - i < 60) ? n - i : 60;
            for (int j = 0; j < line_len; ++j) {
                line[j] = select_random(nucl, len, &thread_seed);
            }
            line[line_len] = '\0';

            #pragma omp critical
            {
                puts(line);
            }
        }
    }
}

int main(int argc, char **argv) {
    int n = (argc > 1) ? atoi(argv[1]) : 1000;

    repeat_fasta(">ONE Homo sapiens alu\n", ALU, n * 2);
    random_fasta(">TWO IUB ambiguity codes\n", IUB, sizeof(IUB) / sizeof(Nucleotide), n * 3, 42);
    random_fasta(">THREE Homo sapiens frequency\n", HomoSapiens, sizeof(HomoSapiens) / sizeof(Nucleotide), n * 5, 42);

    return 0;
}

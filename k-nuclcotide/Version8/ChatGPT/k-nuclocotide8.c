// knucleotide.c — C port of k-nuclcotide8.py (Computer Language Benchmarks Game)
// Build:  gcc -O3 -march=native -std=c11 -o knucleotide knucleotide.c
// Run:    ./knucleotide < input.fa
//
// Output:
//  - 1-mer frequency table (percent, 3 decimals, sorted by freq desc then lex)
//  - blank line
//  - 2-mer frequency table (same sorting)
//  - blank line
//  - counts for: GGT, GGTA, GGTATT, GGTATTTTAATT, GGTATTTTAATTTATAGT

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <inttypes.h>

typedef struct {
    char key[32];
    uint64_t count;
} Item;

static inline int base2bits(int c) {
    // Map A,C,G,T (uppercase) -> 3,2,0,1 respectively; return -1 for others.
    switch (c) {
        case 'G': return 0;
        case 'T': return 1;
        case 'C': return 2;
        case 'A': return 3;
        default:  return -1;
    }
}

static uint8_t* read_three_sequence(size_t *out_len) {
    // Slurp stdin into memory
    size_t cap = 1 << 20, len = 0;
    char *buf = (char*)malloc(cap);
    if (!buf) { fprintf(stderr, "oom\n"); exit(1); }
    int ch;
    while ((ch = getchar()) != EOF) {
        if (len + 1 >= cap) { cap <<= 1; buf = (char*)realloc(buf, cap); if (!buf) { fprintf(stderr, "oom\n"); exit(1); } }
        buf[len++] = (char)ch;
    }
    buf[len] = '\0';

    // Find header line starting with ">THREE"
    size_t n = len, i = 0;
    int found = 0;
    while (i < n) {
        if (buf[i] == '>') {
            size_t j = i + 1;
            while (j < n && buf[j] != '\n' && buf[j] != '\r') j++;
            if (j > i + 1 && j - (i + 1) >= 5 && strncmp(&buf[i + 1], "THREE", 5) == 0) {
                found = 1; i = j + 1; break;
            }
            i = j + 1;
        } else {
            while (i < n && buf[i] != '\n' && buf[i] != '\r') i++;
            i++;
        }
    }
    if (!found) { free(buf); *out_len = 0; return NULL; }

    // Collect sequence until next '>' or EOF; uppercase and map to 0..3, skip others
    size_t cap2 = 1 << 20, m = 0;
    uint8_t *seq = (uint8_t*)malloc(cap2);
    if (!seq) { fprintf(stderr, "oom\n"); exit(1); }

    for (; i < n && buf[i] != '>'; ++i) {
        char c = buf[i];
        if (c >= 'a' && c <= 'z') c = (char)(c - 32); // uppercase
        int v = base2bits((unsigned char)c);
        if (v >= 0) {
            if (m >= cap2) { cap2 <<= 1; seq = (uint8_t*)realloc(seq, cap2); if (!seq) { fprintf(stderr, "oom\n"); exit(1); } }
            seq[m++] = (uint8_t)v;
        }
    }
    free(buf);
    *out_len = m;
    return seq;
}

static inline uint64_t code_of(const char *s) {
    uint64_t x = 0;
    for (const unsigned char *p = (const unsigned char*)s; *p; ++p) {
        int c = *p;
        if (c >= 'a' && c <= 'z') c -= 32;
        int b = base2bits(c);
        x = (x << 2) | (uint64_t)b;
    }
    return x;
}

static void count_k_all(const uint8_t *seq, size_t n, int k, uint64_t *table) {
    if (n < (size_t)k) return;
    const uint64_t mask = (k == 32) ? ~0ULL : ((1ULL << (2*k)) - 1ULL);
    uint64_t code = 0;
    for (int i = 0; i < k - 1; ++i) code = (code << 2) | seq[i];
    for (size_t i = k - 1; i < n; ++i) {
        code = ((code << 2) | seq[i]) & mask;
        table[code] += 1;
    }
}

static uint64_t count_k_specific(const uint8_t *seq, size_t n, int k, uint64_t target_code) {
    if (n < (size_t)k) return 0;
    const uint64_t mask = (k == 32) ? ~0ULL : ((1ULL << (2*k)) - 1ULL);
    uint64_t code = 0, cnt = 0;
    for (int i = 0; i < k - 1; ++i) code = (code << 2) | seq[i];
    for (size_t i = k - 1; i < n; ++i) {
        code = ((code << 2) | seq[i]) & mask;
        cnt += (code == target_code);
    }
    return cnt;
}

static int cmp_items(const void *a, const void *b) {
    const Item *x = (const Item*)a, *y = (const Item*)b;
    if (y->count < x->count) return -1; // descending by count
    if (y->count > x->count) return 1;
    return strcmp(x->key, y->key);      // tie: ascending lexicographically
}

int main(void) {
    size_t n = 0;
    uint8_t *seq = read_three_sequence(&n);
    if (!seq) return 0;

    // Lists as in the Python program
    static const char *queries[] = {
        "GGT", "GGTA", "GGTATT", "GGTATTTTAATT", "GGTATTTTAATTTATAGT"
    };

    // 1-mers ------------------------------------------------------------
    uint64_t c1[4] = {0};
    for (size_t i = 0; i < n; ++i) c1[ seq[i] ]++;

    Item mono_items[4];
    const char *mono_keys[4] = {"A","C","G","T"};
    // We printed in Python order of discovered keys; to be deterministic,
    // we gather from fixed set and sort below.
    for (int i = 0; i < 4; ++i) {
        strncpy(mono_items[i].key, mono_keys[i], sizeof(mono_items[i].key));
        mono_items[i].key[sizeof(mono_items[i].key)-1] = '\0';
        mono_items[i].count = c1[ (int)(code_of(mono_items[i].key)) ];
    }
    qsort(mono_items, 4, sizeof(Item), cmp_items);

    // 2-mers ------------------------------------------------------------
    uint64_t c2[16] = {0};
    count_k_all(seq, n, 2, c2);

    static const char *di_keys[16] = {
        "AA","AC","AG","AT","CA","CC","CG","CT",
        "GA","GC","GG","GT","TA","TC","TG","TT"
    };
    Item di_items[16];
    for (int i = 0; i < 16; ++i) {
        strncpy(di_items[i].key, di_keys[i], sizeof(di_items[i].key));
        di_items[i].key[sizeof(di_items[i].key)-1] = '\0';
        di_items[i].count = c2[ (int)code_of(di_items[i].key) ];
    }
    qsort(di_items, 16, sizeof(Item), cmp_items);

    // Print 1-mer percentages
    const double denom1 = (n > 0) ? (double)n : 1.0;
    for (int i = 0; i < 4; ++i) {
        double pct = (mono_items[i].count * 100.0) / denom1;
        printf("%s %.3f\n", mono_items[i].key, pct);
    }
    printf("\n");

    // Print 2-mer percentages
    const double denom2 = (n >= 2) ? (double)(n - 1) : 1.0;
    for (int i = 0; i < 16; ++i) {
        double pct = (di_items[i].count * 100.0) / denom2;
        printf("%s %.3f\n", di_items[i].key, pct);
    }
    printf("\n");

    // Specific queries (counts)
    for (size_t i = 0; i < sizeof(queries)/sizeof(queries[0]); ++i) {
        const char *pat = queries[i];
        int k = (int)strlen(pat);
        uint64_t cnt = count_k_specific(seq, n, k, code_of(pat));
        printf("%" PRIu64 "\t%s\n", cnt, pat);
    }

    free(seq);
    return 0;
}

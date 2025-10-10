// knucleotide.c — C port of knuclcotide3.py (Computer Language Benchmarks Game)
// Build:  gcc -O3 -march=native -o knucleotide knucleotide.c
// Run:    ./knucleotide < input.fa

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>

typedef struct {
    char s[32];     // k-mer text (e.g., "GGTATTTTAATT")
    uint64_t count; // occurrences
} Item;

static inline int base2bits(int c) {
    // Map Gg->0, Tt->1, Cc->2, Aa->3 ; return -1 for other chars
    switch (c) {
        case 'G': case 'g': return 0;
        case 'T': case 't': return 1;
        case 'C': case 'c': return 2;
        case 'A': case 'a': return 3;
        default: return -1;
    }
}

static uint8_t* read_sequence_three(size_t *out_len) {
    // Read stdin; find header line starting with ">THREE", then collect sequence
    // bytes until next '>' or EOF, skipping whitespace; translate to 0..3 codes.
    size_t cap = 1 << 20, len = 0;
    char *buf = (char*)malloc(cap);
    if (!buf) { fprintf(stderr, "oom\n"); exit(1); }
    size_t rlen = 0;
    int ch;
    while ((ch = getchar()) != EOF) {
        if (rlen + 1 >= cap) { cap <<= 1; buf = (char*)realloc(buf, cap); if (!buf) { fprintf(stderr, "oom\n"); exit(1); } }
        buf[rlen++] = (char)ch;
    }
    buf[rlen] = '\0';

    // Find the section starting with a header that begins with ">THREE"
    size_t i = 0;
    int found = 0;
    while (i < rlen) {
        if (buf[i] == '>') {
            size_t j = i + 1;
            while (j < rlen && buf[j] != '\n' && buf[j] != '\r') j++;
            // header is buf[i+1..j-1]
            if (j > i + 1 && j - (i + 1) >= 5 &&
                strncmp(&buf[i + 1], "THREE", 5) == 0) {
                found = 1;
                i = j + 1;
                break;
            }
            i = j + 1;
        } else {
            // skip until next header
            while (i < rlen && buf[i] != '\n' && buf[i] != '\r') i++;
            i++;
        }
    }
    if (!found) {
        free(buf);
        *out_len = 0;
        return NULL;
    }

    // Collect sequence lines until next '>' or EOF
    // Translate to 0..3 and store into compact array
    size_t cap2 = 1 << 20;
    uint8_t *seq = (uint8_t*)malloc(cap2);
    if (!seq) { fprintf(stderr, "oom\n"); exit(1); }
    len = 0;
    for (; i < rlen; ++i) {
        char c = buf[i];
        if (c == '>') break;
        int v = base2bits((unsigned char)c);
        if (v >= 0) {
            if (len >= cap2) { cap2 <<= 1; seq = (uint8_t*)realloc(seq, cap2); if (!seq) { fprintf(stderr, "oom\n"); exit(1); } }
            seq[len++] = (uint8_t)v;
        }
    }
    free(buf);
    *out_len = len;
    return seq;
}

static inline uint64_t code_of(const char *s) {
    uint64_t x = 0;
    for (const char *p = s; *p; ++p) {
        int b = base2bits((unsigned char)*p);
        x = (x << 2) | (uint64_t)b;
    }
    return x;
}

static void count_k_all(const uint8_t *seq, size_t n, int k, uint64_t *table, size_t table_size) {
    if (n < (size_t)k) return;
    uint64_t mask = (k == 32) ? ~0ULL : ((1ULL << (2*k)) - 1ULL);
    uint64_t code = 0;
    for (int i = 0; i < k - 1; ++i) code = (code << 2) | seq[i];
    for (size_t i = k - 1; i < n; ++i) {
        code = ((code << 2) | seq[i]) & mask;
        table[code] += 1;
    }
}

static uint64_t count_k_specific(const uint8_t *seq, size_t n, int k, uint64_t target_code) {
    if (n < (size_t)k) return 0;
    uint64_t mask = (k == 32) ? ~0ULL : ((1ULL << (2*k)) - 1ULL);
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
    if (y->count < x->count) return -1;
    if (y->count > x->count) return 1;
    return strcmp(x->s, y->s);
}

int main(void) {
    size_t n = 0;
    uint8_t *seq = read_sequence_three(&n);
    if (!seq) return 0;

    // 1-mers (4) and 2-mers (16)
    static const char *mono[] = {"G","A","T","C"};
    static const char *di[] = {
        "GG","GA","GT","GC",
        "AG","AA","AT","AC",
        "TG","TA","TT","TC",
        "CG","CA","CT","CC"
    };
    static const char *klist[] = {
        "GGT","GGTA","GGTATT","GGTATTTTAATT","GGTATTTTAATTTATAGT"
    };

    // Count 1-mers
    uint64_t c1[4] = {0};
    for (size_t i = 0; i < n; ++i) c1[ seq[i] ]++;

    // Count 2-mers (rolling)
    uint64_t c2[16] = {0};
    count_k_all(seq, n, 2, c2, 16);

    // Prepare items for printing (sorted by count desc, then lex)
    // Mono
    Item mono_items[4];
    for (int i = 0; i < 4; ++i) {
        strncpy(mono_items[i].s, mono[i], sizeof(mono_items[i].s));
        mono_items[i].s[sizeof(mono_items[i].s)-1] = '\0';
        mono_items[i].count = c1[ code_of(mono[i]) ];
    }
    qsort(mono_items, 4, sizeof(Item), cmp_items);
    // Di
    Item di_items[16];
    for (int i = 0; i < 16; ++i) {
        strncpy(di_items[i].s, di[i], sizeof(di_items[i].s));
        di_items[i].s[sizeof(di_items[i].s)-1] = '\0';
        di_items[i].count = c2[ code_of(di[i]) ];
    }
    qsort(di_items, 16, sizeof(Item), cmp_items);

    // Print mono percentages
    double denom1 = (n >= 1) ? (double)n : 1.0;
    for (int i = 0; i < 4; ++i) {
        double pct = (mono_items[i].count * 100.0) / denom1;
        printf("%s %.3f\n", mono_items[i].s, pct);
    }
    printf("\n");

    // Print di percentages
    double denom2 = (n >= 2) ? (double)(n - 2 + 1) : 1.0;
    for (int i = 0; i < 16; ++i) {
        double pct = (di_items[i].count * 100.0) / denom2;
        printf("%s %.3f\n", di_items[i].s, pct);
    }
    printf("\n");

    // Print counts for specified k-mers, each as "<count>\t<kmer>"
    for (int i = 0; i < (int)(sizeof(klist)/sizeof(klist[0])); ++i) {
        const char *pat = klist[i];
        int k = (int)strlen(pat);
        uint64_t code = code_of(pat);
        uint64_t cnt  = count_k_specific(seq, n, k, code);
        printf("%d\t%s\n", cnt, pat);
    }

    free(seq);
    return 0;
}

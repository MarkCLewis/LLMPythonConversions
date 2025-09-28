// reverse_complement.c — C port of reverse-complement6.py (Computer Language Benchmarks Game)
// Build:  gcc -O3 -march=native -std=c11 -o reverse_complement reverse_complement.c
// Run:    ./reverse_complement < input.fa > output.fa

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

#define READ_CHUNK (1u<<20)  // 1 MiB
#define LINE_LEN   60

static unsigned char COMP[256];

static void init_comp(void) {
    // Default: identity (uppercased later where applicable)
    for (int i = 0; i < 256; ++i) COMP[i] = (unsigned char)i;

    // Mapping copied from Python table:
    //   source: "ABCDGHKMNRSTUVWYabcdghkmnrstuvwy"
    //   dest  : "TVGHCDMKNYSAABWRTVGHCDMKNYSAABWR"
    const char *src = "ABCDGHKMNRSTUVWYabcdghkmnrstuvwy";
    const char *dst = "TVGHCDMKNYSAABWRTVGHCDMKNYSAABWR";
    for (const char *p = src, *q = dst; *p; ++p, ++q) {
        COMP[(unsigned char)*p] = (unsigned char)*q;
    }

    // If an unmapped alphabetic character is seen, we uppercase and fall back to identity.
    // We also explicitly ignore whitespace chars during sequence accumulation.
}

static unsigned char *read_all(size_t *out_len) {
    size_t cap = READ_CHUNK, len = 0;
    unsigned char *buf = (unsigned char*)malloc(cap);
    if (!buf) { perror("malloc"); exit(1); }

    for (;;) {
        if (len + READ_CHUNK + 1 > cap) {
            cap <<= 1;
            unsigned char *nb = (unsigned char*)realloc(buf, cap);
            if (!nb) { perror("realloc"); free(buf); exit(1); }
            buf = nb;
        }
        size_t got = fread(buf + len, 1, READ_CHUNK, stdin);
        len += got;
        if (got < READ_CHUNK) break;
    }
    buf[len] = 0;
    *out_len = len;
    return buf;
}

int main(void) {
    init_comp();

    size_t n = 0;
    unsigned char *in = read_all(&n);
    size_t i = 0;

    // Walk the FASTA stream
    while (i < n) {
        // Find next header
        while (i < n && in[i] != '>') i++;
        if (i >= n) break;

        // Emit header line including trailing newline
        size_t hstart = i;
        while (i < n && in[i] != '\n') i++;
        size_t hend = (i < n ? i + 1 : i);
        if (hend > hstart) {
            if (fwrite(in + hstart, 1, hend - hstart, stdout) != (hend - hstart)) {
                perror("fwrite"); free(in); return 1;
            }
        }

        // Collect raw sequence until next '>' or EOF; drop '\n', '\r', ' '
        size_t seq_cap = 1 << 20, seq_len = 0;
        unsigned char *seq = (unsigned char*)malloc(seq_cap);
        if (!seq) { perror("malloc"); free(in); return 1; }

        size_t p = hend;
        while (p < n && in[p] != '>') {
            unsigned char c = in[p++];
            if (c == '\n' || c == '\r' || c == ' ') continue;
            if (seq_len + 1 >= seq_cap) {
                seq_cap <<= 1;
                unsigned char *ns = (unsigned char*)realloc(seq, seq_cap);
                if (!ns) { perror("realloc"); free(seq); free(in); return 1; }
                seq = ns;
            }
            seq[seq_len++] = c;
        }

        // Reverse-complement and emit in 60-char lines
        const size_t L = LINE_LEN;
        unsigned char line[L];
        size_t pos = seq_len;

        while (pos > 0) {
            size_t take = (pos >= L ? L : pos);
            // Fill line with complements from the end of seq
            for (size_t k = 0; k < take; ++k) {
                unsigned char c = seq[pos - 1 - k];
                // Use mapping table directly; if letter is lowercase and not in the table,
                // promote to uppercase and fall back to identity.
                unsigned char outc = COMP[c];
                if (isalpha((int)c) && outc == c) {
                    unsigned char uc = (unsigned char)toupper(c);
                    outc = COMP[uc];
                    if (outc == uc) outc = uc; // identity if still unmapped
                }
                line[k] = outc;
            }
            if (fwrite(line, 1, take, stdout) != take) {
                perror("fwrite"); free(seq); free(in); return 1;
            }
            fputc('\n', stdout);
            pos -= take;
        }

        free(seq);
        i = p; // continue at next header (or EOF)
    }

    free(in);
    return 0;
}

// reverse_complement.c — C port of reverse-complement5.py (Computer Language Benchmarks Game)
// Build:  gcc -O3 -march=native -std=c11 -o reverse_complement reverse_complement.c
// Run:    ./reverse_complement < input.fa > output.fa

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

#define READ_CHUNK (1<<20)   // 1 MiB
#define LINE_LEN   60

// Complement table (IUPAC + common letters). We output UPPERCASE.
static unsigned char COMP[256];

static void init_comp(void) {
    for (int i = 0; i < 256; ++i) COMP[i] = (unsigned char)i;

    // Map to uppercase first (we'll index with toupper() below for safety)
    const char *from = "ACGTMRYKVBHDNU";
    const char *to   = "TGCATYRMBDHVNA";
    for (const char *p = from, *q = to; *p; ++p, ++q) {
        COMP[(unsigned char)(*p)] = (unsigned char)(*q);
        COMP[(unsigned char)tolower(*p)] = (unsigned char)(*q);
    }
    // W and S are self-complements
    COMP['W'] = 'W'; COMP['w'] = 'W';
    COMP['S'] = 'S'; COMP['s'] = 'S';

    // Newlines remain newlines if ever encountered in translation; we strip them for RC.
    COMP['\n'] = '\n'; COMP['\r'] = '\r';
}

// Read all of stdin into a single buffer (grows as needed).
static unsigned char *read_all(size_t *out_len) {
    size_t cap = READ_CHUNK, len = 0;
    unsigned char *buf = (unsigned char*)malloc(cap);
    if (!buf) { perror("malloc"); exit(1); }

    for (;;) {
        if (len + READ_CHUNK + 1 > cap) {
            cap = (cap << 1);
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

    size_t in_len = 0;
    unsigned char *in = read_all(&in_len);
    size_t i = 0, n = in_len;

    // Process FASTA: for each header '>', write header line,
    // then collect sequence (skip newlines and CR), reverse-complement, and
    // write with 60 chars per line.
    while (i < n) {
        if (in[i] != '>') {
            // Skip until next header (robust to any preamble)
            while (i < n && in[i] != '>') i++;
            if (i >= n) break;
        }

        // Write header line (up to newline)
        size_t hstart = i;
        while (i < n && in[i] != '\n') i++;
        size_t hend = (i < n ? i + 1 : i); // include trailing newline if present
        if (hend > hstart) {
            if (fwrite(in + hstart, 1, hend - hstart, stdout) != (hend - hstart)) {
                perror("fwrite"); free(in); return 1;
            }
        }

        // Collect sequence bytes until next header '>' or EOF (skip newlines/CR)
        size_t seq_cap = 1 << 20, seq_len = 0;
        unsigned char *seq = (unsigned char*)malloc(seq_cap);
        if (!seq) { perror("malloc"); free(in); return 1; }

        while (hend < n && in[hend] != '>') {
            unsigned char c = in[hend++];
            if (c == '\n' || c == '\r') continue;
            if (seq_len + 1 >= seq_cap) {
                seq_cap <<= 1;
                unsigned char *ns = (unsigned char*)realloc(seq, seq_cap);
                if (!ns) { perror("realloc"); free(seq); free(in); return 1; }
                seq = ns;
            }
            // Store as-is; we will complement on write
            seq[seq_len++] = c;
        }

        // Reverse-complement and write in 60-char lines.
        // We produce UPPERCASE complements.
        size_t pos = seq_len;
        unsigned char line[LINE_LEN];
        while (pos > 0) {
            size_t take = (pos >= LINE_LEN ? LINE_LEN : pos);
            // Fill line from end of seq, complementing
            for (size_t k = 0; k < take; ++k) {
                unsigned char c = seq[pos - 1 - k];
                c = (unsigned char)toupper(c);
                line[k] = COMP[c];
            }
            if (fwrite(line, 1, take, stdout) != take) {
                perror("fwrite"); free(seq); free(in); return 1;
            }
            fputc('\n', stdout);
            pos -= take;
        }

        free(seq);
        i = hend; // continue from where we stopped (next header or EOF)
    }

    free(in);
    return 0;
}

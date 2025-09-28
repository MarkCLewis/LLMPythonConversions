// fannkuch-redux.c — C port of fannkuch-redux6.py (Computer Language Benchmarks Game)
// Build:  gcc -O3 -march=native -o fannkuch fannkuch-redux.c
// Run:    ./fannkuch 12
//
// Prints:
//   <checksum>
//   Pfannkuchen(<n>) = <maxFlips>

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static inline void reverse_prefix(int *a, int k) {
    int i = 0, j = k;
    while (i < j) { int t = a[i]; a[i] = a[j]; a[j] = t; ++i; --j; }
}

static int fannkuch(int n, long long *checksum_out) {
    int *perm1 = (int*)malloc(n * sizeof(int));
    int *count = (int*)malloc(n * sizeof(int));
    int *perm  = (int*)malloc(n * sizeof(int));
    if (!perm1 || !count || !perm) { fprintf(stderr, "oom\n"); exit(1); }

    for (int i = 0; i < n; ++i) { perm1[i] = i; count[i] = i; }

    int maxFlipsCount = 0;
    int permSign = 1;                 // True (1) / False (0) in Python
    long long checksum = 0;
    const int nm = n - 1;

    for (;;) {
        int k = perm1[0];
        if (k != 0) {
            memcpy(perm, perm1, n * sizeof(int));
            int flipsCount = 1;
            int kk = perm[k];
            while (kk != 0) {
                reverse_prefix(perm, k);
                ++flipsCount;
                k  = kk;
                kk = perm[kk];
            }
            if (maxFlipsCount < flipsCount) maxFlipsCount = flipsCount;
            checksum += (permSign ? flipsCount : -flipsCount);
        }

        // Generate next permutation (incremental change)
        if (permSign) {
            int t = perm1[0]; perm1[0] = perm1[1]; perm1[1] = t;
            permSign = 0; // False
        } else {
            int t = perm1[1]; perm1[1] = perm1[2]; perm1[2] = t;
            permSign = 1; // True

            int r;
            int broke = 0;
            for (r = 2; r < n - 1; ++r) {
                if (count[r] != 0) { broke = 1; break; }
                count[r] = r;
                int perm0 = perm1[0];
                // rotate left perm1[0..r] by 1: move perm1[1..r] down, put perm0 at r
                memmove(&perm1[0], &perm1[1], r * sizeof(int));
                perm1[r] = perm0;
            }
            if (!broke) {
                r = nm;
                if (count[r] == 0) {
                    // Done: produce checksum and return max
                    *checksum_out = checksum;
                    free(perm); free(count); free(perm1);
                    return maxFlipsCount;
                }
            }
            count[r] -= 1;
        }
    }
}

int main(int argc, char **argv) {
    if (argc < 2) { fprintf(stderr, "usage: %s <n>\n", argv[0]); return 1; }
    int n = atoi(argv[1]);
    if (n < 1) { fprintf(stderr, "n must be >= 1\n"); return 1; }

    long long checksum = 0;
    int maxFlips = fannkuch(n, &checksum);

    printf("%lld\n", checksum);
    printf("Pfannkuchen(%d) = %d\n", n, maxFlips);
    return 0;
}

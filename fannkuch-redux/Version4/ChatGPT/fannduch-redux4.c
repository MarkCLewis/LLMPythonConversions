// fannkuch-redux.c — C port of fannkuch-redux4.py (Computer Language Benchmarks Game)
// Uses factorial-number-system indexing and (optional) OpenMP parallelism.
//
// Build (with OpenMP):   gcc -O3 -fopenmp -o fannkuch fannkuch-redux.c
// Run:                   ./fannkuch 12
//
// Output matches the Python version:
//   <checksum>
//   Pfannkuchen(<n>) = <maxflips>

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>

#ifdef _OPENMP
#include <omp.h>
#endif

// Safely compute factorials up to n (n <= 12..13 typical for the benchmark)
static void factorials(int n, int64_t *fact) {
    fact[0] = 1;
    for (int i = 1; i <= n; ++i) fact[i] = fact[i - 1] * i;
}

// Initialize permutation perm[0..n-1] from factoradic index 'idx'.
// Also sets 'count' array used by the permutation generator and returns
// the sign parity ( +1 for even, -1 for odd ) for the starting permutation.
static int init_perm_from_index(int n, int64_t idx, int *perm, int *count) {
    // Start with identity
    for (int i = 0; i < n; ++i) perm[i] = i;

    // Build via factoradic: for position i, rotate the first (n-1-i)+1 elements by d
    int sign = 1;
    for (int i = n - 1; i > 0; --i) {
        int d = idx % (i + 1);
        idx /= (i + 1);
        count[i] = d;
        if (d) {
            // rotate left by d the block perm[0..i]
            sign = -sign * ((d % 2) ? 1 : -1); // rotation by odd d flips parity
            int tmp[32]; // n <= ~20 in practice; 32 is safe
            // copy
            for (int k = 0; k <= i; ++k) tmp[k] = perm[k];
            // rotated
            for (int k = 0; k <= i; ++k) perm[k] = tmp[(k + d) % (i + 1)];
        } else {
            // no change
        }
    }
    count[0] = 0;
    return sign;
}

// Compute flips for current perm (perm[0]..perm[n-1]) — classic fannkuch flips.
// Returns 0 if perm[0] == 0.
static int flips_count(const int *perm, int n) {
    int first = perm[0];
    if (first == 0) return 0;

    int flips = 0;
    int p[32]; // small on stack; n is small in benchmark
    memcpy(p, perm, n * sizeof(int));

    while (p[0] != 0) {
        int f = p[0];
        // reverse p[0..f]
        for (int lo = 0, hi = f; lo < hi; ++lo, --hi) {
            int t = p[lo]; p[lo] = p[hi]; p[hi] = t;
        }
        ++flips;
    }
    return flips;
}

// Advance to next permutation in the "tachyon" order used by fannkuch-redux,
// updating 'perm' and 'count'. Also update 'sign' (alternating factor).
static void next_permutation(int n, int *perm, int *count, int *sign) {
    // This generator rotates prefix of length i+1 and manages counters.
    if (*sign == 1) {
        // swap first two
        int t = perm[0]; perm[0] = perm[1]; perm[1] = t;
        *sign = -1;
    } else {
        // rotate left first 3: (0,1,2) -> (1,2,0)
        int t = perm[1];
        perm[1] = perm[2];
        perm[2] = t;
        *sign = 1;
        int i = 2;
        while (i < n) {
            if (count[i] >= i) {
                // restore and carry on
                // rotate left prefix i+1 by one (undo back to identity) 'i' times
                // Effectively: move perm[0] to perm[i], shift left others.
                count[i] = 0;
                int tmp = perm[0];
                for (int k = 0; k < i; ++k) perm[k] = perm[k + 1];
                perm[i] = tmp;
                ++i;
            } else {
                // rotate left first (i+1) by 1
                int tmp = perm[0];
                for (int k = 0; k < i; ++k) perm[k] = perm[k + 1];
                perm[i] = tmp;
                count[i] += 1;
                break;
            }
        }
    }
}

// Process a contiguous chunk of permutations [start, start+size)
// Returns checksum contribution and local maximum flips.
static void process_chunk(int n, int64_t start, int64_t size,
                          const int64_t *fact,
                          int64_t *checksum_out, int *maxflips_out)
{
    int perm[32], count[32];
    int sign = init_perm_from_index(n, start, perm, count);

    int local_max = 0;
    int64_t checksum = 0;

    for (int64_t i = 0; i < size; ++i) {
        int flips = flips_count(perm, n);
        if (flips > local_max) local_max = flips;
        checksum += (sign > 0 ? flips : -flips);

        next_permutation(n, perm, count, &sign);
    }

    *checksum_out = checksum;
    *maxflips_out = local_max;
}

int main(int argc, char **argv) {
    if (argc < 2) {
        fprintf(stderr, "usage: %s <n>\n", argv[0]);
        return 1;
    }
    int n = atoi(argv[1]);
    if (n <= 0 || n > 20) { // sanity limit
        fprintf(stderr, "n must be in 1..20\n");
        return 1;
    }

    int64_t fact[32];
    factorials(n, fact);
    int64_t total = fact[n];

    // Decide chunking similar to the Python version:
    // task_count = num_cores, task_size = ceil(total / task_count), but
    // if too small (< 20000) then do single chunk for better overheads.
    int task_count =
#ifdef _OPENMP
        omp_get_max_threads();
#else
        1;
#endif
    if (task_count < 1) task_count = 1;

    int64_t task_size = (total + task_count - 1) / task_count;
    if (task_size < 20000) {
        task_size = total;
        task_count = 1;
    }
    // Ensure even size (the Python generator asserts even size when >1)
    if (task_size % 2) {
        // Adjust last chunk later; keep chunk sizes even where possible
        // (not strictly required for this generator, but mirrors Python)
    }

    int64_t global_checksum = 0;
    int global_max = 0;

#ifdef _OPENMP
    #pragma omp parallel
    {
        int local_max = 0;
        int64_t local_checksum = 0;

        #pragma omp for schedule(static) nowait
        for (int t = 0; t < task_count; ++t) {
            int64_t start = (int64_t)t * task_size;
            int64_t size = task_size;
            if (start + size > total) size = total - start;
            if (size <= 0) continue;
            int64_t csum = 0; int m = 0;
            process_chunk(n, start, size, fact, &csum, &m);
            if (m > local_max) local_max = m;
            local_checksum += csum;
        }

        #pragma omp critical
        {
            if (local_max > global_max) global_max = local_max;
            global_checksum += local_checksum;
        }
    }
#else
    for (int t = 0; t < task_count; ++t) {
        int64_t start = (int64_t)t * task_size;
        int64_t size = task_size;
        if (start + size > total) size = total - start;
        if (size <= 0) continue;
        int64_t csum = 0; int m = 0;
        process_chunk(n, start, size, fact, &csum, &m);
        if (m > global_max) global_max = m;
        global_checksum += csum;
    }
#endif

    // Match the Python output format
    printf("%lld\n", (long long)global_checksum);
    printf("Pfannkuchen(%d) = %d\n", n, global_max);
    return 0;
}

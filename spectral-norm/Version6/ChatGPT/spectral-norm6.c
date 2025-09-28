// spectral-norm.c — C port of spectral-norm6.py (Computer Language Benchmarks Game)
// Build:  gcc -O3 -fopenmp -march=native -std=c11 -o spectral-norm spectral-norm.c
// Run:    ./spectral-norm 5500
//
// Prints the spectral norm with 9 digits after the decimal point.

#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#ifdef _OPENMP
#include <omp.h>
#endif

// Denominator used by the benchmark's implicit matrix:
// A[i,j] = 1.0 / ( (i+j)*(i+j+1)/2 + i + 1 )
static inline double a_den(const int i, const int j) {
    const long ij = (long)i + (long)j;
    return (double)((ij * (ij + 1)) / 2 + i + 1);
}

static void Av(const double *restrict v, double *restrict out, int n) {
    #pragma omp parallel for schedule(static)
    for (int i = 0; i < n; ++i) {
        double sum = 0.0;
        for (int j = 0; j < n; ++j) {
            sum += v[j] / a_den(i, j);
        }
        out[i] = sum;
    }
}

static void Atv(const double *restrict v, double *restrict out, int n) {
    #pragma omp parallel for schedule(static)
    for (int i = 0; i < n; ++i) {
        double sum = 0.0;
        for (int j = 0; j < n; ++j) {
            sum += v[j] / a_den(j, i);
        }
        out[i] = sum;
    }
}

static void AtAv(const double *restrict v, double *restrict out, int n,
                 double *restrict tmp) {
    // tmp = A * v
    Av(v, tmp, n);
    // out = A^T * tmp
    Atv(tmp, out, n);
}

int main(int argc, char **argv) {
    if (argc < 2) {
        fprintf(stderr, "usage: %s <n>\n", argv[0]);
        return 1;
    }
    const int n = atoi(argv[1]);
    if (n <= 0) {
        fprintf(stderr, "n must be positive\n");
        return 1;
    }

    double *u   = (double*)malloc((size_t)n * sizeof(double));
    double *v   = (double*)malloc((size_t)n * sizeof(double));
    double *tmp = (double*)malloc((size_t)n * sizeof(double));
    if (!u || !v || !tmp) {
        fprintf(stderr, "out of memory\n");
        free(u); free(v); free(tmp);
        return 1;
    }

    for (int i = 0; i < n; ++i) u[i] = 1.0;

    // 10 power iterations of (A^T A)
    for (int k = 0; k < 10; ++k) {
        AtAv(u, v, n, tmp);
        AtAv(v, u, n, tmp);
    }

    // Rayleigh quotient components
    double vBv = 0.0, vv = 0.0;
    #pragma omp parallel for reduction(+:vBv,vv) schedule(static)
    for (int i = 0; i < n; ++i) {
        vBv += u[i] * v[i];
        vv  += v[i] * v[i];
    }

    printf("%.9f\n", sqrt(vBv / vv));

    free(u); free(v); free(tmp);
    return 0;
}

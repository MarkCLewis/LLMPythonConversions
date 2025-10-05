#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <omp.h>

/**
 * @brief Evaluates the element A(i, j) of the infinite matrix A.
 * This is a direct translation of the python eval_A function.
 */
static inline double eval_A(int i, int j) {
    int ij = i + j;
    return (double)(ij * (ij + 1) / 2 + i + 1);
}

/**
 * @brief Multiplies a vector u by the matrix A. (v = A * u)
 * The outer loop is parallelized using OpenMP, similar to how Python's
 * multiprocessing pool parallelizes the A_sum calculation.
 */
void multiply_Av(int n, const double u[], double v[]) {
    #pragma omp parallel for
    for (int i = 0; i < n; i++) {
        double sum = 0.0;
        for (int j = 0; j < n; j++) {
            sum += u[j] / eval_A(i, j);
        }
        v[i] = sum;
    }
}

/**
 * @brief Multiplies a vector u by the transposed matrix A. (v = A' * u)
 * The outer loop is parallelized, similar to the At_sum calculation in Python.
 */
void multiply_Atv(int n, const double u[], double v[]) {
    #pragma omp parallel for
    for (int i = 0; i < n; i++) {
        double sum = 0.0;
        for (int j = 0; j < n; j++) {
            sum += u[j] / eval_A(j, i);
        }
        v[i] = sum;
    }
}

/**
 * @brief Computes v = A' * A * u.
 * @param tmp A pre-allocated temporary vector to store the intermediate result of A*u.
 * Passing it as a parameter avoids repeated memory allocation.
 */
void multiply_AtAv(int n, const double u[], double v[], double tmp[]) {
    multiply_Av(n, u, tmp);
    multiply_Atv(n, tmp, v);
}

int main(int argc, char *argv[]) {
    if (argc < 2) {
        printf("Usage: %s <n>\n", argv[0]);
        return 1;
    }
    int n = atoi(argv[1]);
    if (n <= 0) {
        fprintf(stderr, "Error: n must be a positive integer.\n");
        return 1;
    }

    // Note: The original Python script uses 4 processes. OpenMP will typically
    // default to the number of available cores. You can set the number of threads
    // by using the OMP_NUM_THREADS environment variable, e.g.,
    // OMP_NUM_THREADS=4 ./spectral_norm 5500
    // omp_set_num_threads(4); // Or set it programmatically

    // Allocate memory for vectors
    double *u = (double*)malloc(n * sizeof(double));
    double *v = (double*)malloc(n * sizeof(double));
    double *tmp = (double*)malloc(n * sizeof(double));

    if (!u || !v || !tmp) {
        fprintf(stderr, "Error: Memory allocation failed.\n");
        free(u); free(v); free(tmp);
        return 1;
    }

    // Initialize u vector with all 1.0s
    for (int i = 0; i < n; i++) u[i] = 1.0;

    // 10 iterations of the power method
    for (int i = 0; i < 10; i++) {
        multiply_AtAv(n, u, v, tmp);
        multiply_AtAv(n, v, u, tmp);
    }

    // Calculate the dot products u'v and v'v
    double vBv = 0.0;
    double vv = 0.0;
    #pragma omp parallel for reduction(+:vBv, vv)
    for (int i = 0; i < n; i++) {
        vBv += u[i] * v[i];
        vv += v[i] * v[i];
    }

    // Calculate and print the result
    double result = sqrt(vBv / vv);
    printf("%.9f\n", result);

    // Clean up
    free(u);
    free(v);
    free(tmp);

    return 0;
}

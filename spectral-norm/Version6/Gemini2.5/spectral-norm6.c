#include <stdio.h>
#include <stdlib.h>
#include <math.h>

// A(i,j) = 1/(((i+j)*(i+j+1)/2)+i+1)
static inline double eval_A(int i, int j) {
    // Using double for the calculation to prevent potential integer overflow with large n
    double i_plus_j = (double)i + (double)j;
    return 1.0 / (i_plus_j * (i_plus_j + 1.0) / 2.0 + (double)i + 1.0);
}

// Computes v = A * u
static void eval_A_times_u(int n, const double u[], double v[]) {
    for (int i = 0; i < n; i++) {
        double sum = 0.0;
        for (int j = 0; j < n; j++) {
            sum += eval_A(i, j) * u[j];
        }
        v[i] = sum;
    }
}

// Computes v = A' * u (A transposed)
static void eval_At_times_u(int n, const double u[], double v[]) {
    for (int i = 0; i < n; i++) {
        double sum = 0.0;
        for (int j = 0; j < n; j++) {
            sum += eval_A(j, i) * u[j]; // Note: j, i are swapped for transpose
        }
        v[i] = sum;
    }
}

// Computes v = A' * A * u using a temporary workspace vector 'w'
static void eval_AtA_times_u(int n, const double u[], double v[], double w[]) {
    eval_A_times_u(n, u, w);
    eval_At_times_u(n, w, v);
}

int main(int argc, char *argv[]) {
    if (argc < 2) {
        printf("Usage: %s <n>\n", argv[0]);
        return 1;
    }
    int n = atoi(argv[1]);
    if (n <= 0) return 1;

    // Allocate memory for vectors u, v, and a workspace vector w
    double *u = malloc(n * sizeof(double));
    double *v = malloc(n * sizeof(double));
    double *w = malloc(n * sizeof(double));

    if (u == NULL || v == NULL || w == NULL) {
        fprintf(stderr, "Memory allocation failed\n");
        free(u); free(v); free(w);
        return 1;
    }

    // Initialize u with all elements as 1.0
    for (int i = 0; i < n; i++) u[i] = 1.0;

    // The core algorithm: 10 iterations of the power method
    for (int i = 0; i < 10; i++) {
        eval_AtA_times_u(n, u, v, w);
        eval_AtA_times_u(n, v, u, w);
    }

    // Calculate dot products vBv = (u . v) and vv = (v . v)
    double vBv = 0.0, vv = 0.0;
    for (int i = 0; i < n; i++) {
        vBv += u[i] * v[i];
        vv += v[i] * v[i];
    }

    // Print the final result (the spectral norm)
    printf("%0.9f\n", sqrt(vBv / vv));

    // Free the allocated memory
    free(u);
    free(v);
    free(w);

    return 0;
}

// The Computer Language Benchmarks Game
// https://salsa.debian.org/benchmarksgame-team/benchmarksgame/
//
// Converted from Python to C by Claude

#include <stdio.h>
#include <stdlib.h>
#include <math.h>

// Calculate A[i,j] = 1/((i+j)*(i+j+1)/2+i+1)
double eval_A(int i, int j) {
    double ij = i + j;
    return 1.0 / (ij * (ij + 1) / 2 + i + 1);
}

// Calculate A * u
void eval_A_times_u(const double* u, double* result, int n) {
    for (int i = 0; i < n; i++) {
        double sum = 0.0;
        for (int j = 0; j < n; j++) {
            sum += eval_A(i, j) * u[j];
        }
        result[i] = sum;
    }
}

// Calculate A^T * u
void eval_At_times_u(const double* u, double* result, int n) {
    for (int i = 0; i < n; i++) {
        double sum = 0.0;
        for (int j = 0; j < n; j++) {
            sum += eval_A(j, i) * u[j];
        }
        result[i] = sum;
    }
}

// Calculate A^T * A * u
void eval_AtA_times_u(const double* u, double* result, double* temp, int n) {
    eval_A_times_u(u, temp, n);
    eval_At_times_u(temp, result, n);
}

int main(int argc, char* argv[]) {
    if (argc < 2) {
        fprintf(stderr, "Usage: %s <n>\n", argv[0]);
        return 1;
    }

    int n = atoi(argv[1]);
    
    // Allocate memory for vectors
    double* u = (double*)malloc(n * sizeof(double));
    double* v = (double*)malloc(n * sizeof(double));
    double* temp = (double*)malloc(n * sizeof(double));

    if (!u || !v || !temp) {
        fprintf(stderr, "Memory allocation failed\n");
        free(u);
        free(v);
        free(temp);
        return 1;
    }

    // Initialize u with ones
    for (int i = 0; i < n; i++) {
        u[i] = 1.0;
    }

    // Perform 10 iterations
    for (int i = 0; i < 10; i++) {
        eval_AtA_times_u(u, v, temp, n);
        eval_AtA_times_u(v, u, temp, n);
    }

    // Calculate vBv and vv
    double vBv = 0.0;
    double vv = 0.0;

    for (int i = 0; i < n; i++) {
        vBv += u[i] * v[i];
        vv += v[i] * v[i];
    }

    // Calculate and print result
    double result = sqrt(vBv / vv);
    printf("%.9f\n", result);

    // Clean up
    free(u);
    free(v);
    free(temp);

    return 0;
}
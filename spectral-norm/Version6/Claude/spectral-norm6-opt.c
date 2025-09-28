// The Computer Language Benchmarks Game
// https://salsa.debian.org/benchmarksgame-team/benchmarksgame/
//
// Converted from Python to C with optimizations by Claude

#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <string.h>
#include <pthread.h>

// Number of threads to use
#define NUM_THREADS 4

typedef struct {
    const double* u;
    double* result;
    int n;
    int start;
    int end;
    int mode; // 0 for A*u, 1 for A^T*u
} thread_data_t;

// Calculate A[i,j] = 1/((i+j)*(i+j+1)/2+i+1)
inline double eval_A(int i, int j) {
    double ij = i + j;
    return 1.0 / (ij * (ij + 1) / 2 + i + 1);
}

// Thread function for matrix-vector multiplication
void* multiply_worker(void* arg) {
    thread_data_t* data = (thread_data_t*)arg;
    const double* u = data->u;
    double* result = data->result;
    int n = data->n;
    int start = data->start;
    int end = data->end;
    int mode = data->mode;
    
    if (mode == 0) {
        // A * u
        for (int i = start; i < end; i++) {
            double sum = 0.0;
            for (int j = 0; j < n; j++) {
                sum += eval_A(i, j) * u[j];
            }
            result[i] = sum;
        }
    } else {
        // A^T * u
        for (int i = start; i < end; i++) {
            double sum = 0.0;
            for (int j = 0; j < n; j++) {
                sum += eval_A(j, i) * u[j];
            }
            result[i] = sum;
        }
    }
    
    return NULL;
}

// Parallel version of matrix-vector multiplication
void parallel_multiply(const double* u, double* result, int n, int mode) {
    pthread_t threads[NUM_THREADS];
    thread_data_t thread_data[NUM_THREADS];
    
    int chunk_size = (n + NUM_THREADS - 1) / NUM_THREADS;
    
    // Create threads
    for (int i = 0; i < NUM_THREADS; i++) {
        thread_data[i].u = u;
        thread_data[i].result = result;
        thread_data[i].n = n;
        thread_data[i].start = i * chunk_size;
        thread_data[i].end = (i == NUM_THREADS - 1) ? n : (i + 1) * chunk_size;
        thread_data[i].mode = mode;
        
        pthread_create(&threads[i], NULL, multiply_worker, &thread_data[i]);
    }
    
    // Join threads
    for (int i = 0; i < NUM_THREADS; i++) {
        pthread_join(threads[i], NULL);
    }
}

// Calculate A * u
void eval_A_times_u(const double* u, double* result, int n) {
    parallel_multiply(u, result, n, 0);
}

// Calculate A^T * u
void eval_At_times_u(const double* u, double* result, int n) {
    parallel_multiply(u, result, n, 1);
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
    
    // Allocate memory for vectors with cache-aligned padding
    // Use posix_memalign or aligned_alloc for better cache alignment on supported systems
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
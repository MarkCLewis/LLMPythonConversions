// The Computer Language Benchmarks Game
// https://salsa.debian.org/benchmarksgame-team/benchmarksgame/
//
// Converted from Python to C by Claude

#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <pthread.h>

typedef struct {
    double *u;
    double *result;
    int n;
    int start;
    int end;
} thread_data_t;

// Calculate A[i,j] = 1/((i+j)*(i+j+1)/2+i+1)
double eval_A(int i, int j) {
    int ij = i + j;
    return 1.0 / ((ij * (ij + 1)) / 2 + i + 1);
}

// Calculate sum(u[j] * A[i,j]) for all j
void *A_worker(void *arg) {
    thread_data_t *data = (thread_data_t *)arg;
    double *u = data->u;
    double *result = data->result;
    int n = data->n;
    
    for (int i = data->start; i < data->end; i++) {
        double sum = 0.0;
        for (int j = 0; j < n; j++) {
            sum += u[j] * eval_A(i, j);
        }
        result[i] = sum;
    }
    
    return NULL;
}

// Calculate sum(u[j] * A[j,i]) for all j
void *At_worker(void *arg) {
    thread_data_t *data = (thread_data_t *)arg;
    double *u = data->u;
    double *result = data->result;
    int n = data->n;
    
    for (int i = data->start; i < data->end; i++) {
        double sum = 0.0;
        for (int j = 0; j < n; j++) {
            sum += u[j] * eval_A(j, i);
        }
        result[i] = sum;
    }
    
    return NULL;
}

// Distribute work to threads
void parallel_multiply(double *src, double *dst, int n, void *(*worker)(void *)) {
    int num_threads = 4;  // Same as Python's Pool(processes=4)
    pthread_t threads[num_threads];
    thread_data_t thread_data[num_threads];
    
    int chunk_size = n / num_threads;
    
    // Create threads
    for (int i = 0; i < num_threads; i++) {
        thread_data[i].u = src;
        thread_data[i].result = dst;
        thread_data[i].n = n;
        thread_data[i].start = i * chunk_size;
        thread_data[i].end = (i == num_threads - 1) ? n : (i + 1) * chunk_size;
        
        pthread_create(&threads[i], NULL, worker, &thread_data[i]);
    }
    
    // Join threads
    for (int i = 0; i < num_threads; i++) {
        pthread_join(threads[i], NULL);
    }
}

// Multiply v by A, then by A^T
void multiply_AtAv(double *v, double *Av, double *AtAv, int n) {
    // Calculate Av
    parallel_multiply(v, Av, n, A_worker);
    
    // Calculate A^T(Av)
    parallel_multiply(Av, AtAv, n, At_worker);
}

int main(int argc, char *argv[]) {
    if (argc != 2) {
        fprintf(stderr, "Usage: %s <n>\n", argv[0]);
        return 1;
    }
    
    int n = atoi(argv[1]);
    
    double *u = (double *)malloc(n * sizeof(double));
    double *v = (double *)malloc(n * sizeof(double));
    double *tmp = (double *)malloc(n * sizeof(double));
    
    // Initialize u with ones
    for (int i = 0; i < n; i++) {
        u[i] = 1.0;
    }
    
    // Power iteration: 10 times
    for (int i = 0; i < 10; i++) {
        multiply_AtAv(u, tmp, v, n);
        multiply_AtAv(v, tmp, u, n);
    }
    
    // Calculate the result
    double vBv = 0.0;
    double vv = 0.0;
    
    for (int i = 0; i < n; i++) {
        vBv += u[i] * v[i];
        vv += v[i] * v[i];
    }
    
    double result = sqrt(vBv / vv);
    printf("%.9f\n", result);
    
    // Cleanup
    free(u);
    free(v);
    free(tmp);
    
    return 0;
}
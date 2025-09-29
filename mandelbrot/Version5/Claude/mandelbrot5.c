// The Computer Language Benchmarks Game
// https://salsa.debian.org/benchmarksgame-team/benchmarksgame/
//
// Converted from Python to C by Claude

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <pthread.h>
#include <unistd.h>
#include <math.h>

// Structure to hold thread arguments
typedef struct {
    int thread_id;
    int n_threads;
    int size;
    unsigned char *result;
} thread_args_t;

// Calculate one row of the Mandelbrot set
void do_row(int y, int size, unsigned char *row_result) {
    double x_scale = 2.0 / size;
    double y_scale = 2.0 / size;
    double y_offset = y * y_scale - 1.5;
    
    int byte_count = (size + 7) / 8;
    
    for (int byte_x = 0; byte_x < byte_count; byte_x++) {
        unsigned char byte_acc = 0;
        
        for (int bit = 7; bit >= 0; bit--) {
            int x = byte_x * 8 + (7 - bit);
            if (x >= size) {
                continue;
            }
            
            double cr = x * x_scale - 2.0;
            double ci = y_offset;
            
            double zr = 0.0;
            double zi = 0.0;
            double zr2 = 0.0;
            double zi2 = 0.0;
            
            int i;
            for (i = 0; i < 50; i++) {
                zi = 2.0 * zr * zi + ci;
                zr = zr2 - zi2 + cr;
                zr2 = zr * zr;
                zi2 = zi * zi;
                
                if (zr2 + zi2 > 4.0) {
                    break;
                }
            }
            
            if (i == 50) {
                byte_acc |= (1 << bit);
            }
        }
        
        row_result[byte_x] = byte_acc;
    }
}

// Thread function to compute a portion of the Mandelbrot set
void* mandelbrot_thread(void *arg) {
    thread_args_t *args = (thread_args_t*)arg;
    int size = args->size;
    int byte_count = (size + 7) / 8;
    
    // Each thread handles a subset of rows
    for (int y = args->thread_id; y < size; y += args->n_threads) {
        unsigned char *row_result = args->result + y * byte_count;
        do_row(y, size, row_result);
    }
    
    return NULL;
}

// Main function to generate the Mandelbrot set
void mandelbrot(int size, FILE *out) {
    // Write PBM header
    fprintf(out, "P4\n%d %d\n", size, size);
    
    // Calculate the size of the result buffer
    int byte_count = (size + 7) / 8;
    int result_size = size * byte_count;
    
    // Allocate memory for the result
    unsigned char *result = (unsigned char*)malloc(result_size);
    if (!result) {
        fprintf(stderr, "Failed to allocate memory\n");
        return;
    }
    
    // Get the number of available CPU cores
    int n_threads = sysconf(_SC_NPROCESSORS_ONLN);
    if (n_threads < 1) n_threads = 1;
    
    // Create thread arguments
    thread_args_t *thread_args = (thread_args_t*)malloc(n_threads * sizeof(thread_args_t));
    pthread_t *threads = (pthread_t*)malloc(n_threads * sizeof(pthread_t));
    
    if (!thread_args || !threads) {
        fprintf(stderr, "Failed to allocate memory for threads\n");
        free(result);
        free(thread_args);
        free(threads);
        return;
    }
    
    // Initialize threads
    for (int i = 0; i < n_threads; i++) {
        thread_args[i].thread_id = i;
        thread_args[i].n_threads = n_threads;
        thread_args[i].size = size;
        thread_args[i].result = result;
        
        if (pthread_create(&threads[i], NULL, mandelbrot_thread, &thread_args[i])) {
            fprintf(stderr, "Failed to create thread %d\n", i);
            n_threads = i; // Only join the threads that were created
            break;
        }
    }
    
    // Wait for all threads to complete
    for (int i = 0; i < n_threads; i++) {
        pthread_join(threads[i], NULL);
    }
    
    // Write the result to the output file
    fwrite(result, 1, result_size, out);
    
    // Clean up
    free(result);
    free(thread_args);
    free(threads);
}

int main(int argc, char *argv[]) {
    if (argc != 2) {
        fprintf(stderr, "Usage: %s <size>\n", argv[0]);
        return 1;
    }
    
    int size = atoi(argv[1]);
    if (size <= 0) {
        fprintf(stderr, "Size must be a positive integer\n");
        return 1;
    }
    
    mandelbrot(size, stdout);
    
    return 0;
}
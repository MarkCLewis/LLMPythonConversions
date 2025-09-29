// The Computer Language Benchmarks Game
// https://salsa.debian.org/benchmarksgame-team/benchmarksgame/
//
// Converted from Python to C with SIMD optimizations by Claude

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <pthread.h>
#include <unistd.h>
#include <math.h>
#include <stdbool.h>

#ifdef __SSE2__
#include <emmintrin.h>
#endif

#ifdef __AVX2__
#include <immintrin.h>
#endif

// Structure to hold thread arguments
typedef struct {
    int thread_id;
    int n_threads;
    int size;
    unsigned char *result;
} thread_args_t;

// Calculate one row of the Mandelbrot set (standard version)
void do_row_standard(int y, int size, unsigned char *row_result) {
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

#ifdef __SSE2__
// Calculate one row of the Mandelbrot set using SSE2
void do_row_sse2(int y, int size, unsigned char *row_result) {
    double x_scale = 2.0 / size;
    double y_scale = 2.0 / size;
    double y_offset = y * y_scale - 1.5;
    
    int byte_count = (size + 7) / 8;
    memset(row_result, 0, byte_count);
    
    // SSE2 version can process 2 pixels at once
    for (int x = 0; x < size; x += 2) {
        __m128d cr = _mm_set_pd(
            (x + 1) * x_scale - 2.0,
            x * x_scale - 2.0
        );
        __m128d ci = _mm_set1_pd(y_offset);
        __m128d zr = _mm_setzero_pd();
        __m128d zi = _mm_setzero_pd();
        
        int iterations[2] = {0, 0};
        bool finished[2] = {false, false};
        
        for (int i = 0; i < 50; i++) {
            // z = z * z + c
            __m128d zr2 = _mm_mul_pd(zr, zr);
            __m128d zi2 = _mm_mul_pd(zi, zi);
            __m128d zrzi = _mm_mul_pd(zr, zi);
            
            __m128d magnitudes = _mm_add_pd(zr2, zi2);
            __m128d mask = _mm_cmplt_pd(magnitudes, _mm_set1_pd(4.0));
            
            // Check if we've escaped for both pixels
            int mask_bits = _mm_movemask_pd(mask);
            if (mask_bits == 0) break;
            
            if ((mask_bits & 1) && !finished[0]) {
                iterations[0] = i + 1;
                if (i == 49) iterations[0] = 50;
                if (i == 49 || (_mm_cvtsd_f64(magnitudes) >= 4.0)) {
                    finished[0] = true;
                }
            }
            
            if ((mask_bits & 2) && !finished[1]) {
                iterations[1] = i + 1;
                if (i == 49) iterations[1] = 50;
                if (i == 49 || (_mm_cvtsd_f64(_mm_unpackhi_pd(magnitudes, magnitudes)) >= 4.0)) {
                    finished[1] = true;
                }
            }
            
            if (finished[0] && finished[1]) break;
            
            // Calculate new z values
            zi = _mm_add_pd(_mm_mul_pd(_mm_set1_pd(2.0), zrzi), ci);
            zr = _mm_add_pd(_mm_sub_pd(zr2, zi2), cr);
        }
        
        // Set bits for pixels in Mandelbrot set
        if (iterations[0] == 50) {
            int byte_offset = x / 8;
            int bit_offset = 7 - (x % 8);
            row_result[byte_offset] |= (1 << bit_offset);
        }
        
        if (x + 1 < size && iterations[1] == 50) {
            int byte_offset = (x + 1) / 8;
            int bit_offset = 7 - ((x + 1) % 8);
            row_result[byte_offset] |= (1 << bit_offset);
        }
    }
}
#endif

#ifdef __AVX2__
// Calculate one row of the Mandelbrot set using AVX2
void do_row_avx2(int y, int size, unsigned char *row_result) {
    double x_scale = 2.0 / size;
    double y_scale = 2.0 / size;
    double y_offset = y * y_scale - 1.5;
    
    int byte_count = (size + 7) / 8;
    memset(row_result, 0, byte_count);
    
    // AVX2 version can process 4 pixels at once
    for (int x = 0; x < size; x += 4) {
        __m256d cr = _mm256_set_pd(
            (x + 3) * x_scale - 2.0,
            (x + 2) * x_scale - 2.0,
            (x + 1) * x_scale - 2.0,
            x * x_scale - 2.0
        );
        __m256d ci = _mm256_set1_pd(y_offset);
        __m256d zr = _mm256_setzero_pd();
        __m256d zi = _mm256_setzero_pd();
        
        int iterations[4] = {0, 0, 0, 0};
        bool finished[4] = {false, false, false, false};
        
        for (int i = 0; i < 50; i++) {
            // z = z * z + c
            __m256d zr2 = _mm256_mul_pd(zr, zr);
            __m256d zi2 = _mm256_mul_pd(zi, zi);
            __m256d zrzi = _mm256_mul_pd(zr, zi);
            
            __m256d magnitudes = _mm256_add_pd(zr2, zi2);
            __m256d mask = _mm256_cmp_pd(magnitudes, _mm256_set1_pd(4.0), _CMP_LT_OQ);
            
            // Check if we've escaped for all pixels
            int mask_bits = _mm256_movemask_pd(mask);
            if (mask_bits == 0) break;
            
            // Extract current magnitudes
            double mags[4];
            _mm256_storeu_pd(mags, magnitudes);
            
            for (int j = 0; j < 4; j++) {
                if ((mask_bits & (1 << j)) && !finished[j]) {
                    iterations[j] = i + 1;
                    if (i == 49) iterations[j] = 50;
                    if (i == 49 || mags[j] >= 4.0) {
                        finished[j] = true;
                    }
                }
            }
            
            if (finished[0] && finished[1] && finished[2] && finished[3]) break;
            
            // Calculate new z values
            zi = _mm256_add_pd(_mm256_mul_pd(_mm256_set1_pd(2.0), zrzi), ci);
            zr = _mm256_add_pd(_mm256_sub_pd(zr2, zi2), cr);
        }
        
        // Set bits for pixels in Mandelbrot set
        for (int j = 0; j < 4; j++) {
            if (x + j < size && iterations[j] == 50) {
                int byte_offset = (x + j) / 8;
                int bit_offset = 7 - ((x + j) % 8);
                row_result[byte_offset] |= (1 << bit_offset);
            }
        }
    }
}
#endif

// Choose the best implementation based on available CPU features
void do_row(int y, int size, unsigned char *row_result) {
#ifdef __AVX2__
    do_row_avx2(y, size, row_result);
#elif defined(__SSE2__)
    do_row_sse2(y, size, row_result);
#else
    do_row_standard(y, size, row_result);
#endif
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
    unsigned char *result = (unsigned char*)calloc(result_size, 1);
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
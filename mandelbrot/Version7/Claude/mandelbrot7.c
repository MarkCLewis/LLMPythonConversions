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
#include <complex.h>

// Structure to hold thread arguments
typedef struct {
    int y;           // Row to compute
    int n;           // Image size
    uint8_t* result; // Buffer for result
} RowTask;

// Structure to manage row ordering
typedef struct {
    int n;                  // Image size
    uint8_t** rows;         // Array of row buffers
    pthread_mutex_t mutex;  // Mutex for thread safety
    int next_row_to_output; // Next row to be output
    int rows_completed;     // Count of completed rows
    pthread_cond_t row_ready; // Condition variable for signaling
} RowManager;

// Initialize the row manager
void init_row_manager(RowManager* manager, int n) {
    manager->n = n;
    manager->rows = (uint8_t**)calloc(n, sizeof(uint8_t*));
    manager->next_row_to_output = 0;
    manager->rows_completed = 0;
    pthread_mutex_init(&manager->mutex, NULL);
    pthread_cond_init(&manager->row_ready, NULL);
}

// Clean up the row manager
void destroy_row_manager(RowManager* manager) {
    // Free any rows that might not have been output
    for (int i = 0; i < manager->n; i++) {
        free(manager->rows[i]);
    }
    free(manager->rows);
    
    pthread_mutex_destroy(&manager->mutex);
    pthread_cond_destroy(&manager->row_ready);
}

// Compute pixels for a row
uint8_t* compute_pixels(int y, int n) {
    int byte_count = (n + 7) / 8;
    uint8_t* row = (uint8_t*)calloc(byte_count, sizeof(uint8_t));
    
    double c1 = 2.0 / n;
    double complex c0 = -1.5 + I * (y * c1 - 1.0);
    
    // Pixel bit patterns for each position in a byte
    uint8_t pixel_bits[8] = {128, 64, 32, 16, 8, 4, 2, 1};
    
    for (int x = 0; x < n; x += 8) {
        uint8_t pixel = 0;
        double complex c = x * c1 + c0;
        
        for (int bit = 0; bit < 8 && x + bit < n; bit++) {
            double complex z = c;
            int i;
            
            // Unroll the loop for better performance
            // The original Python has 7 iterations of 7 iterations = 49 total
            for (i = 0; i < 49; i++) {
                z = z * z + c;
                if (cabs(z) >= 2.0) break;
            }
            
            // If we didn't escape, set the corresponding bit
            if (i == 49) {
                pixel |= pixel_bits[bit];
            }
            
            c += c1;
        }
        
        // Store the computed byte
        row[x/8] = pixel;
    }
    
    // Clear any unused bits in the last byte
    if (n % 8 != 0) {
        row[byte_count-1] &= 0xff << (8 - n % 8);
    }
    
    return row;
}

// Compute a row and store it in the row manager
void* compute_row(void* arg) {
    RowTask* task = (RowTask*)arg;
    int y = task->y;
    int n = task->n;
    
    // Compute the row
    uint8_t* row = compute_pixels(y, n);
    
    // Store the result in the task's result buffer
    memcpy(task->result, row, (n + 7) / 8);
    
    // Free temporary buffer
    free(row);
    
    // Thread is done with this task
    free(task);
    return NULL;
}

// Thread pool worker function
void* worker_thread(void* arg) {
    RowManager* manager = (RowManager*)arg;
    int n = manager->n;
    
    while (1) {
        // Get the next row to compute
        int y;
        pthread_mutex_lock(&manager->mutex);
        
        // Find an uncomputed row
        y = manager->rows_completed;
        if (y >= n) {
            // All rows have been assigned
            pthread_mutex_unlock(&manager->mutex);
            break;
        }
        
        // Mark this row as being computed
        manager->rows_completed++;
        pthread_mutex_unlock(&manager->mutex);
        
        // Allocate memory for the row result
        int byte_count = (n + 7) / 8;
        uint8_t* row_result = (uint8_t*)malloc(byte_count);
        
        // Compute the row
        RowTask task = {y, n, row_result};
        compute_row(&task);
        
        // Store the result and signal that a row is ready
        pthread_mutex_lock(&manager->mutex);
        manager->rows[y] = row_result;
        pthread_cond_signal(&manager->row_ready);
        pthread_mutex_unlock(&manager->mutex);
    }
    
    return NULL;
}

// Output thread function
void* output_thread(void* arg) {
    RowManager* manager = (RowManager*)arg;
    int n = manager->n;
    int byte_count = (n + 7) / 8;
    
    // Write PBM header
    printf("P4\n%d %d\n", n, n);
    
    // Output rows in order
    while (manager->next_row_to_output < n) {
        pthread_mutex_lock(&manager->mutex);
        
        // Wait until the next row is ready
        while (manager->next_row_to_output < n && 
               manager->rows[manager->next_row_to_output] == NULL) {
            pthread_cond_wait(&manager->row_ready, &manager->mutex);
        }
        
        if (manager->next_row_to_output < n) {
            uint8_t* row = manager->rows[manager->next_row_to_output];
            manager->rows[manager->next_row_to_output] = NULL; // Mark as output
            manager->next_row_to_output++;
            
            pthread_mutex_unlock(&manager->mutex);
            
            // Write the row to stdout
            fwrite(row, 1, byte_count, stdout);
            free(row);
        } else {
            pthread_mutex_unlock(&manager->mutex);
        }
    }
    
    return NULL;
}

int main(int argc, char* argv[]) {
    // Check command line arguments
    if (argc != 2) {
        fprintf(stderr, "Usage: %s <size>\n", argv[0]);
        return 1;
    }
    
    // Parse image size
    int n = atoi(argv[1]);
    if (n <= 0) {
        fprintf(stderr, "Size must be a positive integer\n");
        return 1;
    }
    
    // Initialize row manager
    RowManager manager;
    init_row_manager(&manager, n);
    
    // Determine number of threads to use (number of CPU cores)
    int num_threads = sysconf(_SC_NPROCESSORS_ONLN);
    if (num_threads <= 0) num_threads = 1;
    
    // Create worker threads
    pthread_t* threads = (pthread_t*)malloc(num_threads * sizeof(pthread_t));
    for (int i = 0; i < num_threads; i++) {
        pthread_create(&threads[i], NULL, worker_thread, &manager);
    }
    
    // Create output thread
    pthread_t output_thread_id;
    pthread_create(&output_thread_id, NULL, output_thread, &manager);
    
    // Wait for worker threads to complete
    for (int i = 0; i < num_threads; i++) {
        pthread_join(threads[i], NULL);
    }
    
    // Wait for output thread
    pthread_join(output_thread_id, NULL);
    
    // Clean up
    destroy_row_manager(&manager);
    free(threads);
    
    return 0;
}
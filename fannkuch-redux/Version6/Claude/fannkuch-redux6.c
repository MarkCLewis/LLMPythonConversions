/* The Computer Language Benchmarks Game
 * https://salsa.debian.org/benchmarksgame-team/benchmarksgame/
 *
 * Converted from Python to C by Claude
 * Based on the Python version initially contributed by Isaac Gouy
 * and modified by Justin Peel
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>
#include <unistd.h>

// Thread task arguments
typedef struct {
    int n;                  // Size of permutation
    int start;              // Start index for this thread
    int chunk_size;         // Number of permutations to process
    int checksum;           // Partial checksum result
    int max_flips;          // Maximum flips found
} task_t;

// Recursive factorial function (for small n)
int factorial(int n) {
    if (n <= 1) return 1;
    return n * factorial(n - 1);
}

// Implementation of fannkuch function for a specific range of permutations
void* fannkuch_task(void* arg) {
    task_t* task = (task_t*)arg;
    int n = task->n;
    int max_flips_count = 0;
    int checksum = 0;
    
    // Allocate arrays for permutation operations
    int* perm1 = (int*)malloc(n * sizeof(int));
    int* perm = (int*)malloc(n * sizeof(int));
    int* count = (int*)malloc(n * sizeof(int));
    
    // Initialize first permutation
    for (int i = 0; i < n; i++) {
        perm1[i] = i;
        count[i] = i;
    }
    
    // Skip to starting permutation for this thread
    int remaining = task->start;
    int r;
    
    for (int i = 1; i < n && remaining > 0; i++) {
        int d = remaining / factorial(n - i);
        remaining = remaining % factorial(n - i);
        count[i] = d;
        
        // Apply count[i] rotations of elements 0..i
        int first = perm1[0];
        for (int j = 0; j < i; j++) {
            perm1[j] = perm1[j + 1];
        }
        perm1[i] = first;
        
        for (int j = 1; j < d; j++) {
            first = perm1[0];
            for (int k = 0; k < i; k++) {
                perm1[k] = perm1[k + 1];
            }
            perm1[i] = first;
        }
    }
    
    int perm_sign = (task->start % 2 == 0) ? 1 : 0;  // True if even number of permutations processed
    int permutation_count = 0;
    
    // Process permutations
    while (permutation_count < task->chunk_size) {
        // Check if first element is not 0
        int k = perm1[0];
        if (k != 0) {
            // Copy the permutation for flipping
            for (int i = 0; i < n; i++) {
                perm[i] = perm1[i];
            }
            
            int flips_count = 1;
            int kk = perm[k];
            
            // Perform flips until first element is 0
            while (kk != 0) {
                // Reverse the first k+1 elements
                for (int i = 0; i <= k/2; i++) {
                    int temp = perm[i];
                    perm[i] = perm[k - i];
                    perm[k - i] = temp;
                }
                
                flips_count++;
                k = kk;
                kk = perm[kk];
            }
            
            // Update max flips if needed
            if (max_flips_count < flips_count) {
                max_flips_count = flips_count;
            }
            
            // Update checksum (add or subtract based on permSign)
            checksum += perm_sign ? flips_count : -flips_count;
        }
        
        // Generate next permutation
        permutation_count++;
        
        if (permutation_count >= task->chunk_size) {
            break;
        }
        
        if (perm_sign) {
            // Swap first two elements
            int temp = perm1[0];
            perm1[0] = perm1[1];
            perm1[1] = temp;
            perm_sign = 0;  // Now false
        } else {
            // Swap second and third elements
            int temp = perm1[1];
            perm1[1] = perm1[2];
            perm1[2] = temp;
            perm_sign = 1;  // Now true
            
            // Find next position to rotate
            for (r = 2; r < n - 1; r++) {
                if (count[r] != 0) {
                    break;
                }
                count[r] = r;
                
                // Rotate elements 0..r+1
                int perm0 = perm1[0];
                for (int i = 0; i <= r; i++) {
                    perm1[i] = perm1[i + 1];
                }
                perm1[r + 1] = perm0;
            }
            
            // Check if we've gone through all permutations
            if (r == n - 1) {
                // Last position
                if (count[r] == 0) {
                    break;  // Done with all permutations
                }
            }
            
            // Decrement count at position r
            count[r]--;
        }
    }
    
    // Store results in the task structure
    task->checksum = checksum;
    task->max_flips = max_flips_count;
    
    // Free allocated memory
    free(perm1);
    free(perm);
    free(count);
    
    return NULL;
}

int main(int argc, char** argv) {
    if (argc < 2) {
        fprintf(stderr, "Usage: %s N\n", argv[0]);
        return 1;
    }
    
    int n = atoi(argv[1]);
    
    // Get number of available CPU cores
    int num_threads = sysconf(_SC_NPROCESSORS_ONLN);
    if (num_threads <= 0) num_threads = 1;
    
    // Calculate total number of permutations
    int total_permutations = factorial(n);
    
    // Limit thread count based on problem size
    if (num_threads > total_permutations) {
        num_threads = total_permutations;
    }
    
    // Create tasks and threads
    task_t* tasks = (task_t*)malloc(num_threads * sizeof(task_t));
    pthread_t* threads = (pthread_t*)malloc(num_threads * sizeof(pthread_t));
    
    // Base chunk size
    int chunk_size = total_permutations / num_threads;
    int remainder = total_permutations % num_threads;
    
    int start_idx = 0;
    for (int i = 0; i < num_threads; i++) {
        // Calculate chunk size for this thread (distribute remainder)
        int this_chunk = chunk_size + (i < remainder ? 1 : 0);
        
        // Initialize task
        tasks[i].n = n;
        tasks[i].start = start_idx;
        tasks[i].chunk_size = this_chunk;
        tasks[i].checksum = 0;
        tasks[i].max_flips = 0;
        
        // Start thread
        pthread_create(&threads[i], NULL, fannkuch_task, &tasks[i]);
        
        // Update starting index for next thread
        start_idx += this_chunk;
    }
    
    // Wait for all threads to complete
    int total_checksum = 0;
    int max_flips = 0;
    
    for (int i = 0; i < num_threads; i++) {
        pthread_join(threads[i], NULL);
        
        // Accumulate results
        total_checksum += tasks[i].checksum;
        if (tasks[i].max_flips > max_flips) {
            max_flips = tasks[i].max_flips;
        }
    }
    
    // Print results
    printf("%d\n", total_checksum);
    printf("Pfannkuchen(%d) = %d\n", n, max_flips);
    
    // Clean up
    free(tasks);
    free(threads);
    
    return 0;
}
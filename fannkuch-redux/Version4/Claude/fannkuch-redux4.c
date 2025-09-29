/* The Computer Language Benchmarks Game
 * https://salsa.debian.org/benchmarksgame-team/benchmarksgame/
 *
 * Converted from Python to C by Claude
 * Based on the Python version contributed by Joerg Baumann
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <pthread.h>
#include <unistd.h>

typedef struct {
    int n;
    int64_t start;
    int64_t size;
    int64_t checksum;
    int max_flips;
} TaskArgs;

/* Calculate factorial */
int64_t factorial(int n) {
    int64_t result = 1;
    for (int i = 2; i <= n; i++) {
        result *= i;
    }
    return result;
}

/* Worker function for each thread */
void* task_function(void* arg) {
    TaskArgs* args = (TaskArgs*)arg;
    int n = args->n;
    int64_t start = args->start;
    int64_t size = args->size;
    
    /* Arrays for permutation operations */
    unsigned char* p = (unsigned char*)malloc(n);
    unsigned char* pp = (unsigned char*)malloc(n);
    unsigned char* count = (unsigned char*)calloc(n, 1);

    /* Initialize permutation */
    for (int i = 0; i < n; i++) {
        p[i] = i;
    }
    
    /* Apply initial permutation based on start index */
    int64_t remainder = start;
    for (int i = n - 1; i > 0; i--) {
        int64_t d = remainder / factorial(i);
        remainder = remainder % factorial(i);
        count[i] = d;
        
        /* Rotate the first i+1 elements d times */
        for (int j = 0; j < d; j++) {
            unsigned char first = p[0];
            memmove(p, p + 1, i);
            p[i] = first;
        }
    }
    
    int64_t checksum = 0;
    int max_flips = 0;
    int alternating_factor = 1;
    
    /* Main loop for permutation generation and flip counting */
    for (int64_t idx = 0; idx < size; idx++) {
        /* Get first element and calculate flips */
        unsigned char first = p[0];
        if (first > 0) {
            /* Count flips */
            memcpy(pp, p, n);
            int flips = 1;
            
            while (1) {
                /* Reverse first+1 elements */
                for (int i = 0; i <= first / 2; i++) {
                    unsigned char temp = pp[i];
                    pp[i] = pp[first - i];
                    pp[first - i] = temp;
                }
                
                /* Get new first element */
                first = pp[0];
                if (first == 0) break;
                flips++;
            }
            
            /* Update max flips and checksum */
            if (max_flips < flips) max_flips = flips;
            checksum += flips * alternating_factor;
        } else {
            /* First element is 0, no flips needed */
            checksum += 0;
        }
        
        /* Flip sign for alternating sum */
        alternating_factor = -alternating_factor;
        
        /* Generate next permutation */
        if (idx < size - 1) {
            /* Swap first two elements */
            unsigned char temp = p[0];
            p[0] = p[1];
            p[1] = temp;
            
            /* Generate next permutation using optimized approach */
            int i = 2;
            while (count[i] >= i) {
                count[i] = 0;
                i++;
            }
            
            count[i]++;
            
            /* Rotate the first i+1 elements */
            unsigned char first = p[0];
            memmove(p, p + 1, i);
            p[i] = first;
        }
    }
    
    args->checksum = checksum;
    args->max_flips = max_flips;
    
    free(p);
    free(pp);
    free(count);
    
    return NULL;
}

void print_permutation(unsigned char* p, int n) {
    for (int i = 0; i < n; i++) {
        printf("%d", p[i] + 1);
    }
    printf("\n");
}

void generate_permutations(int n) {
    unsigned char* p = (unsigned char*)malloc(n);
    unsigned char* count = (unsigned char*)calloc(n, 1);
    
    /* Initialize permutation */
    for (int i = 0; i < n; i++) {
        p[i] = i;
    }
    
    int64_t total = factorial(n);
    for (int64_t idx = 0; idx < total; idx++) {
        print_permutation(p, n);
        
        /* Generate next permutation */
        int i = 1;
        while (i < n && count[i] >= i) {
            count[i] = 0;
            i++;
        }
        
        if (i < n) {
            count[i]++;
            
            /* Rotate the first i+1 elements */
            unsigned char first = p[0];
            memmove(p, p + 1, i);
            p[i] = first;
        }
    }
    
    free(p);
    free(count);
}

int main(int argc, char* argv[]) {
    if (argc < 2) {
        fprintf(stderr, "Usage: %s N\n", argv[0]);
        return 1;
    }
    
    int n = atoi(argv[1]);
    
    if (n < 0) {
        /* Print all permutations for -n */
        generate_permutations(-n);
    } else if (n > 0) {
        int64_t total = factorial(n);
        
        /* Get number of CPU cores */
        int cpu_count = sysconf(_SC_NPROCESSORS_ONLN);
        if (cpu_count <= 0) cpu_count = 1;
        
        /* Calculate task size and count */
        int64_t task_size = (total + cpu_count - 1) / cpu_count;
        if (task_size < 20000) {
            task_size = total;
            cpu_count = 1;
        }
        
        /* Ensure task_size is even */
        task_size = (task_size + 1) & ~1;
        
        int task_count = (total + task_size - 1) / task_size;
        if (task_count > cpu_count) task_count = cpu_count;
        
        /* Create tasks */
        TaskArgs* tasks = (TaskArgs*)malloc(task_count * sizeof(TaskArgs));
        pthread_t* threads = (pthread_t*)malloc(task_count * sizeof(pthread_t));
        
        for (int i = 0; i < task_count; i++) {
            tasks[i].n = n;
            tasks[i].start = i * task_size;
            tasks[i].size = (i == task_count - 1) ? (total - i * task_size) : task_size;
            tasks[i].checksum = 0;
            tasks[i].max_flips = 0;
            
            /* Create thread for task */
            if (pthread_create(&threads[i], NULL, task_function, &tasks[i]) != 0) {
                fprintf(stderr, "Failed to create thread %d\n", i);
                return 1;
            }
        }
        
        /* Wait for all threads to complete */
        for (int i = 0; i < task_count; i++) {
            pthread_join(threads[i], NULL);
        }
        
        /* Compute final results */
        int64_t checksum = 0;
        int max_flips = 0;
        
        for (int i = 0; i < task_count; i++) {
            checksum += tasks[i].checksum;
            if (tasks[i].max_flips > max_flips) {
                max_flips = tasks[i].max_flips;
            }
        }
        
        /* Print results */
        printf("%ld\nPfannkuchen(%d) = %d\n", checksum, n, max_flips);
        
        free(tasks);
        free(threads);
    }
    
    return 0;
}
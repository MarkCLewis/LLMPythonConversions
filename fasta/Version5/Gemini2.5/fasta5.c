#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>
#include <unistd.h> // For sysconf
#include <math.h>

#define LINE_WIDTH 60
#define IM 139968.0f
#define IA 3877.0f
#define IC 29573.0f

// --- Data Structures ---

typedef struct {
    char c;
    float p;
} aminoacid;

// Global shared state for the LCG random number generator
volatile float g_seed = 42.0f;

// Mutexes to replicate the Python script's locking logic
pthread_mutex_t g_seed_mutex;
pthread_mutex_t g_write_mutex_1;
pthread_mutex_t g_write_mutex_2;

// --- Task 1: Repeating Sequence ---

const char alu[] =
    "GGCCGGGCGCGGTGGCTCACGCCTGTAATCCCAGCACTTTGGGAGGCCGAGGCGGGCGGA"
    "TCACCTGAGGTCAGGAGTTCGAGACCAGCCTGGCCAACATGGTGAAACCCCGTCTCTACT"
    "AAAAATACAAAAATTAGCCGGGCGTGGTGGCGCGCGCCTGTAATCCCAGCTACTCGGGAG"
    "GCTGAGGCAGGAGAATCGCTTGAACCCGGGAGGCGGAGGTTGCAGTGAGCCGAGATCGCG"
    "CCACTGCACTCCAGCCTGGGCGACAGAGCGAGACTCCGTCTCAAAAA";

void repeat_fasta(const char *header, const char *seq, int n) {
    printf("%s", header);
    const int seq_len = strlen(seq);
    
    // Allocate a buffer to build the output string for efficient writing
    // Size = n chars + newlines + null terminator
    int buffer_size = n + (n / LINE_WIDTH) + 2; 
    char *buffer = malloc(buffer_size);
    if (!buffer) {
        perror("Failed to allocate buffer");
        exit(1);
    }
    
    int col = 0;
    int buffer_idx = 0;
    for (int i = 0; i < n; ++i) {
        buffer[buffer_idx++] = seq[i % seq_len];
        if (++col == LINE_WIDTH) {
            buffer[buffer_idx++] = '\n';
            col = 0;
        }
    }
    if (col > 0) { // Add final newline if needed
        buffer[buffer_idx++] = '\n';
    }
    buffer[buffer_idx] = '\0';

    fwrite(buffer, 1, buffer_idx, stdout);
    free(buffer);
}

// --- Task 2 & 3: Random Sequence ---

// Helper to convert probability tables to cumulative form
void make_cumulative(aminoacid *a, int count) {
    float cp = 0.0f;
    for (int i = 0; i < count; ++i) {
        cp += a[i].p;
        a[i].p = cp;
    }
}

void random_fasta(const char *header, aminoacid *a, int count, int n) {
    printf("%s", header);

    int buffer_size = n + (n / LINE_WIDTH) + 2;
    char *buffer = malloc(buffer_size);
    if (!buffer) {
        perror("Failed to allocate buffer");
        exit(1);
    }
    
    int col = 0;
    int buffer_idx = 0;

    for (int i = 0; i < n; ++i) {
        // Update seed using the LCG formula
        g_seed = fmodf((g_seed * IA + IC), IM);
        float r = g_seed / IM;

        // Find character by searching cumulative probabilities
        char c = a[count - 1].c; // Default to last
        for(int j = 0; j < count; ++j) {
            if (r < a[j].p) {
                c = a[j].c;
                break;
            }
        }
        
        buffer[buffer_idx++] = c;
        if (++col == LINE_WIDTH) {
            buffer[buffer_idx++] = '\n';
            col = 0;
        }
    }
    if (col > 0) {
        buffer[buffer_idx++] = '\n';
    }
    buffer[buffer_idx] = '\0';
    
    fwrite(buffer, 1, buffer_idx, stdout);
    free(buffer);
}


// --- Threading Wrappers ---

// Struct to pass arguments to each thread
typedef struct {
    int task_id;
    int n;
    aminoacid *a;
    int a_count;
} thread_args;

void* thread_worker(void* args) {
    thread_args* t_args = (thread_args*)args;

    switch(t_args->task_id) {
        case 1: // Task ONE: repeat_fasta
            // No pre-lock, post-lock is write_mutex_1
            repeat_fasta(">ONE Homo sapiens alu\n", alu, t_args->n * 2);
            pthread_mutex_unlock(&g_write_mutex_1);
            break;

        case 2: // Task TWO: random_fasta IUB
            // Pre-locks are write_mutex_1 (for output order) and seed_mutex (before generating)
            // Post-locks are write_mutex_2 (after writing) and seed_mutex (after generating)
            pthread_mutex_lock(&g_write_mutex_1); // Wait for task 1 to finish writing
            
            pthread_mutex_lock(&g_seed_mutex); // Lock seed for update
            random_fasta(">TWO IUB ambiguity codes\n", t_args->a, t_args->a_count, t_args->n * 3);
            pthread_mutex_unlock(&g_seed_mutex); // Release seed for task 3
            
            pthread_mutex_unlock(&g_write_mutex_2); // Allow task 3 to write
            break;
            
        case 3: // Task THREE: random_fasta Homo sapiens
            // Pre-locks are seed_mutex (to wait for task 2's seed update) and write_mutex_2 (for output order)
            pthread_mutex_lock(&g_seed_mutex); // Wait for task 2 to finish with seed
            pthread_mutex_lock(&g_write_mutex_2); // Wait for task 2 to finish writing
            
            random_fasta(">THREE Homo sapiens frequency\n", t_args->a, t_args->a_count, t_args->n * 5);
            
            pthread_mutex_unlock(&g_seed_mutex); // Not strictly needed but good practice
            pthread_mutex_unlock(&g_write_mutex_2);
            break;
    }
    free(t_args);
    return NULL;
}


// --- Main ---

int main(int argc, char **argv) {
    if (argc < 2) {
        fprintf(stderr, "Usage: %s <n>\n", argv[0]);
        return 1;
    }
    int n = atoi(argv[1]);
    
    // --- Data Initialization ---
    aminoacid iub[] = {
        {'a', 0.27}, {'c', 0.12}, {'g', 0.12}, {'t', 0.27}, {'B', 0.02},
        {'D', 0.02}, {'H', 0.02}, {'K', 0.02}, {'M', 0.02}, {'N', 0.02},
        {'R', 0.02}, {'S', 0.02}, {'V', 0.02}, {'W', 0.02}, {'Y', 0.02}
    };
    const int iub_count = sizeof(iub) / sizeof(iub[0]);
    make_cumulative(iub, iub_count);

    aminoacid homosapiens[] = {
        {'a', 0.3029549426680}, {'c', 0.1979883004921},
        {'g', 0.1975473066391}, {'t', 0.3015094502008}
    };
    const int homosapiens_count = sizeof(homosapiens) / sizeof(homosapiens[0]);
    make_cumulative(homosapiens, homosapiens_count);
    
    // --- Execution Logic ---
    long nprocs = sysconf(_SC_NPROCESSORS_ONLN);

    if (nprocs < 2) {
        // Sequential execution
        repeat_fasta(">ONE Homo sapiens alu\n", alu, n * 2);
        random_fasta(">TWO IUB ambiguity codes\n", iub, iub_count, n * 3);
        random_fasta(">THREE Homo sapiens frequency\n", homosapiens, homosapiens_count, n * 5);
    } else {
        // Parallel execution with pthreads
        pthread_t tid1, tid2, tid3;

        pthread_mutex_init(&g_seed_mutex, NULL);
        pthread_mutex_init(&g_write_mutex_1, NULL);
        pthread_mutex_init(&g_write_mutex_2, NULL);

        // Lock mutexes initially to match Python's acquired_lock()
        pthread_mutex_lock(&g_write_mutex_1);
        pthread_mutex_lock(&g_write_mutex_2);
        
        // --- Create threads ---
        thread_args *args1 = malloc(sizeof(thread_args));
        args1->task_id = 1; args1->n = n;
        pthread_create(&tid1, NULL, thread_worker, args1);
        
        thread_args *args2 = malloc(sizeof(thread_args));
        args2->task_id = 2; args2->n = n; args2->a = iub; args2->a_count = iub_count;
        pthread_create(&tid2, NULL, thread_worker, args2);

        thread_args *args3 = malloc(sizeof(thread_args));
        args3->task_id = 3; args3->n = n; args3->a = homosapiens; args3->a_count = homosapiens_count;
        pthread_create(&tid3, NULL, thread_worker, args3);

        // --- Wait for threads to finish ---
        pthread_join(tid1, NULL);
        pthread_join(tid2, NULL);
        pthread_join(tid3, NULL);

        // --- Clean up ---
        pthread_mutex_destroy(&g_seed_mutex);
        pthread_mutex_destroy(&g_write_mutex_1);
        pthread_mutex_destroy(&g_write_mutex_2);
    }

    return 0;
}

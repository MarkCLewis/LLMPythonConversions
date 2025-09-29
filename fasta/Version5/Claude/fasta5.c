/*
 * The Computer Language Benchmarks Game
 * https://salsa.debian.org/benchmarksgame-team/benchmarksgame/
 *
 * Converted from Python to C by Claude
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>
#include <unistd.h>

#define LINE_WIDTH 60
#define LINES_PER_BLOCK 10000
#define BUFFER_SIZE (LINE_WIDTH * LINES_PER_BLOCK + LINES_PER_BLOCK)

// Define the ALU sequence
const char *ALU =
    "GGCCGGGCGCGGTGGCTCACGCCTGTAATCCCAGCACTTTGGGAGGCCGAGGCGGGCGGA"
    "TCACCTGAGGTCAGGAGTTCGAGACCAGCCTGGCCAACATGGTGAAACCCCGTCTCTACT"
    "AAAAATACAAAAATTAGCCGGGCGTGGTGGCGCGCGCCTGTAATCCCAGCTACTCGGGAG"
    "GCTGAGGCAGGAGAATCGCTTGAACCCGGGAGGCGGAGGTTGCAGTGAGCCGAGATCGCG"
    "CCACTGCACTCCAGCCTGGGCGACAGAGCGAGACTCCGTCTCAAAAA";

// Frequency tables
typedef struct {
    char symbol;
    double probability;
} Frequency;

Frequency IUB[] = {
    {'a', 0.27}, {'c', 0.12}, {'g', 0.12}, {'t', 0.27},
    {'B', 0.02}, {'D', 0.02}, {'H', 0.02}, {'K', 0.02},
    {'M', 0.02}, {'N', 0.02}, {'R', 0.02}, {'S', 0.02},
    {'V', 0.02}, {'W', 0.02}, {'Y', 0.02}
};

Frequency HOMOSAPIENS[] = {
    {'a', 0.3029549426680},
    {'c', 0.1979883004921},
    {'g', 0.1975473066391},
    {'t', 0.3015094502008}
};

// LCG parameters
#define IM 139968.0
#define IA 3877.0
#define IC 29573.0

// Thread synchronization
pthread_mutex_t stdout_mutex = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t seed_mutex = PTHREAD_MUTEX_INITIALIZER;
double seed = 42.0;

// Calculate cumulative probabilities
void calculate_cumulative(Frequency *freqs, int n, double *cumulative) {
    double sum = 0.0;
    for (int i = 0; i < n; i++) {
        sum += freqs[i].probability;
        cumulative[i] = sum;
    }
}

// Binary search to find appropriate symbol
char lookup_symbol(double random, double *cumulative, Frequency *freqs, int n) {
    int low = 0;
    int high = n - 1;
    
    while (low <= high) {
        int mid = (low + high) / 2;
        if (random < cumulative[mid])
            high = mid - 1;
        else
            low = mid + 1;
    }
    
    return freqs[low].symbol;
}

// Write output buffer to stdout
void write_output(char *buffer, size_t size) {
    pthread_mutex_lock(&stdout_mutex);
    fwrite(buffer, 1, size, stdout);
    fflush(stdout);
    pthread_mutex_unlock(&stdout_mutex);
}

// Generate repeated sequence
void repeat_fasta(const char *header, const char *sequence, int n) {
    int seq_len = strlen(sequence);
    char *expanded_seq = malloc(seq_len * 2 + 1);
    
    // Create an expanded sequence to avoid modulo operations
    strcpy(expanded_seq, sequence);
    strcpy(expanded_seq + seq_len, sequence);
    expanded_seq[2 * seq_len] = '\0';
    
    // Output buffer
    char *buffer = malloc(BUFFER_SIZE);
    int buffer_pos = 0;
    
    // Write header
    pthread_mutex_lock(&stdout_mutex);
    fputs(header, stdout);
    pthread_mutex_unlock(&stdout_mutex);
    
    // Process sequence in blocks
    int pos = 0;
    int remaining = n;
    
    while (remaining > 0) {
        // Determine size of current block
        int block_size = (remaining < LINE_WIDTH * LINES_PER_BLOCK) ? 
                          remaining : LINE_WIDTH * LINES_PER_BLOCK;
        int lines = (block_size + LINE_WIDTH - 1) / LINE_WIDTH;
        
        buffer_pos = 0;
        
        // Generate lines
        for (int i = 0; i < lines; i++) {
            int line_len = (i == lines - 1 && block_size % LINE_WIDTH != 0) ? 
                            block_size % LINE_WIDTH : LINE_WIDTH;
            
            // Copy sequence data
            if (pos + line_len <= seq_len) {
                memcpy(buffer + buffer_pos, sequence + pos, line_len);
            } else {
                // Handle wrapping around
                int first_part = seq_len - pos;
                memcpy(buffer + buffer_pos, sequence + pos, first_part);
                memcpy(buffer + buffer_pos + first_part, sequence, line_len - first_part);
            }
            
            buffer_pos += line_len;
            buffer[buffer_pos++] = '\n';
            
            pos = (pos + line_len) % seq_len;
            remaining -= line_len;
            
            if (remaining <= 0) break;
        }
        
        // Write block
        write_output(buffer, buffer_pos);
    }
    
    free(expanded_seq);
    free(buffer);
}

// Generate random sequence
void random_fasta(const char *header, Frequency *freqs, int n_freqs, int n) {
    // Calculate cumulative probabilities
    double *cumulative = malloc(n_freqs * sizeof(double));
    calculate_cumulative(freqs, n_freqs, cumulative);
    
    // Output buffer
    char *buffer = malloc(BUFFER_SIZE);
    int buffer_pos = 0;
    
    // Write header
    pthread_mutex_lock(&stdout_mutex);
    fputs(header, stdout);
    pthread_mutex_unlock(&stdout_mutex);
    
    // Process sequence in blocks
    int remaining = n;
    
    while (remaining > 0) {
        // Determine size of current block
        int block_size = (remaining < LINE_WIDTH * LINES_PER_BLOCK) ? 
                          remaining : LINE_WIDTH * LINES_PER_BLOCK;
        int lines = (block_size + LINE_WIDTH - 1) / LINE_WIDTH;
        
        buffer_pos = 0;
        
        // Generate random sequence
        for (int i = 0; i < lines; i++) {
            int line_len = (i == lines - 1 && block_size % LINE_WIDTH != 0) ? 
                            block_size % LINE_WIDTH : LINE_WIDTH;
            
            // Generate line
            for (int j = 0; j < line_len; j++) {
                // Generate random number
                pthread_mutex_lock(&seed_mutex);
                seed = (seed * IA + IC) % IM;
                double r = seed / IM;
                pthread_mutex_unlock(&seed_mutex);
                
                // Lookup symbol
                buffer[buffer_pos++] = lookup_symbol(r, cumulative, freqs, n_freqs);
            }
            
            buffer[buffer_pos++] = '\n';
            remaining -= line_len;
            
            if (remaining <= 0) break;
        }
        
        // Write block
        write_output(buffer, buffer_pos);
    }
    
    free(cumulative);
    free(buffer);
}

// Thread function for repeat fasta
void *repeat_fasta_thread(void *arg) {
    const char **args = (const char **)arg;
    const char *header = args[0];
    const char *sequence = args[1];
    int n = atoi(args[2]);
    
    repeat_fasta(header, sequence, n);
    
    return NULL;
}

// Thread function for random fasta
void *random_fasta_thread(void *arg) {
    void **args = (void **)arg;
    const char *header = (const char *)args[0];
    Frequency *freqs = (Frequency *)args[1];
    int n_freqs = *((int *)args[2]);
    int n = *((int *)args[3]);
    
    random_fasta(header, freqs, n_freqs, n);
    
    return NULL;
}

int main(int argc, char *argv[]) {
    if (argc != 2) {
        fprintf(stderr, "Usage: %s <n>\n", argv[0]);
        return 1;
    }
    
    int n = atoi(argv[1]);
    
    // Get number of available processors
    int num_procs = sysconf(_SC_NPROCESSORS_ONLN);
    
    if (num_procs < 2) {
        // Sequential execution
        repeat_fasta(">ONE Homo sapiens alu\n", ALU, n * 2);
        random_fasta(">TWO IUB ambiguity codes\n", IUB, sizeof(IUB)/sizeof(IUB[0]), n * 3);
        random_fasta(">THREE Homo sapiens frequency\n", HOMOSAPIENS, sizeof(HOMOSAPIENS)/sizeof(HOMOSAPIENS[0]), n * 5);
    } else {
        // Parallel execution with threads
        pthread_t threads[3];
        
        // Arguments for repeat_fasta
        const char *repeat_args[3] = {
            ">ONE Homo sapiens alu\n",
            ALU,
            malloc(20)
        };
        sprintf((char *)repeat_args[2], "%d", n * 2);
        
        // Arguments for IUB
        int iub_size = sizeof(IUB) / sizeof(IUB[0]);
        int iub_n = n * 3;
        void *iub_args[4] = {
            ">TWO IUB ambiguity codes\n",
            IUB,
            &iub_size,
            &iub_n
        };
        
        // Arguments for HOMOSAPIENS
        int homosapiens_size = sizeof(HOMOSAPIENS) / sizeof(HOMOSAPIENS[0]);
        int homosapiens_n = n * 5;
        void *homosapiens_args[4] = {
            ">THREE Homo sapiens frequency\n",
            HOMOSAPIENS,
            &homosapiens_size,
            &homosapiens_n
        };
        
        // Create threads
        pthread_create(&threads[0], NULL, repeat_fasta_thread, (void *)repeat_args);
        pthread_create(&threads[1], NULL, random_fasta_thread, (void *)iub_args);
        pthread_create(&threads[2], NULL, random_fasta_thread, (void *)homosapiens_args);
        
        // Wait for threads to finish
        for (int i = 0; i < 3; i++) {
            pthread_join(threads[i], NULL);
        }
        
        free((void *)repeat_args[2]);
    }
    
    return 0;
}
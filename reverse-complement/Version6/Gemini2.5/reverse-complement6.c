/*
 * The Computer Language Benchmarks Game
 * https://salsa.debian.org/benchmarksgame-team/benchmarksgame/
 *
 * Converted from Python to C by Gemini, inspired by existing C implementations.
 * This version uses pthreads for parallelism and is designed for performance.
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <pthread.h>
#include <stdatomic.h>

#define LINE_WIDTH 60

// --- Data Structures ---

// Represents a single FASTA sequence from the input file
typedef struct {
    char* header;       // Pointer to the header line (e.g., ">ONE...")
    size_t header_len;
    char* data;         // Pointer to the start of the sequence data
    size_t data_len;
} InputSequence;


// --- Global Variables ---
static InputSequence* g_sequences = NULL;
static size_t g_num_sequences = 0;
static atomic_size_t g_next_sequence_idx = 0;

// Synchronization primitives for ensuring output is printed in the correct order
static pthread_mutex_t g_output_mutex;
static pthread_cond_t g_output_cond;
static size_t g_sequences_printed = 0;

// Translation table for complementing DNA nucleotides
static char complement_map[256];


// --- Core Logic ---

/**
 * @brief Initializes the complementation map.
 * This map translates each character to its complement according to the problem's rules.
 * Characters not in the map (like whitespace) are mapped to 0 and ignored during processing.
 */
void setup_complement_map() {
    memset(complement_map, 0, 256);
    const unsigned char* from = (const unsigned char*)"ABCDGHKMNRSTUVWYabcdghkmnrstuvwy";
    const unsigned char* to   = (const unsigned char*)"TVGHCDMKNYSAABWRTVGHCDMKNYSAABWR";
    for(size_t i = 0; from[i] != '\0'; ++i) {
        complement_map[from[i]] = to[i];
    }
}

/**
 * @brief The main function for each worker thread.
 * Threads repeatedly grab the index of the next available sequence, process it,
 * and then wait for their turn to print the result to ensure correct output order.
 */
void* worker_func(void* arg) {
    while (1) {
        // Atomically get the index of the next sequence to process
        size_t idx = atomic_fetch_add(&g_next_sequence_idx, 1);
        if (idx >= g_num_sequences) {
            break; // No more sequences to process
        }

        InputSequence* seq = &g_sequences[idx];

        // Allocate a buffer for the result. The result can't be larger than the original.
        char* result_buf = malloc(seq->data_len);
        if (!result_buf) {
            perror("Failed to allocate memory for result buffer");
            exit(EXIT_FAILURE);
        }

        char* write_ptr = result_buf;
        const char* read_ptr = seq->data + seq->data_len - 1;

        // Perform reverse-complement in a single pass
        while (read_ptr >= seq->data) {
            char complemented_char = complement_map[(unsigned char)*read_ptr];
            if (complemented_char) { // Skip whitespace and other non-DNA chars
                *write_ptr++ = complemented_char;
            }
            read_ptr--;
        }
        size_t result_len = write_ptr - result_buf;

        // --- Synchronize and Print Output ---
        pthread_mutex_lock(&g_output_mutex);
        while (idx != g_sequences_printed) {
            // Wait until it's this thread's turn to print
            pthread_cond_wait(&g_output_cond, &g_output_mutex);
        }

        // It's my turn, print the header and the formatted sequence
        fwrite(seq->header, 1, seq->header_len, stdout);
        for (size_t i = 0; i < result_len; i += LINE_WIDTH) {
            size_t chunk_size = (i + LINE_WIDTH < result_len) ? LINE_WIDTH : result_len - i;
            fwrite(result_buf + i, 1, chunk_size, stdout);
            putchar('\n');
        }

        // Increment the printed counter and notify other waiting threads
        g_sequences_printed++;
        pthread_cond_broadcast(&g_output_cond);
        pthread_mutex_unlock(&g_output_mutex);

        free(result_buf);
    }
    return NULL;
}

/**
 * @brief Reads the entire standard input into a dynamically growing buffer.
 * @param final_size Pointer to store the final size of the buffer.
 * @return A pointer to the buffer containing the file content.
 */
char* read_all_stdin(size_t* final_size) {
    size_t capacity = 16 * 1024 * 1024; // Start with 16MB
    size_t size = 0;
    char* buffer = malloc(capacity);
    if (!buffer) return NULL;

    size_t bytes_read;
    while ((bytes_read = fread(buffer + size, 1, capacity - size, stdin)) > 0) {
        size += bytes_read;
        if (size == capacity) {
            capacity *= 2;
            char* new_buffer = realloc(buffer, capacity);
            if (!new_buffer) {
                free(buffer);
                return NULL;
            }
            buffer = new_buffer;
        }
    }
    *final_size = size;
    return buffer;
}

// --- Main Execution ---

int main() {
    setup_complement_map();

    // 1. Read all data from stdin into one large buffer
    size_t file_size;
    char* file_buffer = read_all_stdin(&file_size);
    if (!file_buffer) {
        perror("Failed to read stdin");
        return 1;
    }

    // 2. Parse the buffer to identify individual FASTA sequences
    // First, count the number of sequences to allocate memory
    for (size_t i = 0; i < file_size; ++i) {
        if (file_buffer[i] == '>') {
            g_num_sequences++;
        }
    }
    g_sequences = malloc(g_num_sequences * sizeof(InputSequence));

    char* p = file_buffer;
    char* end = file_buffer + file_size;
    size_t current_seq_idx = 0;

    while (p < end) {
        if (*p == '>') {
            g_sequences[current_seq_idx].header = p;
            char* header_end = memchr(p, '\n', end - p);
            if (!header_end) header_end = end;
            g_sequences[current_seq_idx].header_len = (header_end - p) + 1;

            char* data_start = header_end + 1;
            char* next_header = data_start;
            // Find the start of the next sequence
            while (next_header < end && *next_header != '>') {
                next_header = memchr(next_header, '\n', end - next_header);
                if (!next_header) {
                    next_header = end;
                } else {
                    next_header++;
                }
            }
            
            g_sequences[current_seq_idx].data = data_start;
            g_sequences[current_seq_idx].data_len = (next_header - data_start);
            if (next_header < end && *(next_header-1) == '\n' && *(next_header-2) == '\r') {
                 g_sequences[current_seq_idx].data_len -=2;
            } else if (next_header < end && *(next_header-1) == '\n') {
                 g_sequences[current_seq_idx].data_len--;
            }

            p = next_header;
            current_seq_idx++;
        } else {
            p++; // Should not happen in a valid FASTA file
        }
    }

    // 3. Set up and launch worker threads
    long num_threads = sysconf(_SC_NPROCESSORS_ONLN);
    if (num_threads <= 0) num_threads = 1;
    pthread_t* threads = malloc(num_threads * sizeof(pthread_t));
    
    pthread_mutex_init(&g_output_mutex, NULL);
    pthread_cond_init(&g_output_cond, NULL);

    for (long i = 0; i < num_threads; i++) {
        pthread_create(&threads[i], NULL, worker_func, NULL);
    }

    // 4. Wait for all threads to complete
    for (long i = 0; i < num_threads; i++) {
        pthread_join(threads[i], NULL);
    }
    
    // 5. Clean up resources
    pthread_mutex_destroy(&g_output_mutex);
    pthread_cond_destroy(&g_output_cond);
    free(threads);
    free(g_sequences);
    free(file_buffer);

    return 0;
}

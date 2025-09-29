/*
 * The Computer Language Benchmarks Game
 * https://salsa.debian.org/benchmarksgame-team/benchmarksgame/
 *
 * Converted from Python to C with optimizations by Claude
 * Original Python by Ian Osgood, Heinrich Acker, Justin Peel, and Christopher Sean Forgeron
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <pthread.h>
#include <unistd.h>

#define LINE_WIDTH 60
#define IM 139968.0
#define BUFFER_SIZE (64 * 1024)  // 64KB buffer for output
#define NUM_BUFFERS 2            // Double buffering for output

// ALU sequence
const char *alu =
   "GGCCGGGCGCGGTGGCTCACGCCTGTAATCCCAGCACTTTGG"
   "GAGGCCGAGGCGGGCGGATCACCTGAGGTCAGGAGTTCGAGA"
   "CCAGCCTGGCCAACATGGTGAAACCCCGTCTCTACTAAAAAT"
   "ACAAAAATTAGCCGGGCGTGGTGGCGCGCGCCTGTAATCCCA"
   "GCTACTCGGGAGGCTGAGGCAGGAGAATCGCTTGAACCCGGG"
   "AGGCGGAGGTTGCAGTGAGCCGAGATCGCGCCACTGCACTCC"
   "AGCCTGGGCGACAGAGCGAGACTCCGTCTCAAAAA";

// Frequency tables
typedef struct {
    char c;
    double p;
} Frequency;

Frequency iub[] = {
    {'a', 0.27}, {'c', 0.12}, {'g', 0.12}, {'t', 0.27},
    {'B', 0.02}, {'D', 0.02}, {'H', 0.02}, {'K', 0.02},
    {'M', 0.02}, {'N', 0.02}, {'R', 0.02}, {'S', 0.02},
    {'V', 0.02}, {'W', 0.02}, {'Y', 0.02}
};

Frequency homosapiens[] = {
    {'a', 0.3029549426680},
    {'c', 0.1979883004921},
    {'g', 0.1975473066391},
    {'t', 0.3015094502008}
};

// Lookup table for faster character selection
typedef struct {
    double prob;
    char c;
} CumulativeFreq;

// Buffer for asynchronous I/O
typedef struct {
    char data[BUFFER_SIZE];
    size_t size;
    int ready;
    pthread_mutex_t mutex;
} Buffer;

// Globals for double buffering
Buffer buffers[NUM_BUFFERS];
int current_buffer = 0;
int write_buffer = 0;
pthread_t writer_thread;
int writer_running = 0;
pthread_mutex_t buffer_switch_mutex = PTHREAD_MUTEX_INITIALIZER;
pthread_cond_t buffer_ready_cond = PTHREAD_COND_INITIALIZER;
pthread_cond_t buffer_written_cond = PTHREAD_COND_INITIALIZER;

// Precomputed lookup tables for faster random generation
CumulativeFreq iub_lookup[15];
CumulativeFreq homosapiens_lookup[4];

// Convert frequency table to cumulative probabilities
void make_cumulative(Frequency *table, int table_size, CumulativeFreq *result) {
    double prob = 0.0;
    for (int i = 0; i < table_size; i++) {
        prob += table[i].p;
        result[i].prob = prob;
        result[i].c = table[i].c;
    }
}

// Binary search to find character based on probability
inline char lookup_char(CumulativeFreq *table, int table_size, double random) {
    // Optimized binary search
    int low = 0;
    int high = table_size - 1;
    
    while (low < high) {
        int mid = (low + high) / 2;
        if (random < table[mid].prob)
            high = mid;
        else
            low = mid + 1;
    }
    
    return table[low].c;
}

// Thread function for asynchronous writing
void* writer_function(void *arg) {
    while (writer_running || buffers[write_buffer].size > 0) {
        pthread_mutex_lock(&buffer_switch_mutex);
        
        // Wait until a buffer is ready
        while (!buffers[write_buffer].ready && writer_running) {
            pthread_cond_wait(&buffer_ready_cond, &buffer_switch_mutex);
        }
        
        // Get the buffer to write
        Buffer *buffer = &buffers[write_buffer];
        size_t size = buffer->size;
        
        // If we're shutting down and there's nothing to write, exit
        if (size == 0 && !writer_running) {
            pthread_mutex_unlock(&buffer_switch_mutex);
            break;
        }
        
        // Mark buffer as not ready
        buffer->ready = 0;
        pthread_mutex_unlock(&buffer_switch_mutex);
        
        // Write the buffer to stdout
        fwrite(buffer->data, 1, size, stdout);
        
        // Update buffer status
        pthread_mutex_lock(&buffer_switch_mutex);
        buffer->size = 0;
        write_buffer = (write_buffer + 1) % NUM_BUFFERS;
        pthread_cond_signal(&buffer_written_cond);
        pthread_mutex_unlock(&buffer_switch_mutex);
    }
    
    return NULL;
}

// Initialize the asynchronous I/O system
void init_io() {
    // Initialize buffers
    for (int i = 0; i < NUM_BUFFERS; i++) {
        buffers[i].size = 0;
        buffers[i].ready = 0;
        pthread_mutex_init(&buffers[i].mutex, NULL);
    }
    
    // Start writer thread
    writer_running = 1;
    pthread_create(&writer_thread, NULL, writer_function, NULL);
}

// Flush current buffer and wait for it to be written
void flush_buffer() {
    pthread_mutex_lock(&buffer_switch_mutex);
    
    // Mark current buffer as ready to be written
    if (buffers[current_buffer].size > 0) {
        buffers[current_buffer].ready = 1;
        pthread_cond_signal(&buffer_ready_cond);
        
        // Switch to next buffer
        current_buffer = (current_buffer + 1) % NUM_BUFFERS;
        
        // Wait until the next buffer is available
        while (buffers[current_buffer].size > 0) {
            pthread_cond_wait(&buffer_written_cond, &buffer_switch_mutex);
        }
    }
    
    pthread_mutex_unlock(&buffer_switch_mutex);
}

// Add data to current buffer, flushing if necessary
void buffer_write(const char *data, size_t size) {
    // If data is too large for buffer, flush and write directly
    if (size > BUFFER_SIZE) {
        flush_buffer();
        fwrite(data, 1, size, stdout);
        return;
    }
    
    // If data won't fit in current buffer, flush it
    if (buffers[current_buffer].size + size > BUFFER_SIZE) {
        flush_buffer();
    }
    
    // Copy data to buffer
    memcpy(buffers[current_buffer].data + buffers[current_buffer].size, data, size);
    buffers[current_buffer].size += size;
}

// Shutdown the asynchronous I/O system
void shutdown_io() {
    // Flush any remaining data
    flush_buffer();
    
    // Signal writer thread to shut down
    pthread_mutex_lock(&buffer_switch_mutex);
    writer_running = 0;
    pthread_cond_signal(&buffer_ready_cond);
    pthread_mutex_unlock(&buffer_switch_mutex);
    
    // Wait for writer thread to finish
    pthread_join(writer_thread, NULL);
    
    // Clean up
    for (int i = 0; i < NUM_BUFFERS; i++) {
        pthread_mutex_destroy(&buffers[i].mutex);
    }
    pthread_mutex_destroy(&buffer_switch_mutex);
    pthread_cond_destroy(&buffer_ready_cond);
    pthread_cond_destroy(&buffer_written_cond);
}

// Generate a repeated sequence
void repeat_fasta(const char *src, int src_len, int n) {
    char buffer[LINE_WIDTH + 1];
    buffer[LINE_WIDTH] = '\n';
    
    int pos = 0;
    int remaining = n;
    
    while (remaining > 0) {
        int line_len = remaining < LINE_WIDTH ? remaining : LINE_WIDTH;
        
        // Optimize for the case where we can copy directly from source
        if (pos + line_len <= src_len) {
            memcpy(buffer, src + pos, line_len);
        } else {
            // Handle the wraparound case
            for (int i = 0; i < line_len; i++) {
                buffer[i] = src[(pos + i) % src_len];
            }
        }
        
        // Add newline if it's a full line
        if (line_len == LINE_WIDTH) {
            buffer_write(buffer, line_len + 1);
        } else {
            // Add newline for partial line
            buffer[line_len] = '\n';
            buffer_write(buffer, line_len + 1);
        }
        
        pos = (pos + line_len) % src_len;
        remaining -= line_len;
    }
}

// Generate a random sequence
double random_fasta(CumulativeFreq *table, int table_size, int n, double seed) {
    char buffer[LINE_WIDTH + 1];
    buffer[LINE_WIDTH] = '\n';
    
    int remaining = n;
    
    while (remaining > 0) {
        int line_len = remaining < LINE_WIDTH ? remaining : LINE_WIDTH;
        
        for (int i = 0; i < line_len; i++) {
            seed = (seed * 3877.0 + 29573.0) % IM;
            buffer[i] = lookup_char(table, table_size, seed / IM);
        }
        
        // Add newline if it's a full line
        if (line_len == LINE_WIDTH) {
            buffer_write(buffer, line_len + 1);
        } else {
            // Add newline for partial line
            buffer[line_len] = '\n';
            buffer_write(buffer, line_len + 1);
        }
        
        remaining -= line_len;
    }
    
    return seed;
}

int main(int argc, char *argv[]) {
    if (argc != 2) {
        fprintf(stderr, "Usage: %s <number>\n", argv[0]);
        return 1;
    }
    
    int n = atoi(argv[1]);
    
    // Calculate the size of the frequency tables
    int iub_size = sizeof(iub) / sizeof(iub[0]);
    int homosapiens_size = sizeof(homosapiens) / sizeof(homosapiens[0]);
    
    // Convert the frequency tables to cumulative probabilities
    make_cumulative(iub, iub_size, iub_lookup);
    make_cumulative(homosapiens, homosapiens_size, homosapiens_lookup);
    
    // Initialize I/O system
    init_io();
    
    // Generate the sequences
    const char *header1 = ">ONE Homo sapiens alu\n";
    buffer_write(header1, strlen(header1));
    repeat_fasta(alu, strlen(alu), n * 2);
    
    const char *header2 = ">TWO IUB ambiguity codes\n";
    buffer_write(header2, strlen(header2));
    double seed = 42.0;
    seed = random_fasta(iub_lookup, iub_size, n * 3, seed);
    
    const char *header3 = ">THREE Homo sapiens frequency\n";
    buffer_write(header3, strlen(header3));
    random_fasta(homosapiens_lookup, homosapiens_size, n * 5, seed);
    
    // Shutdown I/O system
    shutdown_io();
    
    return 0;
}
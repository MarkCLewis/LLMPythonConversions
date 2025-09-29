// The Computer Language Benchmarks Game
// https://salsa.debian.org/benchmarksgame-team/benchmarksgame/
//
// Converted from Python to C by Claude

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <pthread.h>
#include <unistd.h>

#define BUFFER_SIZE 8192
#define LINE_LENGTH 60
#define MAX_SEQUENCES 1024

// Translation table for complementing DNA
unsigned char reverse_translation[256];

// Structure for a sequence
typedef struct {
    char *header;
    size_t header_len;
    char *sequence;
    size_t seq_len;
    size_t seq_capacity;
} Sequence;

// Array to store sequences
Sequence *sequences;
int num_sequences = 0;

// Thread synchronization
pthread_mutex_t output_mutex = PTHREAD_MUTEX_INITIALIZER;
pthread_cond_t sequence_processed_cond = PTHREAD_COND_INITIALIZER;
int next_sequence_to_output = 0;

// Initialize the reverse translation table
void init_reverse_translation() {
    // Initialize with identity (each byte maps to itself)
    for (int i = 0; i < 256; i++) {
        reverse_translation[i] = i;
    }
    
    // Define the complementary bases
    reverse_translation[(int)'A'] = 'T';
    reverse_translation[(int)'B'] = 'V';
    reverse_translation[(int)'C'] = 'G';
    reverse_translation[(int)'D'] = 'H';
    reverse_translation[(int)'G'] = 'C';
    reverse_translation[(int)'H'] = 'D';
    reverse_translation[(int)'K'] = 'M';
    reverse_translation[(int)'M'] = 'K';
    reverse_translation[(int)'N'] = 'N';
    reverse_translation[(int)'R'] = 'Y';
    reverse_translation[(int)'S'] = 'S';
    reverse_translation[(int)'T'] = 'A';
    reverse_translation[(int)'U'] = 'A';
    reverse_translation[(int)'V'] = 'B';
    reverse_translation[(int)'W'] = 'W';
    reverse_translation[(int)'Y'] = 'R';
    
    reverse_translation[(int)'a'] = 't';
    reverse_translation[(int)'b'] = 'v';
    reverse_translation[(int)'c'] = 'g';
    reverse_translation[(int)'d'] = 'h';
    reverse_translation[(int)'g'] = 'c';
    reverse_translation[(int)'h'] = 'd';
    reverse_translation[(int)'k'] = 'm';
    reverse_translation[(int)'m'] = 'k';
    reverse_translation[(int)'n'] = 'n';
    reverse_translation[(int)'r'] = 'y';
    reverse_translation[(int)'s'] = 's';
    reverse_translation[(int)'t'] = 'a';
    reverse_translation[(int)'u'] = 'a';
    reverse_translation[(int)'v'] = 'b';
    reverse_translation[(int)'w'] = 'w';
    reverse_translation[(int)'y'] = 'r';
}

// Process a sequence: reverse complement and prepare for output
char* reverse_complement(Sequence *seq, size_t *result_len) {
    // Create a buffer for the translated sequence (without newlines)
    char *translated = malloc(seq->seq_len);
    if (!translated) {
        perror("Failed to allocate memory for translated sequence");
        exit(EXIT_FAILURE);
    }
    
    // Copy and translate the sequence, skipping newlines and spaces
    size_t translated_len = 0;
    for (size_t i = 0; i < seq->seq_len; i++) {
        char c = seq->sequence[i];
        if (c != '\n' && c != '\r' && c != ' ') {
            translated[translated_len++] = reverse_translation[(unsigned char)c];
        }
    }
    
    // Create the result buffer with added newlines
    // We need at most translated_len + translated_len/LINE_LENGTH + 2 bytes
    char *result = malloc(translated_len + translated_len / LINE_LENGTH + 2);
    if (!result) {
        perror("Failed to allocate memory for result");
        free(translated);
        exit(EXIT_FAILURE);
    }
    
    *result_len = 0;
    
    // Add initial newline
    result[(*result_len)++] = '\n';
    
    // Add the trailing part (the part that doesn't make a full line)
    size_t trailing_length = translated_len % LINE_LENGTH;
    if (trailing_length > 0) {
        for (int i = translated_len - trailing_length; i < translated_len; i++) {
            result[(*result_len)++] = translated[i];
        }
        result[(*result_len)++] = '\n';
    }
    
    // Add full lines in reverse order
    for (size_t i = translated_len - trailing_length; i > 0; i -= LINE_LENGTH) {
        size_t start = i - LINE_LENGTH;
        for (size_t j = 0; j < LINE_LENGTH; j++) {
            result[(*result_len)++] = translated[start + j];
        }
        result[(*result_len)++] = '\n';
    }
    
    free(translated);
    return result;
}

// Thread function to process sequences
void* process_sequence_thread(void *arg) {
    int thread_id = *(int*)arg;
    free(arg);
    
    // Process sequences in a round-robin fashion
    for (int i = thread_id; i < num_sequences; i += sysconf(_SC_NPROCESSORS_ONLN)) {
        // Get the sequence
        Sequence *seq = &sequences[i];
        
        // Process the sequence
        size_t result_len;
        char *result = reverse_complement(seq, &result_len);
        
        // Wait for our turn to output
        pthread_mutex_lock(&output_mutex);
        while (next_sequence_to_output != i) {
            pthread_cond_wait(&sequence_processed_cond, &output_mutex);
        }
        
        // Output the sequence
        fwrite(seq->header, 1, seq->header_len, stdout);
        fwrite(result, 1, result_len, stdout);
        
        // Free the result buffer
        free(result);
        
        // Update the next sequence to output and signal other threads
        next_sequence_to_output++;
        pthread_cond_broadcast(&sequence_processed_cond);
        pthread_mutex_unlock(&output_mutex);
    }
    
    return NULL;
}

// Read sequences from stdin
void read_sequences() {
    char buffer[BUFFER_SIZE];
    size_t bytes_read;
    bool in_header = false;
    Sequence current_seq = {0};
    
    // Allocate space for sequences
    sequences = malloc(MAX_SEQUENCES * sizeof(Sequence));
    if (!sequences) {
        perror("Failed to allocate memory for sequences");
        exit(EXIT_FAILURE);
    }
    
    // Read input one character at a time
    int c;
    while ((c = getc(stdin)) != EOF) {
        // Check for sequence header
        if (c == '>') {
            // If we already have a sequence, add it to the array
            if (current_seq.header != NULL) {
                sequences[num_sequences++] = current_seq;
                
                // Reallocate if needed
                if (num_sequences >= MAX_SEQUENCES) {
                    sequences = realloc(sequences, (num_sequences + MAX_SEQUENCES) * sizeof(Sequence));
                    if (!sequences) {
                        perror("Failed to reallocate memory for sequences");
                        exit(EXIT_FAILURE);
                    }
                }
                
                // Initialize new sequence
                current_seq.header = NULL;
                current_seq.header_len = 0;
                current_seq.sequence = NULL;
                current_seq.seq_len = 0;
                current_seq.seq_capacity = 0;
            }
            
            // Start new header
            in_header = true;
            
            // Allocate memory for the header
            current_seq.header = malloc(BUFFER_SIZE);
            if (!current_seq.header) {
                perror("Failed to allocate memory for header");
                exit(EXIT_FAILURE);
            }
            
            // Add the '>' character
            current_seq.header[0] = '>';
            current_seq.header_len = 1;
        }
        
        // Process the character
        if (in_header) {
            // Add character to header
            if (current_seq.header_len >= BUFFER_SIZE - 1) {
                current_seq.header = realloc(current_seq.header, current_seq.header_len + BUFFER_SIZE);
                if (!current_seq.header) {
                    perror("Failed to reallocate memory for header");
                    exit(EXIT_FAILURE);
                }
            }
            
            current_seq.header[current_seq.header_len++] = c;
            
            // Check for end of header
            if (c == '\n') {
                in_header = false;
                
                // Allocate memory for the sequence
                current_seq.sequence = malloc(BUFFER_SIZE);
                if (!current_seq.sequence) {
                    perror("Failed to allocate memory for sequence");
                    exit(EXIT_FAILURE);
                }
                current_seq.seq_capacity = BUFFER_SIZE;
            }
        } else {
            // Add character to sequence
            if (current_seq.seq_len >= current_seq.seq_capacity - 1) {
                current_seq.seq_capacity += BUFFER_SIZE;
                current_seq.sequence = realloc(current_seq.sequence, current_seq.seq_capacity);
                if (!current_seq.sequence) {
                    perror("Failed to reallocate memory for sequence");
                    exit(EXIT_FAILURE);
                }
            }
            
            current_seq.sequence[current_seq.seq_len++] = c;
        }
    }
    
    // Add the last sequence
    if (current_seq.header != NULL) {
        sequences[num_sequences++] = current_seq;
    }
}

// Process sequences sequentially
void process_sequences_sequential() {
    for (int i = 0; i < num_sequences; i++) {
        Sequence *seq = &sequences[i];
        
        size_t result_len;
        char *result = reverse_complement(seq, &result_len);
        
        fwrite(seq->header, 1, seq->header_len, stdout);
        fwrite(result, 1, result_len, stdout);
        
        free(result);
    }
}

// Free all allocated memory
void cleanup() {
    for (int i = 0; i < num_sequences; i++) {
        free(sequences[i].header);
        free(sequences[i].sequence);
    }
    free(sequences);
}

int main() {
    // Initialize the translation table
    init_reverse_translation();
    
    // Read all sequences
    read_sequences();
    
    // Determine whether to use parallel processing
    int num_cores = sysconf(_SC_NPROCESSORS_ONLN);
    bool use_parallel = (num_cores > 1) && (num_sequences > 0) && 
                        (sequences[0].seq_len >= 1000000);
    
    if (use_parallel) {
        // Create threads
        pthread_t *threads = malloc(num_cores * sizeof(pthread_t));
        if (!threads) {
            perror("Failed to allocate memory for threads");
            exit(EXIT_FAILURE);
        }
        
        // Start threads
        for (int i = 0; i < num_cores; i++) {
            int *thread_id = malloc(sizeof(int));
            *thread_id = i;
            pthread_create(&threads[i], NULL, process_sequence_thread, thread_id);
        }
        
        // Wait for threads to finish
        for (int i = 0; i < num_cores; i++) {
            pthread_join(threads[i], NULL);
        }
        
        free(threads);
    } else {
        // Process sequences sequentially
        process_sequences_sequential();
    }
    
    // Clean up
    cleanup();
    
    return 0;
}
// The Computer Language Benchmarks Game
// https://salsa.debian.org/benchmarksgame-team/benchmarksgame/
//
// Converted from Python to C by Claude

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>
#include <unistd.h>
#include <stdbool.h>
#include <ctype.h>

#define READ_SIZE 65536
#define LINE_LENGTH 60
#define MAX_SEQUENCES 1024

// Thread-safe queue for sequence data
typedef struct {
    char **data;
    size_t *sizes;
    int *sequence_numbers;
    int head;
    int tail;
    int size;
    int capacity;
    pthread_mutex_t mutex;
    pthread_cond_t not_empty;
} SequenceQueue;

// Struct for sequence data
typedef struct {
    char *data;
    size_t size;
    size_t capacity;
    int sequence_number;
} Sequence;

// Global variables
static unsigned char complement_lookup[256];
static SequenceQueue sequence_queue;
static pthread_mutex_t output_mutex = PTHREAD_MUTEX_INITIALIZER;
static pthread_cond_t sequence_written_cond = PTHREAD_COND_INITIALIZER;
static int next_sequence_to_output = 1;
static int num_threads;

// Initialize the complement lookup table
void init_complement_lookup() {
    for (int i = 0; i < 256; i++) {
        complement_lookup[i] = i;
    }
    
    // Set up the DNA complementary bases
    complement_lookup['A'] = 'T'; complement_lookup['a'] = 't';
    complement_lookup['C'] = 'G'; complement_lookup['c'] = 'g';
    complement_lookup['G'] = 'C'; complement_lookup['g'] = 'c';
    complement_lookup['T'] = 'A'; complement_lookup['t'] = 'a';
    complement_lookup['U'] = 'A'; complement_lookup['u'] = 'a';
    complement_lookup['M'] = 'K'; complement_lookup['m'] = 'k';
    complement_lookup['R'] = 'Y'; complement_lookup['r'] = 'y';
    complement_lookup['W'] = 'W'; complement_lookup['w'] = 'w';
    complement_lookup['S'] = 'S'; complement_lookup['s'] = 's';
    complement_lookup['Y'] = 'R'; complement_lookup['y'] = 'r';
    complement_lookup['K'] = 'M'; complement_lookup['k'] = 'm';
    complement_lookup['V'] = 'B'; complement_lookup['v'] = 'b';
    complement_lookup['H'] = 'D'; complement_lookup['h'] = 'd';
    complement_lookup['D'] = 'H'; complement_lookup['d'] = 'h';
    complement_lookup['B'] = 'V'; complement_lookup['b'] = 'v';
    complement_lookup['N'] = 'N'; complement_lookup['n'] = 'n';
    
    // Ensure newlines remain unchanged
    complement_lookup['\n'] = '\n';
}

// Initialize the sequence queue
void init_sequence_queue(int capacity) {
    sequence_queue.data = (char **)malloc(capacity * sizeof(char *));
    sequence_queue.sizes = (size_t *)malloc(capacity * sizeof(size_t));
    sequence_queue.sequence_numbers = (int *)malloc(capacity * sizeof(int));
    sequence_queue.head = 0;
    sequence_queue.tail = 0;
    sequence_queue.size = 0;
    sequence_queue.capacity = capacity;
    
    pthread_mutex_init(&sequence_queue.mutex, NULL);
    pthread_cond_init(&sequence_queue.not_empty, NULL);
}

// Free the sequence queue
void free_sequence_queue() {
    for (int i = 0; i < sequence_queue.capacity; i++) {
        if (sequence_queue.data[i] != NULL) {
            free(sequence_queue.data[i]);
        }
    }
    
    free(sequence_queue.data);
    free(sequence_queue.sizes);
    free(sequence_queue.sequence_numbers);
    
    pthread_mutex_destroy(&sequence_queue.mutex);
    pthread_cond_destroy(&sequence_queue.not_empty);
}

// Add a sequence to the queue
void enqueue_sequence(char *data, size_t size, int sequence_number) {
    pthread_mutex_lock(&sequence_queue.mutex);
    
    if (sequence_queue.size == sequence_queue.capacity) {
        // Queue is full, resize
        int new_capacity = sequence_queue.capacity * 2;
        char **new_data = (char **)realloc(sequence_queue.data, new_capacity * sizeof(char *));
        size_t *new_sizes = (size_t *)realloc(sequence_queue.sizes, new_capacity * sizeof(size_t));
        int *new_sequence_numbers = (int *)realloc(sequence_queue.sequence_numbers, new_capacity * sizeof(int));
        
        if (new_data == NULL || new_sizes == NULL || new_sequence_numbers == NULL) {
            fprintf(stderr, "Failed to resize sequence queue\n");
            exit(1);
        }
        
        // Initialize new slots
        for (int i = sequence_queue.capacity; i < new_capacity; i++) {
            new_data[i] = NULL;
        }
        
        sequence_queue.data = new_data;
        sequence_queue.sizes = new_sizes;
        sequence_queue.sequence_numbers = new_sequence_numbers;
        sequence_queue.capacity = new_capacity;
    }
    
    sequence_queue.data[sequence_queue.tail] = data;
    sequence_queue.sizes[sequence_queue.tail] = size;
    sequence_queue.sequence_numbers[sequence_queue.tail] = sequence_number;
    
    sequence_queue.tail = (sequence_queue.tail + 1) % sequence_queue.capacity;
    sequence_queue.size++;
    
    pthread_cond_signal(&sequence_queue.not_empty);
    pthread_mutex_unlock(&sequence_queue.mutex);
}

// Get a sequence from the queue
bool dequeue_sequence(char **data, size_t *size, int *sequence_number) {
    pthread_mutex_lock(&sequence_queue.mutex);
    
    while (sequence_queue.size == 0) {
        pthread_cond_wait(&sequence_queue.not_empty, &sequence_queue.mutex);
    }
    
    *data = sequence_queue.data[sequence_queue.head];
    *size = sequence_queue.sizes[sequence_queue.head];
    *sequence_number = sequence_queue.sequence_numbers[sequence_queue.head];
    
    sequence_queue.data[sequence_queue.head] = NULL;
    sequence_queue.head = (sequence_queue.head + 1) % sequence_queue.capacity;
    sequence_queue.size--;
    
    pthread_mutex_unlock(&sequence_queue.mutex);
    return true;
}

// Process a sequence (reverse complement it)
void process_sequence(char *sequence, size_t size) {
    if (size <= 0) return;
    
    // Find the header and the actual sequence data
    char *header_end = strchr(sequence, '\n');
    if (header_end == NULL) return;
    
    size_t header_length = header_end - sequence + 1;
    char *seq_data = header_end + 1;
    size_t seq_data_length = size - header_length;
    
    // Create a buffer for the reversed complemented sequence
    char *complemented = (char *)malloc(seq_data_length + 1);
    if (complemented == NULL) {
        fprintf(stderr, "Failed to allocate memory for complemented sequence\n");
        exit(1);
    }
    
    // Check if all lines are of optimal length
    bool optimal_length = true;
    size_t data_size = 0;
    
    for (char *p = seq_data; p < sequence + size; ) {
        char *newline = strchr(p, '\n');
        if (newline == NULL) {
            optimal_length = false;
            break;
        }
        
        size_t line_length = newline - p;
        if (line_length != LINE_LENGTH && p != sequence + size - 1) {
            optimal_length = false;
            break;
        }
        
        p = newline + 1;
        data_size += line_length;
    }
    
    if (optimal_length) {
        // Complement and reverse in one pass
        size_t i = 0;
        for (ssize_t j = seq_data_length - 1; j >= 0; j--) {
            char c = seq_data[j];
            if (c != '\n') {
                complemented[i++] = complement_lookup[(unsigned char)c];
            }
        }
        
        // Reformat into lines of LINE_LENGTH
        char *formatted = (char *)malloc(seq_data_length + 1);
        if (formatted == NULL) {
            fprintf(stderr, "Failed to allocate memory for formatted sequence\n");
            free(complemented);
            exit(1);
        }
        
        size_t formatted_index = 0;
        for (size_t j = 0; j < data_size; j++) {
            formatted[formatted_index++] = complemented[j];
            if ((j + 1) % LINE_LENGTH == 0 || j == data_size - 1) {
                formatted[formatted_index++] = '\n';
            }
        }
        
        // Write the header and complemented sequence
        pthread_mutex_lock(&output_mutex);
        fwrite(sequence, 1, header_length, stdout);
        fwrite(formatted, 1, formatted_index, stdout);
        pthread_mutex_unlock(&output_mutex);
        
        free(formatted);
    } else {
        // Remove newlines, complement, and then reverse
        char *stripped = (char *)malloc(seq_data_length);
        if (stripped == NULL) {
            fprintf(stderr, "Failed to allocate memory for stripped sequence\n");
            free(complemented);
            exit(1);
        }
        
        size_t stripped_length = 0;
        for (size_t i = 0; i < seq_data_length; i++) {
            if (seq_data[i] != '\n') {
                stripped[stripped_length++] = complement_lookup[(unsigned char)seq_data[i]];
            }
        }
        
        // Reverse the stripped sequence
        for (size_t i = 0; i < stripped_length / 2; i++) {
            char temp = stripped[i];
            stripped[i] = stripped[stripped_length - i - 1];
            stripped[stripped_length - i - 1] = temp;
        }
        
        // Format with newlines
        char *formatted = (char *)malloc(seq_data_length + stripped_length / LINE_LENGTH + 2);
        if (formatted == NULL) {
            fprintf(stderr, "Failed to allocate memory for formatted sequence\n");
            free(complemented);
            free(stripped);
            exit(1);
        }
        
        size_t formatted_index = 0;
        for (size_t i = 0; i < stripped_length; i++) {
            formatted[formatted_index++] = stripped[i];
            if ((i + 1) % LINE_LENGTH == 0 || i == stripped_length - 1) {
                formatted[formatted_index++] = '\n';
            }
        }
        
        // Write the header and complemented sequence
        pthread_mutex_lock(&output_mutex);
        fwrite(sequence, 1, header_length, stdout);
        fwrite(formatted, 1, formatted_index, stdout);
        pthread_mutex_unlock(&output_mutex);
        
        free(stripped);
        free(formatted);
    }
    
    free(complemented);
}

// Worker thread function
void *worker_thread(void *arg) {
    while (true) {
        char *sequence;
        size_t size;
        int sequence_number;
        
        if (!dequeue_sequence(&sequence, &size, &sequence_number)) {
            break;
        }
        
        // Skip sequence 0 (content before first header)
        if (sequence_number > 0) {
            // Process the sequence
            process_sequence(sequence, size);
            
            // Signal that this sequence is done
            pthread_mutex_lock(&output_mutex);
            next_sequence_to_output++;
            pthread_cond_broadcast(&sequence_written_cond);
            pthread_mutex_unlock(&output_mutex);
        }
        
        free(sequence);
    }
    
    return NULL;
}

// Read and parse FASTA format sequences
void read_sequences() {
    char buffer[READ_SIZE];
    char *current_sequence = NULL;
    size_t current_size = 0;
    size_t current_capacity = 0;
    int sequence_number = 0;
    bool in_header = false;
    
    ssize_t bytes_read;
    while ((bytes_read = read(STDIN_FILENO, buffer, READ_SIZE)) > 0) {
        for (ssize_t i = 0; i < bytes_read; i++) {
            // Check for new sequence header
            if (buffer[i] == '>') {
                // If we have a current sequence, enqueue it
                if (current_sequence != NULL) {
                    enqueue_sequence(current_sequence, current_size, sequence_number);
                    sequence_number++;
                    
                    // Reset for new sequence
                    current_sequence = NULL;
                    current_size = 0;
                    current_capacity = 0;
                }
                
                in_header = true;
            }
            
            // Grow the current sequence buffer if needed
            if (current_size + 1 >= current_capacity) {
                size_t new_capacity = current_capacity == 0 ? READ_SIZE : current_capacity * 2;
                char *new_sequence = (char *)realloc(current_sequence, new_capacity);
                
                if (new_sequence == NULL) {
                    fprintf(stderr, "Failed to allocate memory for sequence\n");
                    exit(1);
                }
                
                current_sequence = new_sequence;
                current_capacity = new_capacity;
            }
            
            // Add the current character
            current_sequence[current_size++] = buffer[i];
            
            if (buffer[i] == '\n' && in_header) {
                in_header = false;
            }
        }
    }
    
    // Enqueue the last sequence if there is one
    if (current_sequence != NULL) {
        enqueue_sequence(current_sequence, current_size, sequence_number);
    }
}

int main() {
    // Initialize complement lookup table
    init_complement_lookup();
    
    // Determine number of CPU cores for threading
    num_threads = sysconf(_SC_NPROCESSORS_ONLN);
    if (num_threads <= 0) num_threads = 1;
    
    // Initialize sequence queue
    init_sequence_queue(MAX_SEQUENCES);
    
    // Create worker threads
    pthread_t *threads = (pthread_t *)malloc(num_threads * sizeof(pthread_t));
    if (threads == NULL) {
        fprintf(stderr, "Failed to allocate memory for threads\n");
        return 1;
    }
    
    for (int i = 0; i < num_threads; i++) {
        if (pthread_create(&threads[i], NULL, worker_thread, NULL) != 0) {
            fprintf(stderr, "Failed to create worker thread\n");
            return 1;
        }
    }
    
    // Read and parse sequences
    read_sequences();
    
    // Wait for all threads to finish
    for (int i = 0; i < num_threads; i++) {
        pthread_join(threads[i], NULL);
    }
    
    // Clean up
    free_sequence_queue();
    free(threads);
    
    return 0;
}
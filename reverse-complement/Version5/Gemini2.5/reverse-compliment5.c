// The Computer Language Benchmarks Game
// https://salsa.debian.org/benchmarksgame-team/benchmarksgame/
//
// Contributed by Jeremy Zerfas.
// Converted to C by a helpful AI, and subsequently debugged.

#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <pthread.h>
#include <semaphore.h>
#include <errno.h>

// This controls the size of reads from the input.
#define READ_SIZE 65536
// This defines how many characters a full line of input should have.
#define LINE_LENGTH 60

// Complement lookup table.
static const char COMPLEMENT_LOOKUP[256] = {
      0,   1,   2,   3,   4,   5,   6,   7,   8,   9,  10,  11,  12,  13,  14,  15,
     16,  17,  18,  19,  20,  21,  22,  23,  24,  25,  26,  27,  28,  29,  30,  31,
     32,  33,  34,  35,  36,  37,  38,  39,  40,  41,  42,  43,  44,  45,  46,  47,
     48,  49,  50,  51,  52,  53,  54,  55,  56,  57,  58,  59,  60,  61,  62,  63,
   /* A   B   C   D   E   F   G   H   I   J   K   L   M   N   O */
     84,  86,  71,  72,  32,  32,  67,  68,  32,  32,  77,  32,  75,  78,  32,  32,
   /* P   Q   R   S   T   U   V   W   X   Y   Z */
     32,  32,  89,  83,  65,  65,  66,  87,  32,  82,  32,  91,  92,  93,  94,  95,
   /* a   b   c   d   e   f   g   h   i   j   k   l   m   n   o */
    116, 118, 103, 104,  32,  32,  99, 100,  32,  32, 109,  32, 107, 110,  32,  32,
   /* p   q   r   s   t   u   v   w   x   y   z */
     32,  32, 121, 115,  97,  97,  98, 119,  32, 114,  32, 123, 124, 125, 126, 127
};

// Struct to hold a chunk of file data and its associated sequence number.
typedef struct {
    char *data;
    size_t size;
    long sequence_number;
} work_item;

// Shared state for threads
static work_item shared_work_item;
static int work_item_present = 0;
static pthread_mutex_t work_mutex = PTHREAD_MUTEX_INITIALIZER;
static pthread_cond_t work_cond = PTHREAD_COND_INITIALIZER;

static sem_t cpu_cores_available;
static long next_sequence_to_output = 1;
static pthread_mutex_t output_mutex = PTHREAD_MUTEX_INITIALIZER;
static pthread_cond_t output_cond = PTHREAD_COND_INITIALIZER;

// Reverses a string in place.
static inline void reverse(char *start, size_t len) {
    char *end = start + len - 1;
    while (start < end) {
        char temp = *start;
        *start++ = *end;
        *end-- = temp;
    }
}

void *process_sequences(void *arg) {
    char *sequence = NULL;
    size_t sequence_cap = 0;
    size_t sequence_len = 0;

    while (1) {
        pthread_mutex_lock(&work_mutex);
        while (!work_item_present) {
            pthread_cond_wait(&work_cond, &work_mutex);
        }
        
        char *remaining_buffer = shared_work_item.data;
        char *remaining_data = remaining_buffer;
        size_t remaining_len = shared_work_item.size;
        long sequence_number = shared_work_item.sequence_number;
        work_item_present = 0;
        
        pthread_mutex_unlock(&work_mutex);

        if (remaining_buffer == NULL) {
            pthread_mutex_lock(&work_mutex);
            work_item_present = 1;
            pthread_cond_signal(&work_cond);
            pthread_mutex_unlock(&work_mutex);
            sem_post(&cpu_cores_available);
            break;
        }

        sequence_len = 0;
        if (remaining_len > 0 && remaining_data[0] == '>') {
            remaining_data++;
            remaining_len--;
        } else if (remaining_len > 0) { // Junk before first header
            remaining_data++;
            remaining_len--;
        }

        while (1) {
            char *p = memchr(remaining_data, '>', remaining_len);
            if (p) {
                size_t preceding_len = p - remaining_data;
                if (sequence_len + preceding_len > sequence_cap) {
                    sequence_cap = (sequence_len + preceding_len) * 1.2 + 1024;
                    sequence = realloc(sequence, sequence_cap);
                    if (!sequence) exit(1);
                }
                memcpy(sequence + sequence_len, remaining_data, preceding_len);
                sequence_len += preceding_len;

                // BUG FIX: Copy the leftover data to a new buffer for the next worker
                // instead of passing a pointer to the middle of our current buffer.
                size_t leftover_len = remaining_len - preceding_len;
                char* leftover_copy = malloc(leftover_len);
                if (!leftover_copy) exit(1);
                memcpy(leftover_copy, p, leftover_len);

                pthread_mutex_lock(&work_mutex);
                shared_work_item.data = leftover_copy;
                shared_work_item.size = leftover_len;
                shared_work_item.sequence_number = sequence_number + 1;
                work_item_present = 1;
                pthread_cond_signal(&work_cond);
                pthread_mutex_unlock(&work_mutex);
                break;
            }

            if (sequence_len + remaining_len > sequence_cap) {
                sequence_cap = (sequence_len + remaining_len) * 1.2 + 1024;
                sequence = realloc(sequence, sequence_cap);
                if (!sequence) exit(1);
            }
            memcpy(sequence + sequence_len, remaining_data, remaining_len);
            sequence_len += remaining_len;
            free(remaining_buffer);

            char *read_buffer = malloc(READ_SIZE);
            if (!read_buffer) exit(1);
            ssize_t bytes_read = read(STDIN_FILENO, read_buffer, READ_SIZE);

            if (bytes_read <= 0) {
                pthread_mutex_lock(&work_mutex);
                shared_work_item.data = NULL;
                work_item_present = 1;
                pthread_cond_signal(&work_cond);
                pthread_mutex_unlock(&work_mutex);
                free(read_buffer);
                goto process_last_sequence;
            }
            remaining_buffer = remaining_data = read_buffer;
            remaining_len = bytes_read;
        }
        
        free(remaining_buffer);
process_last_sequence:;

        if (sequence_number > 0 && sem_trywait(&cpu_cores_available) == 0) {
            pthread_t thread;
            pthread_create(&thread, NULL, process_sequences, NULL);
            pthread_detach(thread);
        }

        if (sequence_number > 0) {
            char *header_end = memchr(sequence, '\n', sequence_len);
            if (!header_end) continue;

            size_t header_len = header_end - sequence;
            char *seq_data = header_end + 1;
            size_t seq_data_len = sequence_len - header_len - 1;

            char *modified_sequence_data = NULL;
            size_t modified_len = 0;

            if (seq_data_len % (LINE_LENGTH + 1) == 0) {
                modified_sequence_data = malloc(seq_data_len + 1);
                if (!modified_sequence_data) exit(1);
                char *dest = modified_sequence_data;
                char *src = seq_data;
                for (size_t i = 0; i < seq_data_len; ++i) {
                     *dest++ = COMPLEMENT_LOOKUP[(unsigned char)*src++];
                }
                reverse(modified_sequence_data, seq_data_len);
                modified_len = seq_data_len;
                modified_sequence_data[modified_len++] = '\n';
            } else {
                char *temp_seq = malloc(seq_data_len);
                if (!temp_seq) exit(1);
                size_t temp_len = 0;
                for (size_t i = 0; i < seq_data_len; ++i) {
                    if (seq_data[i] != '\n') {
                        temp_seq[temp_len++] = COMPLEMENT_LOOKUP[(unsigned char)seq_data[i]];
                    }
                }
                reverse(temp_seq, temp_len);

                modified_len = temp_len + temp_len / LINE_LENGTH + 2;
                modified_sequence_data = malloc(modified_len);
                if (!modified_sequence_data) exit(1);
                size_t pos = 0;
                modified_sequence_data[pos++] = '\n';
                for (size_t i = 0; i < temp_len; i += LINE_LENGTH) {
                    size_t chunk = (i + LINE_LENGTH < temp_len) ? LINE_LENGTH : temp_len - i;
                    memcpy(modified_sequence_data + pos, temp_seq + i, chunk);
                    pos += chunk;
                    modified_sequence_data[pos++] = '\n';
                }
                modified_len = pos;
                free(temp_seq);
            }

            pthread_mutex_lock(&output_mutex);
            while (next_sequence_to_output < sequence_number) {
                pthread_cond_wait(&output_cond, &output_mutex);
            }

            write(STDOUT_FILENO, sequence, header_len);
            write(STDOUT_FILENO, modified_sequence_data, modified_len);

            next_sequence_to_output++;
            pthread_cond_broadcast(&output_cond);
            pthread_mutex_unlock(&output_mutex);
            
            free(modified_sequence_data);
        }
    }

    free(sequence);
    return NULL;
}

int main() {
    long n_cpus = sysconf(_SC_NPROCESSORS_ONLN);
    if (n_cpus < 1) n_cpus = 1;

    sem_init(&cpu_cores_available, 0, n_cpus - 1);

    char *initial_buffer = malloc(1);
    if (!initial_buffer) exit(1);
    initial_buffer[0] = '>'; // Start with a fake header start
    
    shared_work_item.data = initial_buffer;
    shared_work_item.size = 1;
    shared_work_item.sequence_number = 0;
    work_item_present = 1;
    
    process_sequences(NULL);

    for (int i = 0; i < n_cpus - 1; ++i) {
        sem_wait(&cpu_cores_available);
    }

    sem_destroy(&cpu_cores_available);
    pthread_mutex_destroy(&work_mutex);
    pthread_cond_destroy(&work_cond);
    pthread_mutex_destroy(&output_mutex);
    pthread_cond_destroy(&output_cond);
    
    return 0;
}

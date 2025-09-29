/*
 * The Computer Language Benchmarks Game
 * https://salsa.debian.org/benchmarksgame-team/benchmarksgame/
 *
 * Converted from Python to C by Claude
 * Original Python code by Joerg Baumann
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <stdint.h>
#include <pthread.h>
#include <unistd.h>

// Hash table for k-nucleotide frequency counting
typedef struct KmerEntry {
    uint64_t key;           // The encoded k-mer
    int count;              // Frequency count
    struct KmerEntry *next; // Next entry in the linked list (for hash collisions)
} KmerEntry;

typedef struct {
    KmerEntry **buckets;    // Array of hash table buckets
    int size;               // Number of buckets in the hash table
} HashTable;

// Context for worker threads
typedef struct {
    const unsigned char *sequence;
    int sequence_len;
    int start;
    int end;
    int frame_length;
    HashTable *hash_table;
    pthread_mutex_t *mutex;
} ThreadContext;

// Global constants
#define MAX_LINE 256
#define HASH_SIZE 2097152  // 2^21 - Adjust based on expected k-mer count

// Initialize a hash table
HashTable* create_hash_table(int size) {
    HashTable *table = (HashTable*)malloc(sizeof(HashTable));
    if (!table) {
        perror("Failed to allocate hash table");
        exit(EXIT_FAILURE);
    }
    
    table->size = size;
    table->buckets = (KmerEntry**)calloc(size, sizeof(KmerEntry*));
    if (!table->buckets) {
        perror("Failed to allocate hash table buckets");
        free(table);
        exit(EXIT_FAILURE);
    }
    
    return table;
}

// Free a hash table and all its entries
void free_hash_table(HashTable *table) {
    for (int i = 0; i < table->size; i++) {
        KmerEntry *entry = table->buckets[i];
        while (entry) {
            KmerEntry *next = entry->next;
            free(entry);
            entry = next;
        }
    }
    free(table->buckets);
    free(table);
}

// Hash function for k-mers
uint32_t hash_kmer(uint64_t key, int table_size) {
    // FNV-1a hash
    uint64_t hash = 14695981039346656037ULL;
    uint8_t *bytes = (uint8_t*)&key;
    
    for (int i = 0; i < sizeof(key); i++) {
        hash ^= bytes[i];
        hash *= 1099511628211ULL;
    }
    
    return (uint32_t)(hash % table_size);
}

// Insert or increment a k-mer in the hash table
void increment_kmer(HashTable *table, uint64_t key, pthread_mutex_t *mutex) {
    uint32_t hash = hash_kmer(key, table->size);
    
    // Lock only when accessing the hash table
    if (mutex) pthread_mutex_lock(mutex);
    
    // Look for existing entry
    KmerEntry *entry = table->buckets[hash];
    while (entry) {
        if (entry->key == key) {
            entry->count++;
            if (mutex) pthread_mutex_unlock(mutex);
            return;
        }
        entry = entry->next;
    }
    
    // Create new entry
    KmerEntry *new_entry = (KmerEntry*)malloc(sizeof(KmerEntry));
    if (!new_entry) {
        perror("Failed to allocate hash entry");
        if (mutex) pthread_mutex_unlock(mutex);
        exit(EXIT_FAILURE);
    }
    
    new_entry->key = key;
    new_entry->count = 1;
    new_entry->next = table->buckets[hash];
    table->buckets[hash] = new_entry;
    
    if (mutex) pthread_mutex_unlock(mutex);
}

// Get the count of a k-mer from the hash table
int get_kmer_count(HashTable *table, uint64_t key) {
    uint32_t hash = hash_kmer(key, table->size);
    
    KmerEntry *entry = table->buckets[hash];
    while (entry) {
        if (entry->key == key) {
            return entry->count;
        }
        entry = entry->next;
    }
    
    return 0;
}

// Encode a DNA sequence into a 64-bit integer (2 bits per nucleotide)
uint64_t encode_kmer(const unsigned char *sequence, int length) {
    uint64_t key = 0;
    
    for (int i = 0; i < length; i++) {
        uint64_t nucleotide;
        switch (sequence[i]) {
            case 'A': case 'a': nucleotide = 0; break;
            case 'C': case 'c': nucleotide = 1; break;
            case 'G': case 'g': nucleotide = 2; break;
            case 'T': case 't': nucleotide = 3; break;
            default: nucleotide = 0;  // Default for invalid input
        }
        key = (key << 2) | nucleotide;
    }
    
    return key;
}

// Decode a 64-bit integer back to a DNA sequence string
void decode_kmer(uint64_t key, int length, char *result) {
    static const char nucleotides[] = {'A', 'C', 'G', 'T'};
    
    for (int i = length - 1; i >= 0; i--) {
        result[i] = nucleotides[key & 0x3];
        key >>= 2;
    }
    result[length] = '\0';
}

// Worker thread function to count k-mers in a portion of the sequence
void* count_kmers_worker(void *arg) {
    ThreadContext *ctx = (ThreadContext*)arg;
    const unsigned char *sequence = ctx->sequence;
    int frame_length = ctx->frame_length;
    
    // Process k-mers in the assigned range
    for (int i = ctx->start; i <= ctx->end - frame_length; i++) {
        uint64_t key = encode_kmer(&sequence[i], frame_length);
        increment_kmer(ctx->hash_table, key, ctx->mutex);
    }
    
    return NULL;
}

// Count k-mers of a given length in the sequence using multiple threads
HashTable* count_kmers(const unsigned char *sequence, int sequence_len, int frame_length) {
    HashTable *table = create_hash_table(HASH_SIZE);
    
    // Determine number of threads to use
    int num_threads = sysconf(_SC_NPROCESSORS_ONLN);
    if (num_threads <= 0) num_threads = 1;
    
    // Create mutex for thread synchronization
    pthread_mutex_t mutex;
    pthread_mutex_init(&mutex, NULL);
    
    // Prepare thread contexts and threads
    pthread_t *threads = (pthread_t*)malloc(num_threads * sizeof(pthread_t));
    ThreadContext *contexts = (ThreadContext*)malloc(num_threads * sizeof(ThreadContext));
    
    if (!threads || !contexts) {
        perror("Failed to allocate thread resources");
        exit(EXIT_FAILURE);
    }
    
    // Divide the work among threads
    int chunk_size = (sequence_len - frame_length + 1) / num_threads;
    int remaining = (sequence_len - frame_length + 1) % num_threads;
    
    for (int i = 0; i < num_threads; i++) {
        contexts[i].sequence = sequence;
        contexts[i].sequence_len = sequence_len;
        contexts[i].start = i * chunk_size;
        contexts[i].end = (i + 1) * chunk_size;
        
        // Distribute remaining elements
        if (i < remaining) {
            contexts[i].start += i;
            contexts[i].end += i + 1;
        } else {
            contexts[i].start += remaining;
            contexts[i].end += remaining;
        }
        
        contexts[i].frame_length = frame_length;
        contexts[i].hash_table = table;
        contexts[i].mutex = &mutex;
        
        if (pthread_create(&threads[i], NULL, count_kmers_worker, &contexts[i]) != 0) {
            perror("Failed to create thread");
            exit(EXIT_FAILURE);
        }
    }
    
    // Wait for all threads to complete
    for (int i = 0; i < num_threads; i++) {
        pthread_join(threads[i], NULL);
    }
    
    // Clean up
    pthread_mutex_destroy(&mutex);
    free(threads);
    free(contexts);
    
    return table;
}

// Comparison function for sorting frequencies
typedef struct {
    char *kmer;
    int count;
} KmerCount;

int compare_frequency(const void *a, const void *b) {
    const KmerCount *ka = (const KmerCount*)a;
    const KmerCount *kb = (const KmerCount*)b;
    
    // Sort by count (descending) and then by k-mer (ascending) if counts are equal
    if (kb->count != ka->count) {
        return kb->count - ka->count;
    }
    return strcmp(ka->kmer, kb->kmer);
}

// Extract all k-mers and counts from the hash table
int extract_kmers(HashTable *table, int frame_length, KmerCount **results) {
    // Count total number of unique k-mers
    int total_kmers = 0;
    for (int i = 0; i < table->size; i++) {
        KmerEntry *entry = table->buckets[i];
        while (entry) {
            total_kmers++;
            entry = entry->next;
        }
    }
    
    // Allocate array for results
    *results = (KmerCount*)malloc(total_kmers * sizeof(KmerCount));
    if (!*results) {
        perror("Failed to allocate results array");
        exit(EXIT_FAILURE);
    }
    
    // Extract all k-mers and counts
    int index = 0;
    for (int i = 0; i < table->size; i++) {
        KmerEntry *entry = table->buckets[i];
        while (entry) {
            (*results)[index].kmer = (char*)malloc(frame_length + 1);
            if (!(*results)[index].kmer) {
                perror("Failed to allocate k-mer string");
                exit(EXIT_FAILURE);
            }
            
            decode_kmer(entry->key, frame_length, (*results)[index].kmer);
            (*results)[index].count = entry->count;
            
            index++;
            entry = entry->next;
        }
    }
    
    return total_kmers;
}

// Read the DNA sequence from a FASTA file
unsigned char* read_sequence(FILE *file, const char *header, int *length) {
    char line[MAX_LINE];
    unsigned char *sequence = NULL;
    int sequence_capacity = 0;
    *length = 0;
    
    // Find the header
    while (fgets(line, sizeof(line), file)) {
        if (line[0] == '>' && strstr(line, header) != NULL) {
            break;
        }
    }
    
    // Read the sequence
    while (fgets(line, sizeof(line), file)) {
        if (line[0] == '>') {
            break;
        }
        
        // Resize the buffer if needed
        int line_len = strlen(line);
        if (*length + line_len > sequence_capacity) {
            sequence_capacity = sequence_capacity ? sequence_capacity * 2 : 1024;
            sequence = (unsigned char*)realloc(sequence, sequence_capacity);
            if (!sequence) {
                perror("Failed to allocate sequence buffer");
                exit(EXIT_FAILURE);
            }
        }
        
        // Copy the line, skipping whitespace
        for (int i = 0; i < line_len; i++) {
            if (!isspace(line[i])) {
                sequence[(*length)++] = toupper(line[i]);
            }
        }
    }
    
    // Resize the buffer to the actual length
    sequence = (unsigned char*)realloc(sequence, *length + 1);
    sequence[*length] = '\0';
    
    return sequence;
}

// Print the frequency of k-mers (relative or absolute)
void print_frequencies(HashTable *table, int frame_length, int sequence_len, int relative) {
    KmerCount *results;
    int total_kmers = extract_kmers(table, frame_length, &results);
    
    // Sort by frequency
    qsort(results, total_kmers, sizeof(KmerCount), compare_frequency);
    
    // Print frequencies
    for (int i = 0; i < total_kmers; i++) {
        if (relative) {
            double percentage = 100.0 * results[i].count / (sequence_len - frame_length + 1);
            printf("%s %.3f\n", results[i].kmer, percentage);
        } else {
            printf("%d\t%s\n", results[i].count, results[i].kmer);
        }
    }
    
    // Clean up
    for (int i = 0; i < total_kmers; i++) {
        free(results[i].kmer);
    }
    free(results);
}

// Print the frequency of a specific set of k-mers
void print_specific_frequencies(HashTable *table, const char **kmers, int num_kmers, int frame_length, int sequence_len) {
    for (int i = 0; i < num_kmers; i++) {
        uint64_t key = encode_kmer((const unsigned char*)kmers[i], strlen(kmers[i]));
        int count = get_kmer_count(table, key);
        printf("%d\t%s\n", count, kmers[i]);
    }
}

int main() {
    // Read the DNA sequence from stdin
    int sequence_len;
    unsigned char *sequence = read_sequence(stdin, "THREE", &sequence_len);
    
    if (!sequence || sequence_len == 0) {
        fprintf(stderr, "Failed to read sequence or sequence is empty\n");
        return EXIT_FAILURE;
    }
    
    // Count 1-nucleotide frequencies and print
    HashTable *table1 = count_kmers(sequence, sequence_len, 1);
    print_frequencies(table1, 1, sequence_len, 1); // relative percentages
    free_hash_table(table1);
    printf("\n");
    
    // Count 2-nucleotide frequencies and print
    HashTable *table2 = count_kmers(sequence, sequence_len, 2);
    print_frequencies(table2, 2, sequence_len, 1); // relative percentages
    free_hash_table(table2);
    printf("\n");
    
    // Count and print frequencies of specific k-mers
    const char *specific_kmers[] = {
        "GGT", "GGTA", "GGTATT", "GGTATTTTAATT", "GGTATTTTAATTTATAGT"
    };
    int num_specific = sizeof(specific_kmers) / sizeof(specific_kmers[0]);
    
    for (int i = 0; i < num_specific; i++) {
        int len = strlen(specific_kmers[i]);
        HashTable *table = count_kmers(sequence, sequence_len, len);
        uint64_t key = encode_kmer((const unsigned char*)specific_kmers[i], len);
        int count = get_kmer_count(table, key);
        printf("%d\t%s\n", count, specific_kmers[i]);
        free_hash_table(table);
    }
    
    // Clean up
    free(sequence);
    
    return EXIT_SUCCESS;
}
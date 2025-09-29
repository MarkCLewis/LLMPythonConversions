/*
 * The Computer Language Benchmarks Game
 * https://salsa.debian.org/benchmarksgame-team/benchmarksgame/
 *
 * Converted from Python to C by Claude
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <stdbool.h>

#define MAX_LINE_LENGTH 256
#define MAX_SEQUENCE_LENGTH 10000000
#define HASH_TABLE_SIZE 2097152  // 2^21

// Hash table entry for storing k-mer counts
typedef struct Entry {
    char *key;
    int count;
    struct Entry *next;
} Entry;

// Hash table for storing k-mer counts
typedef struct {
    Entry **buckets;
    int size;
} HashTable;

// Create a new hash table
HashTable* create_hash_table(int size) {
    HashTable *table = (HashTable*)malloc(sizeof(HashTable));
    table->size = size;
    table->buckets = (Entry**)calloc(size, sizeof(Entry*));
    return table;
}

// Free the hash table and its entries
void free_hash_table(HashTable *table) {
    for (int i = 0; i < table->size; i++) {
        Entry *entry = table->buckets[i];
        while (entry) {
            Entry *next = entry->next;
            free(entry->key);
            free(entry);
            entry = next;
        }
    }
    free(table->buckets);
    free(table);
}

// Simple hash function for strings
unsigned int hash_string(const char *str, int len) {
    unsigned int hash = 5381;
    for (int i = 0; i < len; i++) {
        hash = ((hash << 5) + hash) + str[i];
    }
    return hash;
}

// Increment count for a k-mer in the hash table
void increment_count(HashTable *table, const char *key, int len) {
    unsigned int hash = hash_string(key, len) % table->size;
    
    // Look for existing entry
    Entry *entry = table->buckets[hash];
    while (entry) {
        if (strncmp(entry->key, key, len) == 0 && entry->key[len] == '\0') {
            entry->count++;
            return;
        }
        entry = entry->next;
    }
    
    // Create new entry
    entry = (Entry*)malloc(sizeof(Entry));
    entry->key = (char*)malloc(len + 1);
    strncpy(entry->key, key, len);
    entry->key[len] = '\0';
    entry->count = 1;
    entry->next = table->buckets[hash];
    table->buckets[hash] = entry;
}

// Get count for a specific k-mer
int get_count(HashTable *table, const char *key, int len) {
    unsigned int hash = hash_string(key, len) % table->size;
    
    Entry *entry = table->buckets[hash];
    while (entry) {
        if (strncmp(entry->key, key, len) == 0 && entry->key[len] == '\0') {
            return entry->count;
        }
        entry = entry->next;
    }
    
    return 0;  // Not found
}

// Structure for sorted frequency results
typedef struct {
    char *key;
    int count;
    double frequency;
} FrequencyResult;

// Comparison function for sorting frequencies in descending order
int compare_frequency(const void *a, const void *b) {
    FrequencyResult *fa = (FrequencyResult*)a;
    FrequencyResult *fb = (FrequencyResult*)b;
    
    if (fb->count != fa->count) {
        return fb->count - fa->count;  // Sort by count (descending)
    }
    return strcmp(fa->key, fb->key);   // Then by key (ascending)
}

// Count k-mers of specified length
HashTable* base_counts(const char *sequence, int len, int bases) {
    HashTable *counts = create_hash_table(HASH_TABLE_SIZE);
    int size = len + 1 - bases;
    
    for (int i = 0; i < size; i++) {
        increment_count(counts, sequence + i, bases);
    }
    
    return counts;
}

// Calculate and sort frequencies of k-mers
FrequencyResult* sorted_freq(const char *sequence, int len, int bases, int *result_count) {
    HashTable *counts = base_counts(sequence, len, bases);
    
    // Count number of entries
    int entry_count = 0;
    for (int i = 0; i < counts->size; i++) {
        Entry *entry = counts->buckets[i];
        while (entry) {
            entry_count++;
            entry = entry->next;
        }
    }
    
    // Allocate result array
    FrequencyResult *results = (FrequencyResult*)malloc(entry_count * sizeof(FrequencyResult));
    int index = 0;
    int size = len + 1 - bases;
    
    // Fill result array
    for (int i = 0; i < counts->size; i++) {
        Entry *entry = counts->buckets[i];
        while (entry) {
            results[index].key = strdup(entry->key);
            results[index].count = entry->count;
            results[index].frequency = 100.0 * entry->count / size;
            index++;
            entry = entry->next;
        }
    }
    
    // Sort results
    qsort(results, entry_count, sizeof(FrequencyResult), compare_frequency);
    
    *result_count = entry_count;
    free_hash_table(counts);
    return results;
}

// Get count of a specific k-mer
int specific_count(const char *sequence, int len, const char *code, int code_len) {
    HashTable *counts = base_counts(sequence, len, code_len);
    int count = get_count(counts, code, code_len);
    free_hash_table(counts);
    return count;
}

// Read sequence from stdin
char* read_sequence(int *len) {
    char line[MAX_LINE_LENGTH];
    bool found_header = false;
    char *sequence = (char*)malloc(MAX_SEQUENCE_LENGTH);
    *len = 0;
    
    // Find THREE header
    while (fgets(line, MAX_LINE_LENGTH, stdin)) {
        if (strncmp(line, ">THREE", 6) == 0) {
            found_header = true;
            break;
        }
    }
    
    if (!found_header) {
        free(sequence);
        return NULL;
    }
    
    // Read sequence lines
    while (fgets(line, MAX_LINE_LENGTH, stdin)) {
        if (line[0] == '>') {
            break;
        }
        
        int line_len = strlen(line);
        if (line[line_len - 1] == '\n') {
            line[--line_len] = '\0';
        }
        
        // Convert to uppercase and copy to sequence buffer
        for (int i = 0; i < line_len; i++) {
            sequence[*len] = toupper(line[i]);
            (*len)++;
        }
    }
    
    sequence[*len] = '\0';
    return sequence;
}

int main() {
    int sequence_len;
    char *sequence = read_sequence(&sequence_len);
    
    if (!sequence) {
        fprintf(stderr, "Failed to read sequence\n");
        return 1;
    }
    
    // Calculate and print frequencies for 1-mers and 2-mers
    for (int bases = 1; bases <= 2; bases++) {
        int result_count;
        FrequencyResult *results = sorted_freq(sequence, sequence_len, bases, &result_count);
        
        for (int i = 0; i < result_count; i++) {
            printf("%s %.3f\n", results[i].key, results[i].frequency);
        }
        printf("\n");
        
        // Free result memory
        for (int i = 0; i < result_count; i++) {
            free(results[i].key);
        }
        free(results);
    }
    
    // Calculate and print counts for specific sequences
    const char *specific_codes[] = {
        "GGT", "GGTA", "GGTATT", "GGTATTTTAATT", "GGTATTTTAATTTATAGT"
    };
    int num_codes = sizeof(specific_codes) / sizeof(specific_codes[0]);
    
    for (int i = 0; i < num_codes; i++) {
        int count = specific_count(sequence, sequence_len, specific_codes[i], strlen(specific_codes[i]));
        printf("%d\t%s\n", count, specific_codes[i]);
    }
    
    free(sequence);
    return 0;
}
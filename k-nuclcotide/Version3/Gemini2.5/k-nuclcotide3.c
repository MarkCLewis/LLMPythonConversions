// The Computer Language Benchmarks Game
// https://salsa.debian.org/benchmarksgame-team/benchmarksgame/
//
// Converted from Python to C by Gemini
// Original Python version by Joerg Baumann

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <pthread.h>
#include <unistd.h>
#include "uthash.h"

// --- Data Structures ---

// Hash table entry for k-mers where k is large (k > 6)
typedef struct ht_entry {
    uint64_t key;
    long value;
    UT_hash_handle hh;
} ht_entry;

// Data passed to each worker thread
typedef struct worker_data {
    const uint8_t* sequence;
    size_t seq_len;
    size_t start;
    size_t end;

    // Result storage for this thread
    long* freq1;
    long* freq2;
    long* freq3;
    long* freq4;
    long* freq6;
    ht_entry** freq12;
    ht_entry** freq18;
} worker_data;

// For sorting results before printing
typedef struct display_item {
    const char* name;
    long count;
} display_item;


// --- Global Data & Constants ---

static const uint8_t translation[256] = {
    ['A'] = 0, ['a'] = 0,
    ['C'] = 1, ['c'] = 1,
    ['G'] = 2, ['g'] = 2,
    ['T'] = 3, ['t'] = 3,
};

static const char* mono_nucleotides[] = {"A", "C", "G", "T"};
static const char* di_nucleotides[] = {
    "AA", "AC", "AG", "AT", "CA", "CC", "CG", "CT",
    "GA", "GC", "GG", "GT", "TA", "TC", "TG", "TT"
};
static const char* k_nucleotides[] = {
    "GGT", "GGTA", "GGTATT", "GGTATTTTAATT", "GGTATTTTAATTTATAGT"
};


// --- Helper Functions ---

// Converts a nucleotide string like "GGT" to its 2-bit representation
uint64_t str_to_bits(const char* s) {
    uint64_t bits = 0;
    for (; *s; ++s) {
        bits = (bits << 2) | translation[(int)(*s)];
    }
    return bits;
}

// Comparison function for qsort to sort display items by count (desc) then name (asc)
int compare_items(const void* a, const void* b) {
    display_item* item1 = (display_item*)a;
    display_item* item2 = (display_item*)b;
    if (item1->count > item2->count) return -1;
    if (item1->count < item2->count) return 1;
    return strcmp(item1->name, item2->name);
}

// --- Core Logic ---

// Reads the sequence from stdin, skipping headers until ">THREE" is found
uint8_t* read_sequence(size_t* final_len) {
    char line[256];
    const char* header = ">THREE";
    
    // Find header
    while (fgets(line, sizeof(line), stdin) && strncmp(line, header, strlen(header)) != 0);

    size_t capacity = 1024 * 1024;
    uint8_t* sequence = malloc(capacity);
    size_t len = 0;

    // Read sequence data
    while (fgets(line, sizeof(line), stdin) && line[0] != '>') {
        for (char* c = line; *c; ++c) {
            if (*c == 'A' || *c == 'C' || *c == 'G' || *c == 'T' ||
                *c == 'a' || *c == 'c' || *c == 'g' || *c == 't') {
                if (len == capacity) {
                    capacity *= 2;
                    sequence = realloc(sequence, capacity);
                }
                sequence[len++] = translation[(int)(*c)];
            }
        }
    }
    *final_len = len;
    return sequence;
}

// The main function executed by each worker thread
void* count_frequencies_worker(void* arg) {
    worker_data* data = (worker_data*)arg;

    const int frames[] = {18, 12, 6, 4, 3, 2, 1};
    uint64_t masks[] = {
        (1ULL << (2*18)) - 1, (1ULL << (2*12)) - 1, (1ULL << (2*6)) - 1,
        (1ULL << (2*4)) - 1,  (1ULL << (2*3)) - 1,  (1ULL << (2*2)) - 1,
        (1ULL << (2*1)) - 1
    };
    
    int largest_frame = frames[0];
    uint64_t bits = 0;

    // Prime the sliding window. Each thread needs to read `largest_frame - 1`
    // bytes before its official start index to correctly count the first k-mer.
    size_t prime_start = (data->start == 0) ? 0 : data->start - largest_frame + 1;
    for (size_t i = prime_start; i < data->start; ++i) {
        bits = (bits << 2) | data->sequence[i];
    }

    // Main sliding window loop. Count k-mers that END in this thread's chunk.
    for (size_t i = data->start; i < data->end; ++i) {
        bits = (bits << 2) | data->sequence[i];
        if (i < largest_frame - 1) continue;

        // Count 18-mer
        uint64_t key18 = bits & masks[0];
        ht_entry* e18;
        HASH_FIND(hh, *(data->freq18), &key18, sizeof(uint64_t), e18);
        if (e18) e18->value++;
        else {
            e18 = malloc(sizeof(ht_entry));
            e18->key = key18; e18->value = 1;
            HASH_ADD(hh, *(data->freq18), key, sizeof(uint64_t), e18);
        }

        // Count 12-mer
        uint64_t key12 = bits & masks[1];
        ht_entry* e12;
        HASH_FIND(hh, *(data->freq12), &key12, sizeof(uint64_t), e12);
        if (e12) e12->value++;
        else {
            e12 = malloc(sizeof(ht_entry));
            e12->key = key12; e12->value = 1;
            HASH_ADD(hh, *(data->freq12), key, sizeof(uint64_t), e12);
        }

        data->freq6[bits & masks[2]]++;
        data->freq4[bits & masks[3]]++;
        data->freq3[bits & masks[4]]++;
        data->freq2[bits & masks[5]]++;
        data->freq1[bits & masks[6]]++;
    }
    return NULL;
}


// --- Main Execution ---

int main() {
    size_t seq_len;
    uint8_t* sequence = read_sequence(&seq_len);

    int n_threads = sysconf(_SC_NPROCESSORS_ONLN);
    if (seq_len < 2048) n_threads = 1;

    pthread_t threads[n_threads];
    worker_data all_data[n_threads];

    size_t chunk_start = 0;
    for (int i = 0; i < n_threads; ++i) {
        size_t chunk_end = seq_len * (i + 1) / n_threads;
        
        all_data[i].sequence = sequence;
        all_data[i].seq_len = seq_len;
        all_data[i].start = chunk_start;
        all_data[i].end = chunk_end;

        all_data[i].freq1 = calloc(1 << (2*1), sizeof(long));
        all_data[i].freq2 = calloc(1 << (2*2), sizeof(long));
        all_data[i].freq3 = calloc(1 << (2*3), sizeof(long));
        all_data[i].freq4 = calloc(1 << (2*4), sizeof(long));
        all_data[i].freq6 = calloc(1 << (2*6), sizeof(long));
        all_data[i].freq12 = NULL;
        all_data[i].freq18 = NULL;
        
        pthread_create(&threads[i], NULL, count_frequencies_worker, &all_data[i]);
        chunk_start = chunk_end;
    }

    // --- Aggregate Results ---
    long* final_freq1 = all_data[0].freq1;
    long* final_freq2 = all_data[0].freq2;
    long* final_freq3 = all_data[0].freq3;
    long* final_freq4 = all_data[0].freq4;
    long* final_freq6 = all_data[0].freq6;
    ht_entry** final_freq12 = &all_data[0].freq12;
    ht_entry** final_freq18 = &all_data[0].freq18;

    for (int i = 1; i < n_threads; ++i) {
        pthread_join(threads[i], NULL);
        for(int j = 0; j < (1<<(2*1)); ++j) final_freq1[j] += all_data[i].freq1[j];
        for(int j = 0; j < (1<<(2*2)); ++j) final_freq2[j] += all_data[i].freq2[j];
        for(int j = 0; j < (1<<(2*3)); ++j) final_freq3[j] += all_data[i].freq3[j];
        for(int j = 0; j < (1<<(2*4)); ++j) final_freq4[j] += all_data[i].freq4[j];
        for(int j = 0; j < (1<<(2*6)); ++j) final_freq6[j] += all_data[i].freq6[j];
        
        ht_entry *current, *tmp;
        HASH_ITER(hh, all_data[i].freq12, current, tmp) {
            ht_entry* master_e;
            HASH_FIND(hh, *final_freq12, &current->key, sizeof(uint64_t), master_e);
            if (master_e) master_e->value += current->value;
            else HASH_ADD(hh, *final_freq12, key, sizeof(uint64_t), current);
        }
        HASH_ITER(hh, all_data[i].freq18, current, tmp) {
            ht_entry* master_e;
            HASH_FIND(hh, *final_freq18, &current->key, sizeof(uint64_t), master_e);
            if (master_e) master_e->value += current->value;
            else HASH_ADD(hh, *final_freq18, key, sizeof(uint64_t), current);
        }
    }
    pthread_join(threads[0], NULL);


    // --- Display Results ---
    double total_count1 = seq_len;
    display_item items1[4];
    for (int i = 0; i < 4; ++i) {
        items1[i] = (display_item){mono_nucleotides[i], final_freq1[str_to_bits(mono_nucleotides[i])]};
    }
    qsort(items1, 4, sizeof(display_item), compare_items);
    for (int i = 0; i < 4; ++i) {
        printf("%s %.3f\n", items1[i].name, 100.0 * items1[i].count / total_count1);
    }
    printf("\n");

    double total_count2 = (seq_len > 1) ? (seq_len - 1) : 0;
    display_item items2[16];
    for (int i = 0; i < 16; ++i) {
        items2[i] = (display_item){di_nucleotides[i], final_freq2[str_to_bits(di_nucleotides[i])]};
    }
    qsort(items2, 16, sizeof(display_item), compare_items);
    for (int i = 0; i < 16; ++i) {
        printf("%s %.3f\n", items2[i].name, 100.0 * items2[i].count / total_count2);
    }
    printf("\n");

    long count;
    ht_entry* e;

    count = final_freq3[str_to_bits(k_nucleotides[0])];
    printf("%ld\t%s\n", count, k_nucleotides[0]);
    
    count = final_freq4[str_to_bits(k_nucleotides[1])];
    printf("%ld\t%s\n", count, k_nucleotides[1]);

    count = final_freq6[str_to_bits(k_nucleotides[2])];
    printf("%ld\t%s\n", count, k_nucleotides[2]);

    uint64_t key12 = str_to_bits(k_nucleotides[3]);
    HASH_FIND(hh, *final_freq12, &key12, sizeof(uint64_t), e);
    printf("%ld\t%s\n", e ? e->value : 0, k_nucleotides[3]);
    
    uint64_t key18 = str_to_bits(k_nucleotides[4]);
    HASH_FIND(hh, *final_freq18, &key18, sizeof(uint64_t), e);
    printf("%ld\t%s\n", e ? e->value : 0, k_nucleotides[4]);

    // --- Cleanup ---
    // Note: Memory cleanup is omitted as the program is about to exit.
    // In a long-running application, all allocated memory should be freed.
    
    return 0;
}

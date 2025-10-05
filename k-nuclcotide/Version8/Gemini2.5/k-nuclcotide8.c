#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

// A node for the hash table's linked list (to handle collisions)
typedef struct Node {
    char* key;
    int value;
    struct Node* next;
} Node;

// The hash table structure
typedef struct {
    int size;
    Node** table;
} Hashtable;

// Comparison function for qsort to sort by frequency (desc) then key (asc)
int compare_freq(const void* a, const void* b) {
    Node* nodeA = *(Node**)a;
    Node* nodeB = *(Node**)b;
    if (nodeA->value < nodeB->value) return 1;
    if (nodeA->value > nodeB->value) return -1;
    return strcmp(nodeA->key, nodeB->key);
}

// Simple hash function (djb2) for strings
unsigned long hash_function(const char* str) {
    unsigned long hash = 5381;
    int c;
    while ((c = *str++)) {
        hash = ((hash << 5) + hash) + c; // hash * 33 + c
    }
    return hash;
}

// Create and initialize a hash table
Hashtable* create_hashtable(int size) {
    Hashtable* ht = malloc(sizeof(Hashtable));
    ht->size = size;
    ht->table = calloc(size, sizeof(Node*));
    return ht;
}

// Insert or increment a key in the hash table
void insert_or_increment(Hashtable* ht, const char* key) {
    unsigned long index = hash_function(key) % ht->size;
    Node* entry = ht->table[index];
    
    while (entry != NULL) {
        if (strcmp(entry->key, key) == 0) {
            entry->value++;
            return;
        }
        entry = entry->next;
    }
    
    // Key not found, create a new entry
    Node* newNode = malloc(sizeof(Node));
    newNode->key = strdup(key);
    newNode->value = 1;
    newNode->next = ht->table[index];
    ht->table[index] = newNode;
}

// Free all memory used by the hash table
void free_hashtable(Hashtable* ht) {
    for (int i = 0; i < ht->size; ++i) {
        Node* entry = ht->table[i];
        while (entry != NULL) {
            Node* temp = entry;
            entry = entry->next;
            free(temp->key);
            free(temp);
        }
    }
    free(ht->table);
    free(ht);
}

// Reads the sequence from stdin after the ">THREE" marker
char* read_sequence() {
    char* line = NULL;
    size_t len = 0;
    ssize_t read;

    // Skip until >THREE
    while ((read = getline(&line, &len, stdin)) != -1) {
        if (strncmp(line, ">THREE", 6) == 0) {
            break;
        }
    }

    size_t seq_capacity = 256 * 1024; // Initial capacity
    char* seq = malloc(seq_capacity);
    size_t seq_len = 0;
    
    if (seq == NULL) {
        perror("Failed to allocate sequence buffer");
        exit(EXIT_FAILURE);
    }
    seq[0] = '\0';

    // Read sequence lines
    while ((read = getline(&line, &len, stdin)) != -1) {
        if (line[0] == '>') {
            break;
        }
        // Exclude newline character
        if (read > 0 && line[read - 1] == '\n') {
            line[read - 1] = '\0';
            read--;
        }
        
        if (seq_len + read >= seq_capacity) {
            seq_capacity *= 2;
            char* new_seq = realloc(seq, seq_capacity);
            if (new_seq == NULL) {
                perror("Failed to reallocate sequence buffer");
                free(seq);
                exit(EXIT_FAILURE);
            }
            seq = new_seq;
        }
        
        // Append and convert to uppercase
        for (ssize_t i = 0; i < read; ++i) {
            seq[seq_len + i] = toupper(line[i]);
        }
        seq_len += read;
    }
    seq[seq_len] = '\0';
    
    free(line);
    return seq;
}

// Counts all k-nucleotides of a given length
Hashtable* count_bases(const char* seq, int k) {
    size_t seq_len = strlen(seq);
    if (seq_len < k) return NULL;
    
    Hashtable* ht = create_hashtable(1024);
    char* buffer = malloc(k + 1);
    buffer[k] = '\0';
    
    for (size_t i = 0; i <= seq_len - k; ++i) {
        strncpy(buffer, seq + i, k);
        insert_or_increment(ht, buffer);
    }
    
    free(buffer);
    return ht;
}

// Prints the frequencies of k-nucleotides, sorted by count
void print_sorted_freq(const char* seq, int k) {
    Hashtable* ht = count_bases(seq, k);
    if (!ht) return;

    int count = 0;
    for (int i = 0; i < ht->size; ++i) {
        for (Node* entry = ht->table[i]; entry != NULL; entry = entry->next) {
            count++;
        }
    }

    Node** sorted_nodes = malloc(count * sizeof(Node*));
    int j = 0;
    for (int i = 0; i < ht->size; ++i) {
        for (Node* entry = ht->table[i]; entry != NULL; entry = entry->next) {
            sorted_nodes[j++] = entry;
        }
    }

    qsort(sorted_nodes, count, sizeof(Node*), compare_freq);

    double total = (double)strlen(seq) - k + 1;
    for (int i = 0; i < count; ++i) {
        printf("%s %.3f\n", sorted_nodes[i]->key, (sorted_nodes[i]->value * 100.0) / total);
    }
    printf("\n");

    free(sorted_nodes);
    free_hashtable(ht);
}

// Counts occurrences of a specific nucleotide sequence
int count_specific_code(const char* seq, const char* code) {
    int count = 0;
    size_t code_len = strlen(code);
    const char* ptr = seq;
    while ((ptr = strstr(ptr, code)) != NULL) {
        count++;
        ptr += 1; // Advance by one to find overlapping sequences
    }
    return count;
}


int main() {
    char* seq = read_sequence();

    print_sorted_freq(seq, 1);
    print_sorted_freq(seq, 2);

    const char* codes[] = {
        "GGT", "GGTA", "GGTATT", "GGTATTTTAATT", "GGTATTTTAATTTATAGT", NULL
    };
    for (int i = 0; codes[i] != NULL; ++i) {
        printf("%d\t%s\n", count_specific_code(seq, codes[i]), codes[i]);
    }
    
    free(seq);
    return 0;
}

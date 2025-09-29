// The Computer Language Benchmarks Game
// https://salsa.debian.org/benchmarksgame-team/benchmarksgame/
//
// Converted from Python to C by Claude

#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <pthread.h>
#define PCRE2_CODE_UNIT_WIDTH 8
#include <pcre2.h>
#include <sys/sysinfo.h>

// Structure for storing pattern and match count
typedef struct {
    const char *pattern;
    int count;
} CountInfo;

// Structure for storing pattern and replacement
typedef struct {
    const char *pattern;
    const char *replacement;
} ReplaceInfo;

// Structure for worker thread arguments
typedef struct {
    int thread_id;
    int num_threads;
    int task_type; // 0 for count, 1 for replace
    const char *sequences;
    size_t sequences_length;
    CountInfo *count_info;
    int count_info_size;
    ReplaceInfo *replace_info;
    int replace_info_size;
    size_t *result_length; // For replacement task
} WorkerArgs;

// Function to perform regex replacement
char* replace(const char *pattern, const char *replacement,
             const char *src, size_t src_length,
             size_t *dst_length) {
    
    int error_code;
    PCRE2_SIZE error_offset;
    pcre2_code *regex;

    // Compile the pattern
    regex = pcre2_compile((PCRE2_SPTR)pattern, PCRE2_ZERO_TERMINATED,
                          0, &error_code, &error_offset, NULL);
    
    if (regex == NULL) {
        PCRE2_UCHAR error_buffer[256];
        pcre2_get_error_message(error_code, error_buffer, sizeof(error_buffer));
        fprintf(stderr, "PCRE2 compilation failed at offset %zu: %s\n", error_offset, error_buffer);
        return NULL;
    }

    // Enable JIT compilation to make matching faster
    pcre2_jit_compile(regex, PCRE2_JIT_COMPLETE);

    // Allocate a buffer for the result - make it larger to accommodate growth
    size_t dst_capacity = src_length * 1.1;
    char *dst = malloc(dst_capacity);
    if (!dst) {
        fprintf(stderr, "Memory allocation failed\n");
        pcre2_code_free(regex);
        return NULL;
    }

    // Perform the substitution
    PCRE2_SIZE dst_size = dst_capacity;
    int rc = pcre2_substitute(regex, (PCRE2_SPTR)src, src_length,
                            0, PCRE2_SUBSTITUTE_GLOBAL, NULL, NULL,
                            (PCRE2_SPTR)replacement, PCRE2_ZERO_TERMINATED,
                            (PCRE2_UCHAR*)dst, &dst_size);

    if (rc < 0) {
        // If the buffer is too small, try again with a larger buffer
        if (rc == PCRE2_ERROR_NOMEMORY) {
            free(dst);
            dst_capacity = src_length * 2;
            dst = malloc(dst_capacity);
            if (!dst) {
                fprintf(stderr, "Memory allocation failed\n");
                pcre2_code_free(regex);
                return NULL;
            }
            dst_size = dst_capacity;
            rc = pcre2_substitute(regex, (PCRE2_SPTR)src, src_length,
                                 0, PCRE2_SUBSTITUTE_GLOBAL, NULL, NULL,
                                 (PCRE2_SPTR)replacement, PCRE2_ZERO_TERMINATED,
                                 (PCRE2_UCHAR*)dst, &dst_size);
        }
        
        if (rc < 0) {
            fprintf(stderr, "Substitution failed with code %d\n", rc);
            free(dst);
            pcre2_code_free(regex);
            return NULL;
        }
    }

    pcre2_code_free(regex);
    *dst_length = dst_size;
    return dst;
}

// Function to count matches
int count_matches(const char *pattern, const char *src, size_t src_length) {
    int error_code;
    PCRE2_SIZE error_offset;
    pcre2_code *regex;

    // Compile the pattern
    regex = pcre2_compile((PCRE2_SPTR)pattern, PCRE2_ZERO_TERMINATED,
                          0, &error_code, &error_offset, NULL);
    
    if (regex == NULL) {
        PCRE2_UCHAR error_buffer[256];
        pcre2_get_error_message(error_code, error_buffer, sizeof(error_buffer));
        fprintf(stderr, "PCRE2 compilation failed at offset %zu: %s\n", error_offset, error_buffer);
        return -1;
    }

    // Enable JIT compilation to make matching faster
    pcre2_jit_compile(regex, PCRE2_JIT_COMPLETE);

    // Create match data
    pcre2_match_data *match_data = pcre2_match_data_create_from_pattern(regex, NULL);
    
    PCRE2_SIZE start_offset = 0;
    int match_count = 0;
    
    // Count all matches
    while (1) {
        int rc = pcre2_jit_match(regex, (PCRE2_SPTR)src, src_length,
                               start_offset, 0, match_data, NULL);
        if (rc < 0) {
            if (rc != PCRE2_ERROR_NOMATCH) {
                fprintf(stderr, "Matching error %d\n", rc);
            }
            break;
        }
        
        match_count++;
        
        PCRE2_SIZE *ovector = pcre2_get_ovector_pointer(match_data);
        start_offset = ovector[1];
        
        // Break if we've matched a zero-length string at the end
        if (ovector[0] == ovector[1]) {
            if (ovector[0] == src_length) break;
            start_offset++; // Move past this position to avoid infinite loop
        }
    }
    
    pcre2_match_data_free(match_data);
    pcre2_code_free(regex);
    
    return match_count;
}

// Worker thread function
void* worker_thread(void *arg) {
    WorkerArgs *args = (WorkerArgs*)arg;
    
    // Process based on task type
    if (args->task_type == 0) { // Count task
        // Each thread processes a subset of patterns
        for (int i = args->thread_id; i < args->count_info_size; i += args->num_threads) {
            args->count_info[i].count = count_matches(
                args->count_info[i].pattern, 
                args->sequences, 
                args->sequences_length
            );
        }
    } else { // Replace task
        // Only one thread should do the replacements
        if (args->thread_id == 0) {
            const char *current_src = args->sequences;
            size_t current_length = args->sequences_length;
            char *result = NULL;
            
            for (int i = 0; i < args->replace_info_size; i++) {
                size_t new_length;
                char *new_result = replace(
                    args->replace_info[i].pattern,
                    args->replace_info[i].replacement,
                    current_src,
                    current_length,
                    &new_length
                );
                
                if (new_result) {
                    // If we have a previous result, free it
                    if (result) {
                        free((void*)current_src);
                    }
                    
                    // Update the current source for the next iteration
                    current_src = new_result;
                    current_length = new_length;
                    result = new_result;
                } else {
                    fprintf(stderr, "Replacement failed for pattern %s\n", 
                            args->replace_info[i].pattern);
                    break;
                }
            }
            
            // Save the final length
            *args->result_length = current_length;
            
            // Free the last result if it's not the original sequence
            if (result && result != args->sequences) {
                free(result);
            }
        }
    }
    
    return NULL;
}

int main() {
    // Read the entire input from stdin
    char *input = NULL;
    size_t input_capacity = 0;
    size_t input_length = 0;
    char buffer[8192];
    size_t bytes_read;
    
    while ((bytes_read = fread(buffer, 1, sizeof(buffer), stdin)) > 0) {
        // Ensure we have enough capacity
        if (input_length + bytes_read > input_capacity) {
            input_capacity = input_capacity ? input_capacity * 2 : 16384;
            input = realloc(input, input_capacity);
            if (!input) {
                fprintf(stderr, "Memory allocation failed\n");
                return 1;
            }
        }
        
        // Copy the data
        memcpy(input + input_length, buffer, bytes_read);
        input_length += bytes_read;
    }
    
    // Ensure null termination
    if (input_length >= input_capacity) {
        input = realloc(input, input_length + 1);
        if (!input) {
            fprintf(stderr, "Memory allocation failed\n");
            return 1;
        }
    }
    input[input_length] = '\0';
    
    // Remove sequence descriptions and newlines
    size_t sequences_length;
    char *sequences = replace(">.*\\n|\\n", "", input, input_length, &sequences_length);
    
    if (!sequences) {
        fprintf(stderr, "Failed to remove headers and newlines\n");
        free(input);
        return 1;
    }
    
    // Define patterns to count
    CountInfo count_info[] = {
        {"agggtaaa|tttaccct", 0},
        {"[cgt]gggtaaa|tttaccc[acg]", 0},
        {"a[act]ggtaaa|tttacc[agt]t", 0},
        {"ag[act]gtaaa|tttac[agt]ct", 0},
        {"agg[act]taaa|ttta[agt]cct", 0},
        {"aggg[acg]aaa|ttt[cgt]ccct", 0},
        {"agggt[cgt]aa|tt[acg]accct", 0},
        {"agggta[cgt]a|t[acg]taccct", 0},
        {"agggtaa[cgt]|[acg]ttaccct", 0}
    };
    int count_info_size = sizeof(count_info) / sizeof(count_info[0]);
    
    // Define patterns to replace
    ReplaceInfo replace_info[] = {
        {"tHa[Nt]", "<4>"},
        {"aND|caN|Ha[DS]|WaS", "<3>"},
        {"a[NSt]|BY", "<2>"},
        {"<[^>]*>", "|"},
        {"\\|[^|][^|]*\\|", "-"}
    };
    int replace_info_size = sizeof(replace_info) / sizeof(replace_info[0]);
    
    // Get the number of available processors
    int num_threads = get_nprocs();
    if (num_threads < 1) num_threads = 1;
    
    // Create and initialize worker thread arguments
    WorkerArgs count_args = {
        .task_type = 0, // Count task
        .sequences = sequences,
        .sequences_length = sequences_length,
        .count_info = count_info,
        .count_info_size = count_info_size
    };
    
    // Result length for replacement task
    size_t result_length = 0;
    
    WorkerArgs replace_args = {
        .task_type = 1, // Replace task
        .sequences = sequences,
        .sequences_length = sequences_length,
        .replace_info = replace_info,
        .replace_info_size = replace_info_size,
        .result_length = &result_length
    };
    
    // Create worker threads for counting
    pthread_t *threads = malloc(num_threads * sizeof(pthread_t));
    WorkerArgs *thread_args = malloc(num_threads * sizeof(WorkerArgs));
    
    if (!threads || !thread_args) {
        fprintf(stderr, "Memory allocation failed\n");
        free(input);
        free(sequences);
        free(threads);
        free(thread_args);
        return 1;
    }
    
    // Start counting threads
    for (int i = 0; i < num_threads; i++) {
        thread_args[i] = count_args;
        thread_args[i].thread_id = i;
        thread_args[i].num_threads = num_threads;
        
        if (pthread_create(&threads[i], NULL, worker_thread, &thread_args[i]) != 0) {
            fprintf(stderr, "Failed to create thread %d\n", i);
            free(input);
            free(sequences);
            free(threads);
            free(thread_args);
            return 1;
        }
    }
    
    // Wait for all counting threads to finish
    for (int i = 0; i < num_threads; i++) {
        pthread_join(threads[i], NULL);
    }
    
    // Start replacement thread (just use the first thread)
    replace_args.thread_id = 0;
    replace_args.num_threads = 1;
    
    if (pthread_create(&threads[0], NULL, worker_thread, &replace_args) != 0) {
        fprintf(stderr, "Failed to create replacement thread\n");
        free(input);
        free(sequences);
        free(threads);
        free(thread_args);
        return 1;
    }
    
    // Wait for the replacement thread to finish
    pthread_join(threads[0], NULL);
    
    // Print the results
    for (int i = 0; i < count_info_size; i++) {
        printf("%s %d\n", count_info[i].pattern, count_info[i].count);
    }
    
    printf("\n");
    printf("%zu\n", input_length);
    printf("%zu\n", sequences_length);
    printf("%zu\n", result_length);
    
    // Clean up
    free(input);
    free(sequences);
    free(threads);
    free(thread_args);
    
    return 0;
}
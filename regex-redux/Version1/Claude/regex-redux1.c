// The Computer Language Benchmarks Game
// https://salsa.debian.org/benchmarksgame-team/benchmarksgame/
//
// Converted from Python to C by Claude
// Uses PCRE2 library for regular expressions

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>
#include <unistd.h>
#define PCRE2_CODE_UNIT_WIDTH 8
#include <pcre2.h>

// Structure to store a regex pattern and its count
typedef struct {
    const char *pattern;
    int count;
} PatternCount;

// Structure for thread arguments
typedef struct {
    const char *seq;
    size_t seq_len;
    const char *pattern;
    int *result;
} ThreadArg;

// Structure for substitution pattern
typedef struct {
    const char *pattern;
    const char *replacement;
} SubstPattern;

// Function to count pattern matches in a string
int count_matches(const char *seq, size_t seq_len, const char *pattern) {
    pcre2_code *re;
    pcre2_match_data *match_data;
    PCRE2_SIZE offset = 0;
    int count = 0;
    int error_code;
    PCRE2_SIZE error_offset;
    
    // Compile the regular expression
    re = pcre2_compile((PCRE2_SPTR)pattern, PCRE2_ZERO_TERMINATED, 
                       PCRE2_MULTILINE, &error_code, &error_offset, NULL);
    if (re == NULL) {
        PCRE2_UCHAR error_buffer[256];
        pcre2_get_error_message(error_code, error_buffer, sizeof(error_buffer));
        fprintf(stderr, "PCRE2 compilation failed at offset %zu: %s\n", error_offset, error_buffer);
        return -1;
    }
    
    // Create match data block
    match_data = pcre2_match_data_create_from_pattern(re, NULL);
    
    // Count all matches
    while (1) {
        int rc = pcre2_match(re, (PCRE2_SPTR)seq, seq_len, offset, 0, match_data, NULL);
        if (rc < 0) {
            if (rc != PCRE2_ERROR_NOMATCH) {
                fprintf(stderr, "Matching error %d\n", rc);
            }
            break;
        }
        count++;
        PCRE2_SIZE *ovector = pcre2_get_ovector_pointer(match_data);
        offset = ovector[1];  // Start at end of previous match
        
        // Break if we've reached the end of the string
        if (ovector[0] == ovector[1]) {
            if (ovector[0] == seq_len) break;
            offset++;
        }
    }
    
    // Clean up
    pcre2_match_data_free(match_data);
    pcre2_code_free(re);
    
    return count;
}

// Thread function for counting matches
void* count_matches_thread(void* arg) {
    ThreadArg* thread_arg = (ThreadArg*)arg;
    *(thread_arg->result) = count_matches(thread_arg->seq, thread_arg->seq_len, thread_arg->pattern);
    return NULL;
}

// Function to perform regex substitution
char* regex_substitute(const char *seq, size_t seq_len, const char *pattern, const char *replacement) {
    pcre2_code *re;
    pcre2_match_data *match_data;
    int error_code;
    PCRE2_SIZE error_offset;
    
    // Compile the regular expression
    re = pcre2_compile((PCRE2_SPTR)pattern, PCRE2_ZERO_TERMINATED, 
                       PCRE2_MULTILINE, &error_code, &error_offset, NULL);
    if (re == NULL) {
        PCRE2_UCHAR error_buffer[256];
        pcre2_get_error_message(error_code, error_buffer, sizeof(error_buffer));
        fprintf(stderr, "PCRE2 compilation failed at offset %zu: %s\n", error_offset, error_buffer);
        return NULL;
    }
    
    // Create match data block
    match_data = pcre2_match_data_create_from_pattern(re, NULL);
    
    // Create output buffer for substitution
    PCRE2_SIZE output_len = seq_len * 2; // Initial guess at required size
    PCRE2_UCHAR *output = (PCRE2_UCHAR*)malloc(output_len * sizeof(PCRE2_UCHAR));
    if (output == NULL) {
        fprintf(stderr, "Memory allocation failed\n");
        pcre2_match_data_free(match_data);
        pcre2_code_free(re);
        return NULL;
    }
    
    // Perform the substitution
    PCRE2_SIZE new_len = output_len;
    int rc = pcre2_substitute(re, (PCRE2_SPTR)seq, seq_len, 0, 
                              PCRE2_SUBSTITUTE_GLOBAL, match_data, NULL,
                              (PCRE2_SPTR)replacement, PCRE2_ZERO_TERMINATED,
                              output, &new_len);
    
    // Check for buffer too small
    if (rc == PCRE2_ERROR_NOMEMORY) {
        // Retry with larger buffer (double the size again)
        free(output);
        output_len = seq_len * 4;
        output = (PCRE2_UCHAR*)malloc(output_len * sizeof(PCRE2_UCHAR));
        if (output == NULL) {
            fprintf(stderr, "Memory allocation failed\n");
            pcre2_match_data_free(match_data);
            pcre2_code_free(re);
            return NULL;
        }
        
        new_len = output_len;
        rc = pcre2_substitute(re, (PCRE2_SPTR)seq, seq_len, 0, 
                              PCRE2_SUBSTITUTE_GLOBAL, match_data, NULL,
                              (PCRE2_SPTR)replacement, PCRE2_ZERO_TERMINATED,
                              output, &new_len);
    }
    
    if (rc < 0) {
        fprintf(stderr, "Substitution failed with code %d\n", rc);
        free(output);
        pcre2_match_data_free(match_data);
        pcre2_code_free(re);
        return NULL;
    }
    
    // Clean up
    pcre2_match_data_free(match_data);
    pcre2_code_free(re);
    
    // Ensure null-termination and return
    output[new_len] = '\0';
    return (char*)output;
}

// Function to remove sequences headers and newlines
char* remove_headers_and_newlines(const char *seq, size_t seq_len, size_t *new_len) {
    pcre2_code *re;
    int error_code;
    PCRE2_SIZE error_offset;
    
    // Compile the pattern to match headers and newlines
    const char *pattern = ">.*\\n|\\n";
    re = pcre2_compile((PCRE2_SPTR)pattern, PCRE2_ZERO_TERMINATED, 
                       PCRE2_MULTILINE, &error_code, &error_offset, NULL);
    if (re == NULL) {
        PCRE2_UCHAR error_buffer[256];
        pcre2_get_error_message(error_code, error_buffer, sizeof(error_buffer));
        fprintf(stderr, "PCRE2 compilation failed at offset %zu: %s\n", error_offset, error_buffer);
        return NULL;
    }
    
    // Create match data block
    pcre2_match_data *match_data = pcre2_match_data_create_from_pattern(re, NULL);
    
    // Allocate output buffer (same size as input initially)
    PCRE2_SIZE output_len = seq_len;
    PCRE2_UCHAR *output = (PCRE2_UCHAR*)malloc((output_len + 1) * sizeof(PCRE2_UCHAR));
    if (output == NULL) {
        fprintf(stderr, "Memory allocation failed\n");
        pcre2_match_data_free(match_data);
        pcre2_code_free(re);
        return NULL;
    }
    
    // Perform the substitution with empty replacement
    PCRE2_SIZE new_size = output_len;
    int rc = pcre2_substitute(re, (PCRE2_SPTR)seq, seq_len, 0, 
                              PCRE2_SUBSTITUTE_GLOBAL, match_data, NULL,
                              (PCRE2_SPTR)"", 0, output, &new_size);
    
    // Check for errors
    if (rc < 0) {
        fprintf(stderr, "Substitution failed with code %d\n", rc);
        free(output);
        pcre2_match_data_free(match_data);
        pcre2_code_free(re);
        return NULL;
    }
    
    // Clean up
    pcre2_match_data_free(match_data);
    pcre2_code_free(re);
    
    // Ensure null-termination
    output[new_size] = '\0';
    *new_len = new_size;
    
    return (char*)output;
}

int main() {
    // Read the entire input
    char *seq = NULL;
    size_t seq_len = 0;
    size_t capacity = 0;
    char buffer[8192];
    size_t bytes_read;
    
    while ((bytes_read = fread(buffer, 1, sizeof(buffer), stdin)) > 0) {
        if (seq_len + bytes_read > capacity) {
            capacity = seq_len + bytes_read + 8192;
            char *new_seq = (char*)realloc(seq, capacity);
            if (!new_seq) {
                fprintf(stderr, "Memory allocation failed\n");
                free(seq);
                return 1;
            }
            seq = new_seq;
        }
        memcpy(seq + seq_len, buffer, bytes_read);
        seq_len += bytes_read;
    }
    
    // Ensure null-termination
    if (seq_len >= capacity) {
        char *new_seq = (char*)realloc(seq, seq_len + 1);
        if (!new_seq) {
            fprintf(stderr, "Memory allocation failed\n");
            free(seq);
            return 1;
        }
        seq = new_seq;
    }
    seq[seq_len] = '\0';
    
    // Store original length
    size_t ilen = seq_len;
    
    // Remove headers and newlines
    size_t clen;
    char *clean_seq = remove_headers_and_newlines(seq, seq_len, &clen);
    if (!clean_seq) {
        fprintf(stderr, "Failed to clean sequence\n");
        free(seq);
        return 1;
    }
    
    // Free original sequence, we only need the cleaned version now
    free(seq);
    seq = clean_seq;
    seq_len = clen;
    
    // Define patterns to search for
    const char *patterns[] = {
        "agggtaaa|tttaccct",
        "[cgt]gggtaaa|tttaccc[acg]",
        "a[act]ggtaaa|tttacc[agt]t",
        "ag[act]gtaaa|tttac[agt]ct",
        "agg[act]taaa|ttta[agt]cct",
        "aggg[acg]aaa|ttt[cgt]ccct",
        "agggt[cgt]aa|tt[acg]accct",
        "agggta[cgt]a|t[acg]taccct",
        "agggtaa[cgt]|[acg]ttaccct"
    };
    int num_patterns = sizeof(patterns) / sizeof(patterns[0]);
    
    // Create result array and threads
    int *counts = (int*)calloc(num_patterns, sizeof(int));
    pthread_t *threads = (pthread_t*)malloc(num_patterns * sizeof(pthread_t));
    ThreadArg *args = (ThreadArg*)malloc(num_patterns * sizeof(ThreadArg));
    
    if (!counts || !threads || !args) {
        fprintf(stderr, "Memory allocation failed\n");
        free(seq);
        free(counts);
        free(threads);
        free(args);
        return 1;
    }
    
    // Start threads for pattern matching
    for (int i = 0; i < num_patterns; i++) {
        args[i].seq = seq;
        args[i].seq_len = seq_len;
        args[i].pattern = patterns[i];
        args[i].result = &counts[i];
        
        if (pthread_create(&threads[i], NULL, count_matches_thread, &args[i]) != 0) {
            fprintf(stderr, "Failed to create thread %d\n", i);
            return 1;
        }
    }
    
    // Wait for all threads to complete
    for (int i = 0; i < num_patterns; i++) {
        pthread_join(threads[i], NULL);
        printf("%s %d\n", patterns[i], counts[i]);
    }
    
    // Define substitution patterns
    SubstPattern subst[] = {
        {"tHa[Nt]", "<4>"},
        {"aND|caN|Ha[DS]|WaS", "<3>"},
        {"a[NSt]|BY", "<2>"},
        {"<[^>]*>", "|"},
        {"\\|[^|][^|]*\\|", "-"}
    };
    int num_subst = sizeof(subst) / sizeof(subst[0]);
    
    // Perform substitutions
    for (int i = 0; i < num_subst; i++) {
        char *new_seq = regex_substitute(seq, seq_len, subst[i].pattern, subst[i].replacement);
        if (new_seq) {
            free(seq);
            seq = new_seq;
            seq_len = strlen(seq);
        } else {
            fprintf(stderr, "Substitution failed for pattern: %s\n", subst[i].pattern);
        }
    }
    
    // Print results
    printf("\n");
    printf("%zu\n", ilen);
    printf("%zu\n", clen);
    printf("%zu\n", seq_len);
    
    // Clean up
    free(seq);
    free(counts);
    free(threads);
    free(args);
    
    return 0;
}
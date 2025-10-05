#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>
#include <unistd.h>

// This must be defined before including pcre2.h for 8-bit character support.
#define PCRE2_CODE_UNIT_WIDTH 8
#include <pcre2.h>

// Represents a pattern to count and stores its result.
typedef struct {
    const char* pattern;
    int count;
} CountInfo;

// Data structure shared among all worker threads.
typedef struct {
    const char* sequences;
    size_t sequences_len;
    CountInfo* count_info;
    int num_patterns;
    int next_task_idx;
    pthread_mutex_t task_mutex;
} ThreadData;


/**
 * @brief Reads the entire content from stdin into a dynamically allocated buffer.
 * @param final_len A pointer to store the final size of the read content.
 * @return A pointer to the allocated buffer containing stdin content. The caller must free this buffer.
 */
char* read_stdin(size_t* final_len) {
    size_t capacity = 16384; // Start with 16KB
    size_t len = 0;
    char* buffer = malloc(capacity);
    if (!buffer) {
        perror("malloc for stdin buffer failed");
        exit(1);
    }

    size_t bytes_read;
    while ((bytes_read = fread(buffer + len, 1, capacity - len, stdin)) > 0) {
        len += bytes_read;
        if (len == capacity) {
            capacity *= 2;
            char* new_buffer = realloc(buffer, capacity);
            if (!new_buffer) {
                perror("realloc for stdin buffer failed");
                free(buffer);
                exit(1);
            }
            buffer = new_buffer;
        }
    }

    *final_len = len;
    return buffer;
}


/**
 * @brief Performs a global regex substitution using PCRE2 with JIT compilation.
 * @return A pointer to a newly allocated buffer with the result. The caller must free this buffer.
 */
char* pcre2_replace(
    const char* pattern,
    const char* replacement,
    const char* subject,
    size_t subject_len,
    size_t* result_len
) {
    pcre2_code* re;
    int error_code;
    PCRE2_SIZE error_offset;

    // Compile the regex pattern.
    re = pcre2_compile(
        (PCRE2_SPTR)pattern, PCRE2_ZERO_TERMINATED, 0,
        &error_code, &error_offset, NULL);
    if (re == NULL) {
        PCRE2_UCHAR err_buf[256];
        pcre2_get_error_message(error_code, err_buf, sizeof(err_buf));
        fprintf(stderr, "PCRE2 compilation failed at offset %d: %s\n", (int)error_offset, err_buf);
        exit(1);
    }

    // JIT compile the pattern for significantly faster matching.
    if (pcre2_jit_compile(re, PCRE2_JIT_COMPLETE) < 0) {
        fprintf(stderr, "PCRE2 JIT compilation failed\n");
        pcre2_code_free(re);
        exit(1);
    }

    // Allocate an output buffer. We use a heuristic size to avoid a second pass.
    PCRE2_SIZE out_len = (PCRE2_SIZE)(subject_len * 1.5);
    PCRE2_UCHAR* out_buf = malloc(out_len);
    if (!out_buf) {
        perror("malloc for replace buffer failed");
        exit(1);
    }

    // Perform the substitution.
    int rc = pcre2_substitute(
        re, (PCRE2_SPTR)subject, subject_len, 0,
        PCRE2_SUBSTITUTE_GLOBAL, NULL, NULL,
        (PCRE2_SPTR)replacement, PCRE2_ZERO_TERMINATED,
        out_buf, &out_len
    );

    // If the buffer was too small, realloc and try again.
    if (rc == PCRE2_ERROR_NOMEMORY) {
        out_buf = realloc(out_buf, out_len);
        rc = pcre2_substitute(
            re, (PCRE2_SPTR)subject, subject_len, 0, PCRE2_SUBSTITUTE_GLOBAL,
            NULL, NULL, (PCRE2_SPTR)replacement, PCRE2_ZERO_TERMINATED,
            out_buf, &out_len);
    }

    if (rc < 0) {
        fprintf(stderr, "PCRE2 substitution failed with error code %d\n", rc);
        pcre2_code_free(re);
        free(out_buf);
        exit(1);
    }

    pcre2_code_free(re);
    *result_len = out_len;
    return (char*)out_buf;
}


/**
 * @brief The main function for worker threads.
 * Each worker repeatedly fetches a task, counts regex matches, and stores the result.
 */
void* count_matches_worker(void* arg) {
    ThreadData* data = (ThreadData*)arg;

    pcre2_match_data* match_data = pcre2_match_data_create(1, NULL);
    PCRE2_SIZE* ovector = pcre2_get_ovector_pointer(match_data);

    while (1) {
        // Get the next task index in a thread-safe manner.
        pthread_mutex_lock(&data->task_mutex);
        int task_idx = data->next_task_idx++;
        pthread_mutex_unlock(&data->task_mutex);

        if (task_idx >= data->num_patterns) {
            break; // No more tasks.
        }

        const char* pattern = data->count_info[task_idx].pattern;

        // Compile and JIT the pattern for this task.
        pcre2_code* re;
        int error_code;
        PCRE2_SIZE error_offset;
        re = pcre2_compile((PCRE2_SPTR)pattern, PCRE2_ZERO_TERMINATED, 0, &error_code, &error_offset, NULL);
        if(!re) exit(1);
        if(pcre2_jit_compile(re, PCRE2_JIT_COMPLETE) < 0) exit(1);

        int count = 0;
        PCRE2_SIZE offset = 0;

        // Loop through the subject string to find all non-overlapping matches.
        while(pcre2_jit_match(re, (PCRE2_SPTR)data->sequences, data->sequences_len,
            offset, 0, match_data, NULL) >= 0) {
            count++;
            offset = ovector[1]; // Start next search after this match.
        }

        data->count_info[task_idx].count = count;
        pcre2_code_free(re);
    }

    pcre2_match_data_free(match_data);
    return NULL;
}


int main() {
    // 1. Read all input from stdin.
    size_t input_len;
    char* input_str = read_stdin(&input_len);

    // 2. Clean sequence data by removing headers and newlines.
    size_t sequences_len;
    char* sequences = pcre2_replace(">.*\\n|\\n", "", input_str, input_len, &sequences_len);
    free(input_str);

    // 3. Perform the five sequential IUPAC replacements.
    const char* replacements[][2] = {
        {"tHa[Nt]", "<4>"}, {"aND|caN|Ha[DS]|WaS", "<3>"}, {"a[NSt]|BY", "<2>"},
        {"<[^>]*>", "|"},   {"\\|[^|][^|]*\\|", "-"}
    };

    char* temp_in = sequences;
    size_t temp_in_len = sequences_len;
    char* temp_out = NULL;
    size_t postreplace_len;

    for (int i = 0; i < 5; ++i) {
        temp_out = pcre2_replace(replacements[i][0], replacements[i][1], temp_in, temp_in_len, &postreplace_len);
        if (temp_in != sequences) {
            free(temp_in);
        }
        temp_in = temp_out;
        temp_in_len = postreplace_len;
    }
    char* final_replaced_string = temp_out;


    // 4. Set up data for parallel counting tasks.
    CountInfo count_info[] = {
        {"agggtaaa|tttaccct", 0}, {"[cgt]gggtaaa|tttaccc[acg]", 0},
        {"a[act]ggtaaa|tttacc[agt]t", 0}, {"ag[act]gtaaa|tttac[agt]ct", 0},
        {"agg[act]taaa|ttta[agt]cct", 0}, {"aggg[acg]aaa|ttt[cgt]ccct", 0},
        {"agggt[cgt]aa|tt[acg]accct", 0}, {"agggta[cgt]a|t[acg]taccct", 0},
        {"agggtaa[cgt]|[acg]ttaccct", 0}
    };
    int num_patterns = sizeof(count_info) / sizeof(CountInfo);

    int n_threads = sysconf(_SC_NPROCESSORS_ONLN);
    if (n_threads <= 0) n_threads = 2;

    pthread_t threads[n_threads];
    ThreadData thread_data = {
        .sequences = sequences, .sequences_len = sequences_len,
        .count_info = count_info, .num_patterns = num_patterns,
        .next_task_idx = 0
    };
    pthread_mutex_init(&thread_data.task_mutex, NULL);


    // 5. Create and run worker threads.
    for (int i = 0; i < n_threads; ++i) {
        pthread_create(&threads[i], NULL, count_matches_worker, &thread_data);
    }

    // 6. Wait for all threads to complete.
    for (int i = 0; i < n_threads; ++i) {
        pthread_join(threads[i], NULL);
    }

    // 7. Print the results in the required format.
    for (int i = 0; i < num_patterns; ++i) {
        printf("%s %d\n", count_info[i].pattern, count_info[i].count);
    }

    printf("\n%zu\n%zu\n%zu\n", input_len, sequences_len, postreplace_len);

    // 8. Clean up all allocated resources.
    free(sequences);
    free(final_replaced_string);
    pthread_mutex_destroy(&thread_data.task_mutex);

    return 0;
}

#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <pcre2.h>
#include <pthread.h>
#include <unistd.h>

#define THREAD_COUNT 8
#define PATTERN_COUNT 9

typedef struct {
    const char *pattern;
    const char *subject;
    size_t length;
    int count;
} MatchTask;

typedef struct {
    const char *pattern;
    const char *replacement;
} SubstRule;

const char *patterns[PATTERN_COUNT] = {
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

SubstRule substitutions[] = {
    {"tHa[Nt]", "<4>"},
    {"aND|caN|Ha[DS]|WaS", "<3>"},
    {"a[NSt]|BY", "<2>"},
    {"<[^>]*>", "|"},
    {"\\|[^|][^|]*\\|", "-"}
};

void *count_matches(void *arg) {
    MatchTask *task = (MatchTask *)arg;

    pcre2_code *re = pcre2_compile((PCRE2_SPTR)task->pattern, PCRE2_ZERO_TERMINATED, 0, NULL, NULL, NULL);
    if (!re) pthread_exit(NULL);
    pcre2_jit_compile(re, PCRE2_JIT_COMPLETE);

    pcre2_match_data *mdata = pcre2_match_data_create(1, NULL);
    size_t offset = 0;
    int count = 0;

    while (pcre2_jit_match(re, (PCRE2_SPTR)task->subject, task->length, offset, 0, mdata, NULL) >= 0) {
        PCRE2_SIZE *ov = pcre2_get_ovector_pointer(mdata);
        offset = ov[1];
        count++;
    }

    task->count = count;
    pcre2_match_data_free(mdata);
    pcre2_code_free(re);
    pthread_exit(NULL);
}

char *regex_substitute(const char *input, const char *pattern, const char *replacement) {
    pcre2_code *re = pcre2_compile((PCRE2_SPTR)pattern, PCRE2_ZERO_TERMINATED, 0, NULL, NULL, NULL);
    pcre2_jit_compile(re, PCRE2_JIT_COMPLETE);

    size_t outlen = strlen(input) * 2;
    char *output = malloc(outlen);
    PCRE2_SIZE len = outlen;

    pcre2_substitute(
        re,
        (PCRE2_SPTR)input, strlen(input),
        0,
        PCRE2_SUBSTITUTE_GLOBAL,
        NULL, NULL,
        (PCRE2_SPTR)replacement, PCRE2_ZERO_TERMINATED,
        (PCRE2_UCHAR *)output, &len
    );

    pcre2_code_free(re);
    output[len] = '\0';
    return output;
}

char *strip_headers_and_newlines(const char *input, size_t len, size_t *new_len) {
    char *cleaned = malloc(len + 1);
    size_t j = 0;
    for (size_t i = 0; i < len; ++i) {
        if (input[i] == '>') {
            while (i < len && input[i] != '\n') i++;
        } else if (input[i] != '\n') {
            cleaned[j++] = input[i];
        }
    }
    cleaned[j] = '\0';
    *new_len = j;
    return cleaned;
}

int main() {
    // Read stdin into buffer
    size_t capacity = 1 << 20;
    size_t length = 0;
    char *input = malloc(capacity);
    int ch;
    while ((ch = getchar()) != EOF) {
        if (length + 1 >= capacity) {
            capacity *= 2;
            input = realloc(input, capacity);
        }
        input[length++] = (char)ch;
    }
    input[length] = '\0';

    // Strip headers and newlines
    size_t clen;
    char *sequence = strip_headers_and_newlines(input, length, &clen);

    // Multithreaded match counting
    MatchTask tasks[PATTERN_COUNT];
    pthread_t threads[PATTERN_COUNT];

    for (int i = 0; i < PATTERN_COUNT; i++) {
        tasks[i] = (MatchTask){ patterns[i], sequence, clen, 0 };
        pthread_create(&threads[i], NULL, count_matches, &tasks[i]);
    }

    for (int i = 0; i < PATTERN_COUNT; i++) {
        pthread_join(threads[i], NULL);
        printf("%s %d\n", tasks[i].pattern, tasks[i].count);
    }

    // Serial substitutions
    char *modified = strdup(sequence);
    for (size_t i = 0; i < sizeof(substitutions) / sizeof(substitutions[0]); ++i) {
        char *tmp = regex_substitute(modified, substitutions[i].pattern, substitutions[i].replacement);
        free(modified);
        modified = tmp;
    }

    printf("\n%zu\n", length);
    printf("%zu\n", clen);
    printf("%zu\n", strlen(modified));

    free(input);
    free(sequence);
    free(modified);
    return 0;
}

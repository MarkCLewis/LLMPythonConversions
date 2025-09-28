// regex-redux.c - Converted from Python to C for performance
#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <regex.h>
#include <pthread.h>

#define NUM_VARIANTS 9

const char* variants[NUM_VARIANTS] = {
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

const char* substitutions[][2] = {
    { "tHa[Nt]", "<4>" },
    { "aND|caN|Ha[DS]|WaS", "<3>" },
    { "a[NSt]|BY", "<2>" },
    { "<[^>]*>", "|" },
    { "\\|[^|][^|]*\\|", "-" }
};

typedef struct {
    const char* pattern;
    const char* sequence;
    int count;
} RegexTask;

void* count_pattern(void* arg) {
    RegexTask* task = (RegexTask*)arg;
    regex_t regex;
    regcomp(&regex, task->pattern, REG_EXTENDED | REG_NEWLINE | REG_ICASE);

    const char* cursor = task->sequence;
    regmatch_t match;
    task->count = 0;

    while (regexec(&regex, cursor, 1, &match, 0) == 0) {
        task->count++;
        cursor += match.rm_eo;
    }

    regfree(&regex);
    return NULL;
}

char* read_stdin(size_t* size) {
    size_t cap = 1 << 20;
    size_t len = 0;
    char* buf = malloc(cap);
    int c;
    while ((c = getchar()) != EOF) {
        if (len + 1 >= cap) {
            cap *= 2;
            buf = realloc(buf, cap);
        }
        buf[len++] = (char)c;
    }
    buf[len] = '\0';
    *size = len;
    return buf;
}

char* regex_substitute(const char* text, const char* pattern, const char* replacement) {
    regex_t regex;
    regcomp(&regex, pattern, REG_EXTENDED);
    regmatch_t match;

    size_t cap = strlen(text) * 2 + 1;
    char* result = malloc(cap);
    size_t out_index = 0;

    const char* cursor = text;

    while (regexec(&regex, cursor, 1, &match, 0) == 0) {
        size_t start = match.rm_so;
        size_t end = match.rm_eo;

        strncpy(&result[out_index], cursor, start);
        out_index += start;

        strcpy(&result[out_index], replacement);
        out_index += strlen(replacement);

        cursor += end;
    }

    strcpy(&result[out_index], cursor);
    regfree(&regex);
    return result;
}

int main() {
    size_t ilen;
    char* input = read_stdin(&ilen);

    // Remove headers and newlines
    char* sequence = malloc(ilen + 1);
    size_t sindex = 0;
    for (size_t i = 0; i < ilen; ++i) {
        if (input[i] == '>') {
            while (i < ilen && input[i] != '\n') i++;
        } else if (input[i] != '\n') {
            sequence[sindex++] = input[i];
        }
    }
    sequence[sindex] = '\0';
    size_t clen = strlen(sequence);

    // Count matches using threads
    pthread_t threads[NUM_VARIANTS];
    RegexTask tasks[NUM_VARIANTS];

    for (int i = 0; i < NUM_VARIANTS; i++) {
        tasks[i].pattern = variants[i];
        tasks[i].sequence = sequence;
        pthread_create(&threads[i], NULL, count_pattern, &tasks[i]);
    }

    for (int i = 0; i < NUM_VARIANTS; i++) {
        pthread_join(threads[i], NULL);
        printf("%s %d\n", variants[i], tasks[i].count);
    }

    // Apply substitutions
    char* modified = strdup(sequence);
    for (int i = 0; i < 5; i++) {
        char* replaced = regex_substitute(modified, substitutions[i][0], substitutions[i][1]);
        free(modified);
        modified = replaced;
    }

    printf("\n%zu\n", ilen);
    printf("%zu\n", clen);
    printf("%zu\n", strlen(modified));

    free(modified);
    free(sequence);
    free(input);
    return 0;
}

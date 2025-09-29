/* The Computer Language Benchmarks Game
 * https://salsa.debian.org/benchmarksgame-team/benchmarksgame/
 *
 * Converted from Python to C by Claude
 * Based on the Python version contributed by Antoine Pitrou
 * modified by Dominique Wahli, Daniel Nanz, Joerg Baumann, and Jonathan Ultis
 */

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <pthread.h>
#include <unistd.h>

/* Tree node structure */
typedef struct node_t {
    struct node_t* left;
    struct node_t* right;
} node_t;

/* Thread task structure */
typedef struct {
    int depth;
    int iterations;
    int64_t check_sum;
} task_t;

/* Create a binary tree of specified depth */
node_t* make_tree(int depth) {
    node_t* node = (node_t*)malloc(sizeof(node_t));
    if (node == NULL) {
        fprintf(stderr, "Memory allocation failed\n");
        exit(1);
    }
    
    if (depth > 0) {
        node->left = make_tree(depth - 1);
        node->right = make_tree(depth - 1);
    } else {
        node->left = NULL;
        node->right = NULL;
    }
    
    return node;
}

/* Check a tree and return node count */
int check_tree(node_t* node) {
    if (node->left == NULL) {
        return 1;
    } else {
        return 1 + check_tree(node->left) + check_tree(node->right);
    }
}

/* Free memory used by a tree */
void free_tree(node_t* node) {
    if (node == NULL) return;
    
    if (node->left != NULL) {
        free_tree(node->left);
        free_tree(node->right);
    }
    
    free(node);
}

/* Make and check a single tree */
int make_check(int depth) {
    node_t* tree = make_tree(depth);
    int result = check_tree(tree);
    free_tree(tree);
    return result;
}

/* Thread function to make and check multiple trees */
void* process_chunk(void* arg) {
    task_t* task = (task_t*)arg;
    int64_t sum = 0;
    
    for (int i = 0; i < task->iterations; i++) {
        sum += make_check(task->depth);
    }
    
    task->check_sum = sum;
    return NULL;
}

int main(int argc, char* argv[]) {
    if (argc < 2) {
        fprintf(stderr, "Usage: %s <n>\n", argv[0]);
        return 1;
    }
    
    int n = atoi(argv[1]);
    int min_depth = 4;
    int max_depth = (min_depth + 2 > n) ? min_depth + 2 : n;
    int stretch_depth = max_depth + 1;
    int mmd = max_depth + min_depth;
    
    /* Calculate number of CPU cores available */
    int num_procs = sysconf(_SC_NPROCESSORS_ONLN);
    if (num_procs <= 0) num_procs = 1;
    
    /* Stretch tree */
    {
        int stretch_check = make_check(stretch_depth);
        printf("stretch tree of depth %d\t check: %d\n", stretch_depth, stretch_check);
    }
    
    /* Create long-lived tree */
    node_t* long_lived_tree = make_tree(max_depth);
    
    /* Process trees of different depths */
    for (int d = min_depth; d < stretch_depth; d += 2) {
        int iterations = 1 << (mmd - d);
        
        /* Determine how to parallelize the work */
        int num_threads = num_procs;
        if (num_threads > iterations) num_threads = iterations;
        if (num_threads < 1) num_threads = 1;
        
        /* Create thread arguments */
        task_t* tasks = (task_t*)malloc(num_threads * sizeof(task_t));
        pthread_t* threads = (pthread_t*)malloc(num_threads * sizeof(pthread_t));
        
        /* Divide iterations among threads */
        int base_iterations_per_thread = iterations / num_threads;
        int remainder = iterations % num_threads;
        
        for (int i = 0; i < num_threads; i++) {
            tasks[i].depth = d;
            tasks[i].iterations = base_iterations_per_thread + (i < remainder ? 1 : 0);
            tasks[i].check_sum = 0;
            
            /* Create thread */
            if (pthread_create(&threads[i], NULL, process_chunk, &tasks[i]) != 0) {
                fprintf(stderr, "Failed to create thread %d\n", i);
                exit(1);
            }
        }
        
        /* Wait for all threads to complete */
        int64_t total_check = 0;
        for (int i = 0; i < num_threads; i++) {
            pthread_join(threads[i], NULL);
            total_check += tasks[i].check_sum;
        }
        
        printf("%d\t trees of depth %d\t check: %ld\n", iterations, d, total_check);
        
        /* Cleanup */
        free(tasks);
        free(threads);
    }
    
    /* Check long-lived tree */
    printf("long lived tree of depth %d\t check: %d\n", 
           max_depth, check_tree(long_lived_tree));
    free_tree(long_lived_tree);
    
    return 0;
}
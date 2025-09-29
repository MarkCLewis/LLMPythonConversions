/* The Computer Language Benchmarks Game
 * https://salsa.debian.org/benchmarksgame-team/benchmarksgame/
 *
 * Converted from Python to C by Claude
 * Based on the Python version contributed by Antoine Pitrou
 * and modified by Dominique Wahli and Heinrich Acker
 */

#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>

/* Tree node structure */
typedef struct node {
    struct node* left;
    struct node* right;
} node_t;

/* Create a binary tree of specified depth */
node_t* make_tree(int depth) {
    if (depth == 0) {
        return NULL;
    }

    node_t* node = (node_t*)malloc(sizeof(node_t));
    if (!node) {
        fprintf(stderr, "Memory allocation failed\n");
        exit(1);
    }

    depth--;
    node->left = make_tree(depth);
    node->right = make_tree(depth);
    return node;
}

/* Count nodes in a binary tree */
int check_tree(node_t* node) {
    if (node == NULL) {
        return 0;
    }
    return 1 + check_tree(node->left) + check_tree(node->right);
}

/* Free memory used by a tree */
void free_tree(node_t* node) {
    if (node == NULL) {
        return;
    }
    free_tree(node->left);
    free_tree(node->right);
    free(node);
}

/* Thread task structure */
typedef struct {
    int depth;
    int iterations;
    int check_sum;
} task_t;

/* Thread function to create and check multiple trees */
void* process_trees(void* arg) {
    task_t* task = (task_t*)arg;
    int check = 0;
    
    for (int i = 1; i <= task->iterations; i++) {
        node_t* tree = make_tree(task->depth);
        check += check_tree(tree);
        free_tree(tree);
    }
    
    task->check_sum = check;
    return NULL;
}

int main(int argc, char* argv[]) {
    if (argc != 2) {
        fprintf(stderr, "Usage: %s <n>\n", argv[0]);
        return 1;
    }
    
    int n = atoi(argv[1]);
    int min_depth = 4;
    int max_depth = (min_depth + 2 > n) ? min_depth + 2 : n;
    int stretch_depth = max_depth + 1;
    
    /* Stretch depth tree */
    {
        node_t* stretch_tree = make_tree(stretch_depth);
        printf("stretch tree of depth %d\t check: %d\n", 
               stretch_depth, check_tree(stretch_tree));
        free_tree(stretch_tree);
    }
    
    /* Create long-lived tree */
    node_t* long_lived_tree = make_tree(max_depth);
    
    /* Process trees of various depths in parallel */
    int num_depths = ((max_depth - min_depth) / 2) + 1;
    task_t* tasks = (task_t*)malloc(num_depths * sizeof(task_t));
    pthread_t* threads = (pthread_t*)malloc(num_depths * sizeof(pthread_t));
    
    for (int i = 0; i < num_depths; i++) {
        int depth = min_depth + i * 2;
        int iterations = 1 << (max_depth - depth + min_depth);
        
        tasks[i].depth = depth;
        tasks[i].iterations = iterations;
        tasks[i].check_sum = 0;
        
        /* Create thread to process this depth */
        if (pthread_create(&threads[i], NULL, process_trees, &tasks[i]) != 0) {
            fprintf(stderr, "Failed to create thread for depth %d\n", depth);
            return 1;
        }
    }
    
    /* Wait for all threads to complete and print results */
    for (int i = 0; i < num_depths; i++) {
        pthread_join(threads[i], NULL);
        int depth = min_depth + i * 2;
        int iterations = 1 << (max_depth - depth + min_depth);
        printf("%d\t trees of depth %d\t check: %d\n", 
               iterations, depth, tasks[i].check_sum);
    }
    
    /* Check long-lived tree */
    printf("long lived tree of depth %d\t check: %d\n", 
           max_depth, check_tree(long_lived_tree));
    free_tree(long_lived_tree);
    
    /* Clean up */
    free(tasks);
    free(threads);
    
    return 0;
}
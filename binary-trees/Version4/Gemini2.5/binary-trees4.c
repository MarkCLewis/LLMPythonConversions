// The Computer Language Benchmarks Game
// https://salsa.debian.org/benchmarksgame-team/benchmarksgame/
//
// contributed by Antoine Pitrou
// modified by Dominique Wahli and Daniel Nanz
// modified by Joerg Baumann
// modified by Jonathan Ultis
// converted to C by Gemini

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <pthread.h>
#include <unistd.h>
#include <math.h>

// A simple memory pool for fast allocation of tree nodes
typedef struct apr_pool_t {
    void* memory;
    void* free;
    size_t size;
} apr_pool_t;

apr_pool_t* create_pool(size_t size) {
    apr_pool_t* pool = malloc(sizeof(apr_pool_t));
    if (!pool) return NULL;
    pool->size = size;
    pool->memory = malloc(size);
    if (!pool->memory) {
        free(pool);
        return NULL;
    }
    pool->free = pool->memory;
    return pool;
}

void destroy_pool(apr_pool_t* pool) {
    if (pool) {
        free(pool->memory);
        free(pool);
    }
}

// Tree node structure
typedef struct TreeNode {
    struct TreeNode* left;
    struct TreeNode* right;
} TreeNode;

// Allocate a node from the pool. Assumes pool has enough space.
TreeNode* alloc_node(apr_pool_t* pool) {
    TreeNode* node = (TreeNode*)pool->free;
    pool->free = (char*)pool->free + sizeof(TreeNode);
    node->left = node->right = NULL;
    return node;
}

// Create a binary tree of a given depth
TreeNode* make_tree(int depth, apr_pool_t* pool) {
    if (depth > 0) {
        TreeNode* node = alloc_node(pool);
        node->left = make_tree(depth - 1, pool);
        node->right = make_tree(depth - 1, pool);
        return node;
    }
    return alloc_node(pool); // Leaf node
}

// Check the nodes of a tree
int check_tree(const TreeNode* node) {
    if (node->left) {
        return 1 + check_tree(node->left) + check_tree(node->right);
    }
    return 1; // Leaf node
}

// Data structure to pass arguments to worker threads
typedef struct {
    int depth;
    int iterations;
    long result;
} worker_args;

// Worker function executed by each thread
void* worker(void* arg) {
    worker_args* args = (worker_args*)arg;
    long check = 0;
    
    // Calculate the required pool size for a tree of this depth
    size_t nodes = (1 << (args->depth + 1)) - 1;
    size_t pool_size = nodes * sizeof(TreeNode);

    for (int i = 0; i < args->iterations; ++i) {
        apr_pool_t* temp_pool = create_pool(pool_size);
        if (!temp_pool) {
            args->result = -1; // Indicate error
            return NULL;
        }
        TreeNode* tree = make_tree(args->depth, temp_pool);
        check += check_tree(tree);
        destroy_pool(temp_pool);
    }
    args->result = check;
    return NULL;
}

// Get the number of available CPU cores
int get_cpu_count() {
    return (int)sysconf(_SC_NPROCESSORS_ONLN);
}

int main(int argc, char* argv[]) {
    int n = (argc > 1) ? atoi(argv[1]) : 10;
    const int min_depth = 4;
    const int max_depth = (min_depth + 2 > n) ? min_depth + 2 : n;
    const int stretch_depth = max_depth + 1;

    // Create and check the stretch tree
    size_t stretch_nodes = (1 << (stretch_depth + 1)) - 1;
    apr_pool_t* stretch_pool = create_pool(stretch_nodes * sizeof(TreeNode));
    TreeNode* stretch_tree = make_tree(stretch_depth, stretch_pool);
    printf("stretch tree of depth %d\t check: %d\n", stretch_depth, check_tree(stretch_tree));
    destroy_pool(stretch_pool);

    // Create the long-lived tree
    size_t long_lived_nodes = (1 << (max_depth + 1)) - 1;
    apr_pool_t* long_lived_pool = create_pool(long_lived_nodes * sizeof(TreeNode));
    TreeNode* long_lived_tree = make_tree(max_depth, long_lived_pool);

    // Main loop for creating and checking trees of various depths
    int num_workers = get_cpu_count();
    pthread_t threads[num_workers];
    worker_args args[num_workers];

    for (int d = min_depth; d <= max_depth; d += 2) {
        int iterations = 1 << (max_depth - d + min_depth);
        long total_check = 0;

        // Distribute iterations among worker threads
        int base_iterations = iterations / num_workers;
        int remainder = iterations % num_workers;

        for (int i = 0; i < num_workers; ++i) {
            args[i].depth = d;
            args[i].iterations = base_iterations + (i < remainder ? 1 : 0);
            if (args[i].iterations > 0) {
                pthread_create(&threads[i], NULL, worker, &args[i]);
            }
        }

        for (int i = 0; i < num_workers; ++i) {
             if (args[i].iterations > 0) {
                pthread_join(threads[i], NULL);
                total_check += args[i].result;
            }
        }
        
        printf("%d\t trees of depth %d\t check: %ld\n", iterations, d, total_check);
    }

    // Check the long-lived tree at the end
    printf("long lived tree of depth %d\t check: %d\n", max_depth, check_tree(long_lived_tree));
    destroy_pool(long_lived_pool);

    return 0;
}

#include <stdio.h>
#include <stdlib.h>
#include <omp.h> // For OpenMP

// Represents a node in the binary tree
typedef struct TreeNode {
    struct TreeNode *left;
    struct TreeNode *right;
} TreeNode;

// A simple memory pool for fast, sequential node allocation
typedef struct {
    TreeNode* buffer;
    long index;
} MemoryPool;

// Allocates a memory pool large enough for a tree of a given depth
static MemoryPool create_pool(int depth) {
    // The number of nodes in a perfect binary tree of depth 'd' is 2^(d+1) - 1
    long num_nodes = (1L << (depth + 1)) - 1;
    MemoryPool pool;
    pool.buffer = (TreeNode*)malloc(num_nodes * sizeof(TreeNode));
    if (pool.buffer == NULL) {
        fprintf(stderr, "Failed to allocate memory for the pool.\n");
        exit(1);
    }
    pool.index = 0;
    return pool;
}

// Allocates a single node from the pool. Not thread-safe.
// For performance, we assume the pool has enough space.
static inline TreeNode* alloc_node(MemoryPool* pool) {
    return &pool->buffer[pool->index++];
}

// Frees the memory used by the pool
static void destroy_pool(MemoryPool* pool) {
    free(pool->buffer);
    pool->buffer = NULL;
}

// Recursively builds a binary tree of a given depth using a memory pool
static TreeNode* make_tree(int depth, MemoryPool* pool) {
    TreeNode* node = alloc_node(pool);
    if (depth > 0) {
        int d = depth - 1;
        node->left = make_tree(d, pool);
        node->right = make_tree(d, pool);
    } else {
        node->left = NULL;
        node->right = NULL;
    }
    return node;
}

// Recursively walks the tree and computes a checksum
static long check_tree(const TreeNode* node) {
    if (node->left == NULL) { // Leaf node
        return 1;
    }
    return 1 + check_tree(node->left) + check_tree(node->right);
}

int main(int argc, char* argv[]) {
    if (argc < 2) {
        fprintf(stderr, "Usage: %s <N>\n", argv[0]);
        return 1;
    }
    const int n = atoi(argv[1]);
    
    const int min_depth = 4;
    const int max_depth = (min_depth + 2 > n) ? min_depth + 2 : n;
    const int stretch_depth = max_depth + 1;

    // 1. Create and check the stretch tree
    {
        MemoryPool stretch_pool = create_pool(stretch_depth);
        TreeNode* stretch_tree = make_tree(stretch_depth, &stretch_pool);
        printf("stretch tree of depth %d\t check: %ld\n",
               stretch_depth, check_tree(stretch_tree));
        destroy_pool(&stretch_pool);
    }

    // 2. Create the long-lived tree
    MemoryPool long_lived_pool = create_pool(max_depth);
    TreeNode* long_lived_tree = make_tree(max_depth, &long_lived_pool);

    // 3. Create, check, and destroy many short-lived trees in parallel
    for (int d = min_depth; d <= max_depth; d += 2) {
        int iterations = 1 << (max_depth - d + min_depth);
        long total_check = 0;

        #pragma omp parallel for reduction(+:total_check)
        for (int i = 0; i < iterations; ++i) {
            // Each thread creates its own tree and memory pool to avoid conflicts
            MemoryPool temp_pool = create_pool(d);
            TreeNode* temp_tree = make_tree(d, &temp_pool);
            total_check += check_tree(temp_tree);
            destroy_pool(&temp_pool);
        }

        printf("%d\t trees of depth %d\t check: %ld\n",
               iterations, d, total_check);
    }

    // 4. Check the long-lived tree at the end
    printf("long lived tree of depth %d\t check: %ld\n",
           max_depth, check_tree(long_lived_tree));
    destroy_pool(&long_lived_pool);

    return 0;
}

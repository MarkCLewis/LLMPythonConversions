#include <stdio.h>
#include <stdlib.h>
#include <math.h>

// A node in the binary tree
typedef struct TreeNode {
    struct TreeNode *left;
    struct TreeNode *right;
} TreeNode;

// A simple memory pool for fast allocation
typedef struct {
    TreeNode* memory_area;
    TreeNode* next_free;
} MemoryPool;

// Creates a memory pool large enough for a tree of a given depth
MemoryPool create_pool(int depth) {
    // A full binary tree of depth 'd' has 2^(d+1) - 1 nodes
    long long num_nodes = (1LL << (depth + 1)) - 1;
    MemoryPool pool;
    pool.memory_area = (TreeNode*)malloc(num_nodes * sizeof(TreeNode));
    pool.next_free = pool.memory_area;
    return pool;
}

// Destroys a memory pool
void destroy_pool(MemoryPool* pool) {
    free(pool->memory_area);
    pool->memory_area = NULL;
    pool->next_free = NULL;
}

// Allocates a single node from the pool
TreeNode* alloc_node(MemoryPool* pool) {
    return pool->next_free++;
}

// Build a tree using nodes from the memory pool
TreeNode* make_tree(int depth, MemoryPool* pool) {
    TreeNode* node = alloc_node(pool);
    if (depth > 0) {
        node->left = make_tree(depth - 1, pool);
        node->right = make_tree(depth - 1, pool);
    } else {
        node->left = NULL;
        node->right = NULL;
    }
    return node;
}

// Recursively count the nodes in a tree
int check_tree(const TreeNode* node) {
    if (node->left == NULL) {
        return 1;
    }
    return 1 + check_tree(node->left) + check_tree(node->right);
}

// Helper function for max
int max(int a, int b) {
    return (a > b) ? a : b;
}

int main(int argc, char *argv[]) {
    if (argc < 2) {
        printf("Usage: %s <depth>\n", argv[0]);
        return 1;
    }
    
    int min_depth = 4;
    int max_depth = max(min_depth + 2, atoi(argv[1]));
    int stretch_depth = max_depth + 1;

    // 1. Stretch tree: create, check, and delete a tree of depth max_depth + 1
    {
        MemoryPool stretch_pool = create_pool(stretch_depth);
        TreeNode* stretch_tree = make_tree(stretch_depth, &stretch_pool);
        printf("stretch tree of depth %d\t check: %d\n", stretch_depth, check_tree(stretch_tree));
        destroy_pool(&stretch_pool);
    }

    // 2. Long-lived tree: create a tree that survives across iterations
    MemoryPool long_lived_pool = create_pool(max_depth);
    TreeNode* long_lived_tree = make_tree(max_depth, &long_lived_pool);

    // 3. Main loop: create, check, and delete many trees of varying depths
    long long iterations = 1LL << max_depth;
    for (int depth = min_depth; depth <= max_depth; depth += 2) {
        long long check = 0;
        MemoryPool temp_pool = create_pool(depth);
        
        for (long long i = 1; i <= iterations; i++) {
            // Reset the pool's free pointer to reuse the memory for each tree
            temp_pool.next_free = temp_pool.memory_area;
            TreeNode* temp_tree = make_tree(depth, &temp_pool);
            check += check_tree(temp_tree);
        }
        
        destroy_pool(&temp_pool);
        printf("%lld\t trees of depth %d\t check: %lld\n", iterations, depth, check);
        iterations /= 4;
    }

    // 4. Final check of the long-lived tree
    printf("long lived tree of depth %d\t check: %d\n", max_depth, check_tree(long_lived_tree));
    destroy_pool(&long_lived_pool);

    return 0;
}

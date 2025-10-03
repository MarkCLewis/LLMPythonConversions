#include <stdio.h>
#include <stdlib.h>
#include <math.h>

// A helper function to find the maximum of two integers.
int max(int a, int b) {
    return (a > b) ? a : b;
}

// Defines the structure for a single node in the binary tree.
// Each node has pointers to a left and a right child node.
typedef struct TreeNode {
    struct TreeNode* left;
    struct TreeNode* right;
} TreeNode;

// Recursively builds a binary tree of a given depth.
TreeNode* make_tree(int depth) {
    // Allocate memory for the new node.
    TreeNode* node = (TreeNode*)malloc(sizeof(TreeNode));
    if (!node) {
        fprintf(stderr, "Memory allocation failed\n");
        exit(1);
    }

    if (depth > 0) {
        // If not at the bottom level, create left and right sub-trees.
        int d = depth - 1;
        node->left = make_tree(d);
        node->right = make_tree(d);
    } else {
        // If at the leaf level, the children are NULL.
        node->left = NULL;
        node->right = NULL;
    }
    return node;
}

// Recursively frees the memory allocated for the entire tree.
// This is crucial to prevent memory leaks.
void free_tree(TreeNode* node) {
    if (node) {
        free_tree(node->left);
        free_tree(node->right);
        free(node);
    }
}

// Recursively walks the tree and counts the number of nodes.
int check_tree(const TreeNode* node) {
    if (node->left) {
        // It's an inner node: 1 (for itself) + nodes in its sub-trees.
        return 1 + check_tree(node->left) + check_tree(node->right);
    }
    // It's a leaf node.
    return 1;
}

int main(int argc, char *argv[]) {
    // Get the desired depth from the command-line arguments.
    int n = 0;
    if (argc > 1) {
        n = atoi(argv[1]);
    }

    const int min_depth = 4;
    int max_depth = max(min_depth + 2, n);
    int stretch_depth = max_depth + 1;

    // 1. Create a "stretch" tree, check it, and then delete it.
    TreeNode* stretch_tree = make_tree(stretch_depth);
    printf("stretch tree of depth %d\t check: %d\n",
           stretch_depth, check_tree(stretch_tree));
    free_tree(stretch_tree);

    // 2. Create a "long-lived" tree that will persist for a while.
    TreeNode* long_lived_tree = make_tree(max_depth);

    // 3. Create, check, and delete trees of varying depths.
    long long iterations = 1 << max_depth; // Efficient way to calculate 2^max_depth
    for (int depth = min_depth; depth <= max_depth; depth += 2) {
        long long check_sum = 0;
        for (long long i = 1; i <= iterations; i++) {
            TreeNode* temp_tree = make_tree(depth);
            check_sum += check_tree(temp_tree);
            free_tree(temp_tree);
        }
        printf("%lld\t trees of depth %d\t check: %lld\n",
               iterations, depth, check_sum);
        iterations /= 4; // Reduce iterations for the next, deeper level.
    }

    // 4. Finally, check the "long-lived" tree and delete it.
    printf("long lived tree of depth %d\t check: %d\n",
           max_depth, check_tree(long_lived_tree));
    free_tree(long_lived_tree);

    return 0;
}

// The Computer Language Benchmarks Game
// https://salsa.debian.org/benchmarksgame-team/benchmarksgame/
//
// Translated from Python version by ChatGPT

#include <stdio.h>
#include <stdlib.h>

typedef struct Node {
    struct Node* left;
    struct Node* right;
} Node;

// Create a binary tree of given depth
Node* make_tree(int depth) {
    if (depth == 0) {
        return NULL;
    }
    Node* node = (Node*)malloc(sizeof(Node));
    node->left = make_tree(depth - 1);
    node->right = make_tree(depth - 1);
    return node;
}

// Check tree by traversing nodes
int check_tree(Node* node) {
    if (node == NULL) {
        return 1;
    }
    return 1 + check_tree(node->left) + check_tree(node->right);
}

// Free memory recursively
void free_tree(Node* node) {
    if (node == NULL) return;
    free_tree(node->left);
    free_tree(node->right);
    free(node);
}

int main(int argc, char* argv[]) {
    if (argc < 2) {
        fprintf(stderr, "Usage: %s <max_depth>\n", argv[0]);
        return 1;
    }

    int min_depth = 4;
    int max_depth = atoi(argv[1]);
    if (max_depth < min_depth + 2) {
        max_depth = min_depth + 2;
    }
    int stretch_depth = max_depth + 1;

    Node* stretch_tree = make_tree(stretch_depth);
    printf("stretch tree of depth %d\t check: %d\n",
           stretch_depth, check_tree(stretch_tree));
    free_tree(stretch_tree);

    Node* long_lived_tree = make_tree(max_depth);

    int iterations = 1 << max_depth;
    for (int depth = min_depth; depth <= stretch_depth; depth += 2) {
        long check = 0;
        for (int i = 1; i <= iterations; i++) {
            Node* tree = make_tree(depth);
            check += check_tree(tree);
            free_tree(tree);
        }
        printf("%d\t trees of depth %d\t check: %ld\n", iterations, depth, check);
        iterations >>= 2;
    }

    printf("long lived tree of depth %d\t check: %d\n",
           max_depth, check_tree(long_lived_tree));
    free_tree(long_lived_tree);

    return 0;
}

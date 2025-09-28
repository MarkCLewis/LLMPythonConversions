// The Computer Language Benchmarks Game
// https://salsa.debian.org/benchmarksgame-team/benchmarksgame/
//
// Translated from Python version by ChatGPT
//
// Compile with:
//   gcc -O3 -fopenmp -march=native -o binarytrees binarytrees4.c
//
// Run with:
//   ./binarytrees 21

#include <stdio.h>
#include <stdlib.h>
#include <omp.h>

typedef struct Node {
    struct Node* left;
    struct Node* right;
} Node;

// Recursively allocate a binary tree of given depth
Node* make_tree(int depth) {
    if (depth <= 0) return NULL;
    Node* node = (Node*)malloc(sizeof(Node));
    node->left = make_tree(depth - 1);
    node->right = make_tree(depth - 1);
    return node;
}

// Recursively check tree
int check_tree(Node* node) {
    if (node == NULL) return 1;
    return 1 + check_tree(node->left) + check_tree(node->right);
}

// Free tree memory
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

    int n = atoi(argv[1]);
    int min_depth = 4;
    int max_depth = (n > min_depth + 2) ? n : (min_depth + 2);
    int stretch_depth = max_depth + 1;

    // Stretch tree
    Node* stretch_tree = make_tree(stretch_depth);
    printf("stretch tree of depth %d\t check: %d\n",
           stretch_depth, check_tree(stretch_tree));
    free_tree(stretch_tree);

    // Long-lived tree
    Node* long_lived_tree = make_tree(max_depth);

    int mmd = max_depth + min_depth;

    for (int depth = min_depth; depth <= stretch_depth - 1; depth += 2) {
        int iterations = 1 << (mmd - depth);
        long check = 0;

        #pragma omp parallel for reduction(+:check) schedule(dynamic)
        for (int i = 0; i < iterations; i++) {
            Node* tree = make_tree(depth);
            check += check_tree(tree);
            free_tree(tree);
        }

        printf("%d\t trees of depth %d\t check: %ld\n",
               iterations, depth, check);
    }

    printf("long lived tree of depth %d\t check: %d\n",
           max_depth, check_tree(long_lived_tree));
    free_tree(long_lived_tree);

    return 0;
}

#ifndef KD_TREE_H
#define KD_TREE_H

#include "particle.h"
#include "vector.h"

#define MAX_PARTS 7
#define THETA 0.3

// Represents a node in the k-d tree.
// A node is a leaf if num_parts > 0.
typedef struct KDTreeNode {
    // Shared properties
    double m;     // Total mass of particles in the node
    F64x3 cm;     // Center of mass
    double size;  // Size of the node (max dimension extent)

    // Leaf or Internal specific data
    int num_parts; // If > 0, this is a leaf node
    union {
        // For leaves
        struct {
            int particles[MAX_PARTS];
        } leaf;
        // For internal nodes
        struct {
            int split_dim;
            double split_val;
            int left;
            int right;
        } internal;
    } data;

} KDTreeNode;

// Represents the entire simulation system.
typedef struct {
    int* indices;
    KDTreeNode* nodes;
    int node_count;
    int node_capacity;
} System;

System* create_system(int n);
void destroy_system(System* sys);
void simple_sim(Particle* bodies, int num_bodies, int steps, double dt);

#endif // KD_TREE_H

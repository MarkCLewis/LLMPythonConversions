#include "kd_tree.h"
#include <stdio.h>
#include <stdlib.h>
#include <math.h>

// Forward declarations for internal functions
static int build_tree(System* sys, int start, int end, Particle* particles, int cur_node_idx);
static F64x3 accel_recur(int cur_node_idx, int p_idx, Particle* particles, System* sys);

System* create_system(int n) {
    System* sys = (System*)malloc(sizeof(System));
    if (!sys) return NULL;

    sys->indices = (int*)malloc(n * sizeof(int));
    if (!sys->indices) {
        free(sys);
        return NULL;
    }

    // Pre-allocate a generous number of nodes to avoid reallocations.
    sys->node_capacity = 2 * (n / (MAX_PARTS - 1) + 1) + 2;
    sys->nodes = (KDTreeNode*)malloc(sys->node_capacity * sizeof(KDTreeNode));
    if (!sys->nodes) {
        free(sys->indices);
        free(sys);
        return NULL;
    }
    sys->node_count = 0;
    return sys;
}

void destroy_system(System* sys) {
    if (sys) {
        free(sys->indices);
        free(sys->nodes);
        free(sys);
    }
}

// Partition helper for build_tree (quickselect-like)
void partition(int* indices, Particle* particles, int start, int end, int mid, int dim) {
    int s = start;
    int e = end;
    while (s < e) {
        int pivot = s + (e - s) / 2;
        double pivot_val = particles[indices[pivot]].p.x;
        if (dim == 1) pivot_val = particles[indices[pivot]].p.y;
        if (dim == 2) pivot_val = particles[indices[pivot]].p.z;

        // Hoare partitioning scheme
        int i = s - 1;
        int j = e + 1;
        while (1) {
            double val_i, val_j;
            do {
                i++;
                val_i = particles[indices[i]].p.x;
                if (dim == 1) val_i = particles[indices[i]].p.y;
                if (dim == 2) val_i = particles[indices[i]].p.z;
            } while (val_i < pivot_val);

            do {
                j--;
                val_j = particles[indices[j]].p.x;
                if (dim == 1) val_j = particles[indices[j]].p.y;
                if (dim == 2) val_j = particles[indices[j]].p.z;
            } while (val_j > pivot_val);
            
            if (i >= j) break;
            
            int temp = indices[i];
            indices[i] = indices[j];
            indices[j] = temp;
        }

        if (j < mid) s = j + 1;
        else e = j;
    }
}


static int build_tree(System* sys, int start, int end, Particle* particles, int cur_node_idx) {
    if (cur_node_idx >= sys->node_capacity) {
        // This should not happen with proper pre-allocation, but is here as a safeguard.
        fprintf(stderr, "Error: Ran out of nodes.\n");
        exit(1);
    }

    int np = end - start;
    KDTreeNode* node = &sys->nodes[cur_node_idx];
    int next_node_idx = cur_node_idx + 1;

    if (np <= MAX_PARTS) {
        node->num_parts = np;
        for (int i = 0; i < np; i++) {
            node->data.leaf.particles[i] = sys->indices[start + i];
        }
        return next_node_idx;
    }

    // Calculate bounding box and center of mass
    F64x3 min_p = {1e100, 1e100, 1e100};
    F64x3 max_p = {-1e100, -1e100, -1e100};
    double m = 0.0;
    F64x3 cm = {0, 0, 0};

    for (int i = start; i < end; i++) {
        int p_idx = sys->indices[i];
        m += particles[p_idx].m;
        cm = vec_add(cm, vec_mul_scalar(particles[p_idx].p, particles[p_idx].m));
        min_p = vec_min(min_p, particles[p_idx].p);
        max_p = vec_max(max_p, particles[p_idx].p);
    }
    cm = vec_div_scalar(cm, m);

    // *** BUG FIX ***
    // The original Python code had a bug here, only checking dim 1.
    // This C version correctly checks all 3 dimensions to find the largest extent.
    int split_dim = 0;
    F64x3 extent = vec_sub(max_p, min_p);
    if (extent.y > extent.x && extent.y > extent.z) {
        split_dim = 1;
    } else if (extent.z > extent.x && extent.z > extent.y) {
        split_dim = 2;
    }

    double size = extent.x;
    if (split_dim == 1) size = extent.y;
    if (split_dim == 2) size = extent.z;

    // Partition particles around the median
    int mid = start + (end - start) / 2;
    partition(sys->indices, particles, start, end - 1, mid, split_dim);
    double split_val = (split_dim == 0) ? particles[sys->indices[mid]].p.x :
                       (split_dim == 1) ? particles[sys->indices[mid]].p.y :
                                          particles[sys->indices[mid]].p.z;

    // Recurse on children and build this node
    int left_child_idx = next_node_idx;
    int right_child_idx_start = build_tree(sys, start, mid, particles, left_child_idx);
    int right_child_idx = right_child_idx_start;
    int next_available_idx = build_tree(sys, mid, end, particles, right_child_idx);

    node->num_parts = 0; // Mark as internal node
    node->m = m;
    node->cm = cm;
    node->size = size;
    node->data.internal.split_dim = split_dim;
    node->data.internal.split_val = split_val;
    node->data.internal.left = left_child_idx;
    node->data.internal.right = right_child_idx;
    
    return next_available_idx;
}

static F64x3 accel_recur(int cur_node_idx, int p_idx, Particle* particles, System* sys) {
    KDTreeNode* node = &sys->nodes[cur_node_idx];

    if (node->num_parts > 0) { // Leaf node
        F64x3 acc = {0, 0, 0};
        for (int i = 0; i < node->num_parts; i++) {
            int other_p_idx = node->data.leaf.particles[i];
            if (p_idx != other_p_idx) {
                acc = vec_add(acc, calc_pp_accel(&particles[p_idx], &particles[other_p_idx]));
            }
        }
        return acc;
    } else { // Internal node
        F64x3 dp = vec_sub(particles[p_idx].p, node->cm);
        double dist_sqr = vec_dot(dp, dp);

        if (node->size * node->size < THETA * THETA * dist_sqr) {
            // Far enough away, approximate
            double dist = sqrt(dist_sqr);
            double magi = -node->m / (dist_sqr * dist);
            return vec_mul_scalar(dp, magi);
        } else {
            // Too close, recurse
            F64x3 left_acc = accel_recur(node->data.internal.left, p_idx, particles, sys);
            F64x3 right_acc = accel_recur(node->data.internal.right, p_idx, particles, sys);
            return vec_add(left_acc, right_acc);
        }
    }
}

F64x3 calc_accel(int p_idx, Particle* particles, System* sys) {
    return accel_recur(0, p_idx, particles, sys);
}

void simple_sim(Particle* bodies, int num_bodies, int steps, double dt) {
    System* sys = create_system(num_bodies);
    F64x3* acc = (F64x3*)malloc(num_bodies * sizeof(F64x3));
    if (!acc) {
        destroy_system(sys);
        return;
    }

    for (int step = 0; step < steps; step++) {
        // Reset indices for the new tree
        for (int i = 0; i < num_bodies; i++) {
            sys->indices[i] = i;
        }

        sys->node_count = build_tree(sys, 0, num_bodies, bodies, 0);

        // Calculate accelerations for all bodies
        for (int i = 0; i < num_bodies; i++) {
            acc[i] = calc_accel(i, bodies, sys);
        }

        // Update velocities and positions
        for (int i = 0; i < num_bodies; i++) {
            bodies[i].v = vec_add(bodies[i].v, vec_mul_scalar(acc[i], dt));
            bodies[i].p = vec_add(bodies[i].p, vec_mul_scalar(bodies[i].v, dt));
        }
    }

    free(acc);
    destroy_system(sys);
}

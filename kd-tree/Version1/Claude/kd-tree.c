#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <math.h>
#include <string.h>
#include <time.h>

// Constants
#define MAX_PARTS 7
#define THETA 0.3

// 3D vector structure
typedef struct {
    double x, y, z;
} F64x3;

// Particle structure
typedef struct {
    double m;       // mass
    F64x3 p;        // position
    F64x3 v;        // velocity
} Particle;

// KD-tree node structure
typedef struct {
    // For leaves
    int num_parts;
    int* particles;  // array of particle indices

    // For internal nodes
    int split_dim;
    double split_val;
    double m;
    F64x3 cm;        // center of mass
    double size;
    int left;
    int right;
} KDTree;

// System structure
typedef struct {
    int* indices;
    KDTree* nodes;
    int nodes_count;
    int nodes_capacity;
} System;

// Vector operations
F64x3 f64x3_create(double x, double y, double z) {
    F64x3 vec = {x, y, z};
    return vec;
}

F64x3 f64x3_add(F64x3 a, F64x3 b) {
    F64x3 result = {a.x + b.x, a.y + b.y, a.z + b.z};
    return result;
}

F64x3 f64x3_sub(F64x3 a, F64x3 b) {
    F64x3 result = {a.x - b.x, a.y - b.y, a.z - b.z};
    return result;
}

F64x3 f64x3_mul(F64x3 a, double scalar) {
    F64x3 result = {a.x * scalar, a.y * scalar, a.z * scalar};
    return result;
}

F64x3 f64x3_div(F64x3 a, double scalar) {
    F64x3 result = {a.x / scalar, a.y / scalar, a.z / scalar};
    return result;
}

double f64x3_dot(F64x3 a, F64x3 b) {
    return a.x * b.x + a.y * b.y + a.z * b.z;
}

F64x3 f64x3_min(F64x3 a, F64x3 b) {
    F64x3 result = {
        fmin(a.x, b.x),
        fmin(a.y, b.y),
        fmin(a.z, b.z)
    };
    return result;
}

F64x3 f64x3_max(F64x3 a, F64x3 b) {
    F64x3 result = {
        fmax(a.x, b.x),
        fmax(a.y, b.y),
        fmax(a.z, b.z)
    };
    return result;
}

// Particle operations
F64x3 calc_pp_accel(Particle* a, Particle* b) {
    F64x3 dp = f64x3_sub(b->p, a->p);
    double dist_sqr = f64x3_dot(dp, dp);
    double dist = sqrt(dist_sqr);
    double magi = -b->m / (dist_sqr * dist);
    return f64x3_mul(dp, magi);
}

// Tree operations
KDTree create_leaf(int num_parts, int* particles, int particles_count) {
    KDTree node;
    node.num_parts = num_parts;
    
    node.particles = (int*)malloc(sizeof(int) * particles_count);
    if (particles != NULL && particles_count > 0) {
        memcpy(node.particles, particles, sizeof(int) * particles_count);
    }
    
    node.split_dim = 0;
    node.split_val = 0.0;
    node.m = 0.0;
    node.cm = f64x3_create(0.0, 0.0, 0.0);
    node.size = 0.0;
    node.left = -1;
    node.right = -1;
    
    return node;
}

void free_tree(KDTree* nodes, int node_count) {
    for (int i = 0; i < node_count; i++) {
        free(nodes[i].particles);
    }
    free(nodes);
}

System system_create(int n) {
    System sys;
    
    // Initialize indices
    sys.indices = (int*)malloc(sizeof(int) * n);
    for (int i = 0; i < n; i++) {
        sys.indices[i] = i;
    }
    
    // Initialize nodes
    int num_nodes = 2 * (n / (MAX_PARTS - 1) + 1);
    sys.nodes = (KDTree*)malloc(sizeof(KDTree) * num_nodes);
    for (int i = 0; i < num_nodes; i++) {
        sys.nodes[i] = create_leaf(0, NULL, 0);
    }
    
    sys.nodes_count = 0;
    sys.nodes_capacity = num_nodes;
    
    return sys;
}

void system_free(System* sys) {
    free(sys->indices);
    free_tree(sys->nodes, sys->nodes_capacity);
}

// Resize nodes array if needed
void ensure_node_capacity(System* sys, int needed_index) {
    if (needed_index >= sys->nodes_capacity) {
        int new_capacity = needed_index * 2;
        sys->nodes = (KDTree*)realloc(sys->nodes, sizeof(KDTree) * new_capacity);
        
        for (int i = sys->nodes_capacity; i < new_capacity; i++) {
            sys->nodes[i] = create_leaf(0, NULL, 0);
        }
        
        sys->nodes_capacity = new_capacity;
    }
}

// Shuffle elements in an array
void swap(int* a, int* b) {
    int temp = *a;
    *a = *b;
    *b = temp;
}

// Random number in range [min, max)
int randrange(int min, int max) {
    return min + rand() % (max - min);
}

// Build KD-tree recursively
int build_tree(System* sys, int start, int end, Particle* particles, int cur_node) {
    int np1 = end - start;
    
    if (np1 <= MAX_PARTS) {
        // Leaf node
        ensure_node_capacity(sys, cur_node);
        
        // Free previous particles array if it exists
        free(sys->nodes[cur_node].particles);
        
        sys->nodes[cur_node].num_parts = np1;
        sys->nodes[cur_node].particles = (int*)malloc(sizeof(int) * np1);
        
        for (int i = 0; i < np1; i++) {
            sys->nodes[cur_node].particles[i] = sys->indices[start + i];
        }
        
        return cur_node;
    } else {
        // Internal node
        // Pick split dimension and value
        F64x3 min_val = f64x3_create(1e100, 1e100, 1e100);
        F64x3 max_val = f64x3_create(-1e100, -1e100, -1e100);
        double m = 0.0;
        F64x3 cm = f64x3_create(0.0, 0.0, 0.0);
        
        for (int i = start; i < end; i++) {
            Particle* p = &particles[sys->indices[i]];
            m += p->m;
            
            cm.x += p->m * p->p.x;
            cm.y += p->m * p->p.y;
            cm.z += p->m * p->p.z;
            
            min_val = f64x3_min(min_val, p->p);
            max_val = f64x3_max(max_val, p->p);
        }
        
        cm = f64x3_div(cm, m);
        
        // Find dimension with largest spread
        int split_dim = 0;
        double dims[3] = {
            max_val.x - min_val.x,
            max_val.y - min_val.y, 
            max_val.z - min_val.z
        };
        
        for (int dim = 1; dim < 3; dim++) {
            if (dims[dim] > dims[split_dim]) {
                split_dim = dim;
            }
        }
        
        double size = dims[split_dim];
        
        // Partition particles on split_dim
        int mid = (start + end) / 2;
        int s = start;
        int e = end;
        
        while (s + 1 < e) {
            int pivot = randrange(s, e);
            swap(&sys->indices[s], &sys->indices[pivot]);
            
            int low = s + 1;
            int high = e - 1;
            
            double pivot_val;
            switch(split_dim) {
                case 0: pivot_val = particles[sys->indices[s]].p.x; break;
                case 1: pivot_val = particles[sys->indices[s]].p.y; break;
                case 2: pivot_val = particles[sys->indices[s]].p.z; break;
            }
            
            while (low <= high) {
                double low_val;
                switch(split_dim) {
                    case 0: low_val = particles[sys->indices[low]].p.x; break;
                    case 1: low_val = particles[sys->indices[low]].p.y; break;
                    case 2: low_val = particles[sys->indices[low]].p.z; break;
                }
                
                if (low_val < pivot_val) {
                    low++;
                } else {
                    swap(&sys->indices[low], &sys->indices[high]);
                    high--;
                }
            }
            
            swap(&sys->indices[s], &sys->indices[high]);
            
            if (high < mid) {
                s = high + 1;
            } else if (high > mid) {
                e = high;
            } else {
                s = e;
            }
        }
        
        double split_val;
        switch(split_dim) {
            case 0: split_val = particles[sys->indices[mid]].p.x; break;
            case 1: split_val = particles[sys->indices[mid]].p.y; break;
            case 2: split_val = particles[sys->indices[mid]].p.z; break;
        }
        
        // Recurse on children
        int left = build_tree(sys, start, mid, particles, cur_node + 1);
        int right = build_tree(sys, mid, end, particles, left + 1);
        
        ensure_node_capacity(sys, cur_node);
        
        // Free previous particles array if it exists
        free(sys->nodes[cur_node].particles);
        sys->nodes[cur_node].particles = NULL;
        
        sys->nodes[cur_node].num_parts = 0;
        sys->nodes[cur_node].split_dim = split_dim;
        sys->nodes[cur_node].split_val = split_val;
        sys->nodes[cur_node].m = m;
        sys->nodes[cur_node].cm = cm;
        sys->nodes[cur_node].size = size;
        sys->nodes[cur_node].left = cur_node + 1;
        sys->nodes[cur_node].right = left + 1;
        
        return right;
    }
}

// Calculate acceleration recursively through tree
F64x3 accel_recur(int cur_node, int p, Particle* particles, KDTree* nodes) {
    if (nodes[cur_node].num_parts > 0) {
        // Leaf node - direct calculation
        F64x3 acc = f64x3_create(0.0, 0.0, 0.0);
        
        for (int i = 0; i < nodes[cur_node].num_parts; i++) {
            if (nodes[cur_node].particles[i] != p) {
                F64x3 part_acc = calc_pp_accel(&particles[p], &particles[nodes[cur_node].particles[i]]);
                acc = f64x3_add(acc, part_acc);
            }
        }
        
        return acc;
    } else {
        // Internal node
        F64x3 dp = f64x3_sub(particles[p].p, nodes[cur_node].cm);
        double dist_sqr = f64x3_dot(dp, dp);
        
        if (nodes[cur_node].size * nodes[cur_node].size < THETA * THETA * dist_sqr) {
            // Far enough to use approximation
            double dist = sqrt(dist_sqr);
            double magi = -nodes[cur_node].m / (dist_sqr * dist);
            return f64x3_mul(dp, magi);
        } else {
            // Too close, need to recurse
            F64x3 left_acc = accel_recur(nodes[cur_node].left, p, particles, nodes);
            F64x3 right_acc = accel_recur(nodes[cur_node].right, p, particles, nodes);
            return f64x3_add(left_acc, right_acc);
        }
    }
}

// Calculate acceleration for a particle
F64x3 calc_accel(int p, Particle* particles, KDTree* nodes) {
    return accel_recur(0, p, particles, nodes);
}

// Run simulation for given number of steps
void simple_sim(Particle* bodies, int body_count, double dt, int steps, bool print_steps) {
    System sys = system_create(body_count);
    
    for (int step = 0; step < steps; step++) {
        if (print_steps) {
            printf("Step %d\n", step);
        }
        
        // Reset indices
        for (int i = 0; i < body_count; i++) {
            sys.indices[i] = i;
        }
        
        // Build tree
        build_tree(&sys, 0, body_count, bodies, 0);
        
        // Calculate accelerations
        F64x3* acc = (F64x3*)malloc(sizeof(F64x3) * body_count);
        for (int i = 0; i < body_count; i++) {
            acc[i] = calc_accel(i, bodies, sys.nodes);
        }
        
        // Update positions and velocities
        for (int i = 0; i < body_count; i++) {
            bodies[i].v = f64x3_add(bodies[i].v, f64x3_mul(acc[i], dt));
            bodies[i].p = f64x3_add(bodies[i].p, f64x3_mul(bodies[i].v, dt));
        }
        
        free(acc);
    }
    
    system_free(&sys);
}

// Print tree structure to file
void print_tree(int step, KDTree* tree, int tree_size, Particle* particles) {
    char filename[32];
    sprintf(filename, "tree%d.txt", step);
    
    FILE* file = fopen(filename, "w");
    if (!file) return;
    
    // Count particles by looking at the first leaf node
    int particle_count = 0;
    for (int i = 0; i < tree_size; i++) {
        if (tree[i].num_parts > 0) {
            particle_count += tree[i].num_parts;
        }
    }
    
    fprintf(file, "%d\n", particle_count);
    
    for (int i = 0; i < tree_size; i++) {
        if (tree[i].num_parts > 0) {
            // Leaf node
            fprintf(file, "L %d\n", tree[i].num_parts);
            
            for (int j = 0; j < tree[i].num_parts; j++) {
                int p = tree[i].particles[j];
                fprintf(file, "%f %f %f\n", particles[p].p.x, particles[p].p.y, particles[p].p.z);
            }
        } else if (tree[i].left >= 0 && tree[i].right >= 0) {
            // Internal node
            fprintf(file, "I %d %f %d %d\n", tree[i].split_dim, tree[i].split_val, tree[i].left, tree[i].right);
        }
    }
    
    fclose(file);
}

// Test tree structure
void recur_test_tree_struct(int node, KDTree* nodes, Particle* particles, F64x3 min_bounds, F64x3 max_bounds) {
    if (nodes[node].num_parts > 0) {
        // Leaf node - check all particles
        for (int idx = 0; idx < nodes[node].num_parts; idx++) {
            int i = nodes[node].particles[idx];
            
            for (int dim = 0; dim < 3; dim++) {
                double p_val;
                double min_val, max_val;
                
                switch(dim) {
                    case 0:
                        p_val = particles[i].p.x;
                        min_val = min_bounds.x;
                        max_val = max_bounds.x;
                        break;
                    case 1:
                        p_val = particles[i].p.y;
                        min_val = min_bounds.y;
                        max_val = max_bounds.y;
                        break;
                    case 2:
                        p_val = particles[i].p.z;
                        min_val = min_bounds.z;
                        max_val = max_bounds.z;
                        break;
                }
                
                if (p_val < min_val) {
                    fprintf(stderr, "Particle dim %d is below min. i=%d p=%f min=%f\n", 
                            dim, i, p_val, min_val);
                }
                
                if (p_val >= max_val) {
                    fprintf(stderr, "Particle dim %d is above max. i=%d p=%f max=%f\n", 
                            dim, i, p_val, max_val);
                }
            }
        }
    } else {
        // Internal node - recurse on children
        int split_dim = nodes[node].split_dim;
        double tmin, tmax;
        
        switch(split_dim) {
            case 0:
                tmin = min_bounds.x;
                tmax = max_bounds.x;
                
                max_bounds.x = nodes[node].split_val;
                recur_test_tree_struct(nodes[node].left, nodes, particles, min_bounds, max_bounds);
                
                max_bounds.x = tmax;
                min_bounds.x = nodes[node].split_val;
                recur_test_tree_struct(nodes[node].right, nodes, particles, min_bounds, max_bounds);
                
                min_bounds.x = tmin;
                break;
                
            case 1:
                tmin = min_bounds.y;
                tmax = max_bounds.y;
                
                max_bounds.y = nodes[node].split_val;
                recur_test_tree_struct(nodes[node].left, nodes, particles, min_bounds, max_bounds);
                
                max_bounds.y = tmax;
                min_bounds.y = nodes[node].split_val;
                recur_test_tree_struct(nodes[node].right, nodes, particles, min_bounds, max_bounds);
                
                min_bounds.y = tmin;
                break;
                
            case 2:
                tmin = min_bounds.z;
                tmax = max_bounds.z;
                
                max_bounds.z = nodes[node].split_val;
                recur_test_tree_struct(nodes[node].left, nodes, particles, min_bounds, max_bounds);
                
                max_bounds.z = tmax;
                min_bounds.z = nodes[node].split_val;
                recur_test_tree_struct(nodes[node].right, nodes, particles, min_bounds, max_bounds);
                
                min_bounds.z = tmin;
                break;
        }
    }
}

// Example usage
int main() {
    srand(time(NULL));
    
    // Create some test particles
    const int NUM_PARTICLES = 1000;
    Particle* particles = (Particle*)malloc(sizeof(Particle) * NUM_PARTICLES);
    
    // Initialize particles (random positions)
    for (int i = 0; i < NUM_PARTICLES; i++) {
        particles[i].m = 1.0;
        particles[i].p.x = (double)rand() / RAND_MAX * 100.0 - 50.0;
        particles[i].p.y = (double)rand() / RAND_MAX * 100.0 - 50.0;
        particles[i].p.z = (double)rand() / RAND_MAX * 100.0 - 50.0;
        particles[i].v.x = (double)rand() / RAND_MAX * 2.0 - 1.0;
        particles[i].v.y = (double)rand() / RAND_MAX * 2.0 - 1.0;
        particles[i].v.z = (double)rand() / RAND_MAX * 2.0 - 1.0;
    }
    
    // Run simulation
    double dt = 0.1;
    int steps = 100;
    clock_t start = clock();
    simple_sim(particles, NUM_PARTICLES, dt, steps, true);
    clock_t end = clock();
    
    double time_spent = (double)(end - start) / CLOCKS_PER_SEC;
    printf("Simulation completed in %f seconds\n", time_spent);
    
    // Clean up
    free(particles);
    
    return 0;
}
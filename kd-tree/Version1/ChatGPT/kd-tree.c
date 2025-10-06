// kd_tree.c  —  C port of kd_tree.py  (Barnes–Hut-style k-d tree)
// Converted from: kd_tree.py  (θ = 0.3, MAX_PARTS = 7)  — see citation in chat.
//
// Build (single-threaded):
//   cc -O3 -march=native -flto kd_tree.c -o kd_tree -lm
//
// Build (OpenMP parallel accel loop):
//   cc -O3 -march=native -flto -fopenmp kd_tree.c -o kd_tree -lm
//
// Run demo:
//   ./kd_tree  (generates a small random system, runs a few steps)

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <float.h>
#include <time.h>

#ifdef _OPENMP
  #include <omp.h>
#endif

// ----- Constants (match the Python) -----
#ifndef MAX_PARTS
#define MAX_PARTS 7
#endif

#ifndef THETA
#define THETA 0.3
#endif

// Optional softening to avoid singularities on exact overlap in point–point accel.
// (The Python pairwise function wasn't provided; this keeps things robust.)
#ifndef SOFTENING2
#define SOFTENING2 0.0
#endif

// ===== Math / types ==========================================================
typedef struct { double x, y, z; } Vec3;

static inline Vec3 v3(double x, double y, double z) { Vec3 r = {x,y,z}; return r; }
static inline Vec3 v3_add(Vec3 a, Vec3 b){ return v3(a.x+b.x, a.y+b.y, a.z+b.z); }
static inline Vec3 v3_sub(Vec3 a, Vec3 b){ return v3(a.x-b.x, a.y-b.y, a.z-b.z); }
static inline Vec3 v3_scale(Vec3 a, double s){ return v3(a.x*s, a.y*s, a.z*s); }
static inline double v3_dot(Vec3 a, Vec3 b){ return a.x*b.x + a.y*b.y + a.z*b.z; }

// ===== Particle ==============================================================
typedef struct {
    Vec3 p;   // position
    Vec3 v;   // velocity
    double m; // mass
} Particle;

// Pairwise acceleration on 'a' due to 'b':  a <- a - (m_b * (a - b) / |a-b|^3)
static inline Vec3 calc_pp_accel(const Particle* a, const Particle* b) {
    Vec3 dp = v3_sub(a->p, b->p);
    double r2 = v3_dot(dp, dp) + SOFTENING2;
    double r = sqrt(r2);
    if (r == 0.0) return v3(0,0,0);
    double mag = -b->m / (r2 * r);
    return v3_scale(dp, mag);
}

// ===== KD tree node =========================================================
typedef struct {
    // Leaf
    int   num_parts;   // >0 => leaf; ==0 => internal
    int*  particles;   // length = num_parts (indices into particle array)

    // Internal
    int   split_dim;   // 0/1/2
    double split_val;
    double m;          // total mass in node
    Vec3  cm;          // center of mass
    double size;       // extent along split_dim (max - min)
    int   left;        // index of left child node in nodes[]
    int   right;       // index of right child node in nodes[]
} KDNode;

typedef struct {
    int     n;           // number of bodies
    int*    indices;     // permutation of [0..n)
    KDNode* nodes;       // contiguous node storage
    int     nodes_cap;   // capacity
} System;

// ---- Node helpers ----
static void node_clear(KDNode* nd){
    if (nd->particles){ free(nd->particles); }
    memset(nd, 0, sizeof(KDNode));
}
static KDNode node_make_leaf(int count, const int* idx_src){
    KDNode nd; memset(&nd, 0, sizeof(nd));
    nd.num_parts = count;
    nd.particles = (int*)malloc(sizeof(int)*count);
    for(int i=0;i<count;i++) nd.particles[i] = idx_src[i];
    return nd;
}
static void ensure_nodes_capacity(System* sys, int need_index){
    if (need_index < sys->nodes_cap) return;
    int newcap = sys->nodes_cap ? sys->nodes_cap : 2;
    while (newcap <= need_index) newcap = newcap*2 + 16;
    sys->nodes = (KDNode*)realloc(sys->nodes, sizeof(KDNode)*newcap);
    for (int i = sys->nodes_cap; i < newcap; ++i){
        sys->nodes[i].particles = NULL;
        sys->nodes[i].num_parts = 0;
        sys->nodes[i].split_dim = 0;
        sys->nodes[i].split_val = 0.0;
        sys->nodes[i].m = 0.0;
        sys->nodes[i].cm = v3(0,0,0);
        sys->nodes[i].size = 0.0;
        sys->nodes[i].left = sys->nodes[i].right = 0;
    }
    sys->nodes_cap = newcap;
}

// ---- System init/free -------------------------------------------------------
static void system_init(System* sys, int n){
    sys->n = n;
    sys->indices = (int*)malloc(sizeof(int)*n);
    for (int i=0;i<n;i++) sys->indices[i] = i;
    // initial guess for nodes, from Python: 2 * (n // (MAX_PARTS-1) + 1)
    int guess = 2 * (n / (MAX_PARTS-1) + 1);
    sys->nodes = (KDNode*)calloc(guess, sizeof(KDNode));
    sys->nodes_cap = guess;
}
static void system_clear_nodes(System* sys){
    for (int i=0;i<sys->nodes_cap;i++){
        if (sys->nodes[i].num_parts > 0 && sys->nodes[i].particles){
            free(sys->nodes[i].particles);
            sys->nodes[i].particles = NULL;
        }
        sys->nodes[i].num_parts = 0;
        // zeroing other fields not strictly required each step
    }
}
static void system_free(System* sys){
    if (sys->nodes){
        for (int i=0;i<sys->nodes_cap;i++){
            if (sys->nodes[i].particles) free(sys->nodes[i].particles);
        }
        free(sys->nodes);
    }
    free(sys->indices);
    memset(sys, 0, sizeof(*sys));
}

// ---- Partition/quickselect (as in the Python loop) --------------------------
static inline double coord(const Particle* p, int dim){
    return (dim==0)?p->p.x : (dim==1)?p->p.y : p->p.z;
}

// Partition-based nth element to put the k-th element (start+k) in place by split_dim
static void nth_element_indices(int* idx, int start, int end, int k, const Particle* parts, int split_dim){
    // We reproduce the randomized loop style used in the Python code.
    // Invariant: we keep shrinking [s,e) until pivot lands at mid (start+k).
    int s = start, e = end;
    int mid = start + k;
    while (s + 1 < e){
        int pivot = s + rand() % (e - s);

        // swap idx[s], idx[pivot]
        int tmp = idx[s]; idx[s] = idx[pivot]; idx[pivot] = tmp;

        int low = s + 1;
        int high = e - 1;
        while (low <= high){
            if (coord(&parts[idx[low]], split_dim) < coord(&parts[idx[s]], split_dim)){
                low++;
            } else {
                int t = idx[low]; idx[low] = idx[high]; idx[high] = t;
                high--;
            }
        }
        // place pivot to its final position at 'high'
        int t = idx[s]; idx[s] = idx[high]; idx[high] = t;

        if (high < mid){
            s = high + 1;
        } else if (high > mid){
            e = high;
        } else {
            // done
            return;
        }
    }
    // trivial tail: if only one element remains, it's already correct
}

// ---- Build tree -------------------------------------------------------------
static int build_tree(System* sys, int start, int end, const Particle* parts, int cur_node){
    int np1 = end - start;
    if (np1 <= MAX_PARTS){
        ensure_nodes_capacity(sys, cur_node);
        // clear any previous leaf storage before overwriting
        if (sys->nodes[cur_node].num_parts > 0 && sys->nodes[cur_node].particles){
            free(sys->nodes[cur_node].particles);
            sys->nodes[cur_node].particles = NULL;
        }
        sys->nodes[cur_node] = node_make_leaf(np1, &sys->indices[start]);
        return cur_node;
    } else {
        // bounding box + mass/CM
        Vec3 minv = v3( 1e100,  1e100,  1e100);
        Vec3 maxv = v3(-1e100, -1e100, -1e100);
        double m = 0.0;
        Vec3 cm = v3(0,0,0);

        for (int i=start;i<end;i++){
            const Particle* p = &parts[ sys->indices[i] ];
            m += p->m;
            cm = v3_add(cm, v3_scale(p->p, p->m));
            if (p->p.x < minv.x) minv.x = p->p.x;
            if (p->p.y < minv.y) minv.y = p->p.y;
            if (p->p.z < minv.z) minv.z = p->p.z;
            if (p->p.x > maxv.x) maxv.x = p->p.x;
            if (p->p.y > maxv.y) maxv.y = p->p.y;
            if (p->p.z > maxv.z) maxv.z = p->p.z;
        }
        cm = v3_scale(cm, 1.0 / m);

        // choose split dim with largest span (the Python had a FIXME; we do the natural 0..2)
        double span[3] = { maxv.x - minv.x, maxv.y - minv.y, maxv.z - minv.z };
        int split_dim = 0;
        if (span[1] > span[split_dim]) split_dim = 1;
        if (span[2] > span[split_dim]) split_dim = 2;
        double size = span[split_dim];

        // partition around median on split_dim
        int mid = (start + end) / 2;
        nth_element_indices(sys->indices, start, end, mid - start, parts, split_dim);
        double split_val = coord(&parts[ sys->indices[mid] ], split_dim);

        // recurse
        int left_root = cur_node + 1;
        int left_last = build_tree(sys, start, mid, parts, left_root);
        int right_root = left_last + 1;
        int right_last = build_tree(sys, mid, end, parts, right_root);

        ensure_nodes_capacity(sys, cur_node);
        // clear any previous leaf storage
        if (sys->nodes[cur_node].num_parts > 0 && sys->nodes[cur_node].particles){
            free(sys->nodes[cur_node].particles);
            sys->nodes[cur_node].particles = NULL;
        }
        sys->nodes[cur_node].num_parts = 0; // mark internal
        sys->nodes[cur_node].split_dim = split_dim;
        sys->nodes[cur_node].split_val = split_val;
        sys->nodes[cur_node].m = m;
        sys->nodes[cur_node].cm = cm;
        sys->nodes[cur_node].size = size;
        sys->nodes[cur_node].left = left_root;
        sys->nodes[cur_node].right = right_root;
        return right_last;
    }
}

// ---- Acceleration recursion -------------------------------------------------
static Vec3 accel_recur(int cur_node, int p_index, const Particle* parts, const KDNode* nodes){
    const KDNode* node = &nodes[cur_node];
    if (node->num_parts > 0){
        Vec3 acc = v3(0,0,0);
        for (int i=0;i<node->num_parts;i++){
            int q = node->particles[i];
            if (q != p_index){
                Vec3 a = calc_pp_accel(&parts[p_index], &parts[q]);
                acc = v3_add(acc, a);
            }
        }
        return acc;
    } else {
        Vec3 dp = v3_sub(parts[p_index].p, node->cm);
        double dist2 = v3_dot(dp, dp);
        // Barnes–Hut criterion: size^2 < θ^2 * r^2
        if (node->size * node->size < (THETA * THETA) * dist2){
            double r = sqrt(dist2);
            if (r == 0.0) return v3(0,0,0);
            double mag = -node->m / (dist2 * r);
            return v3_scale(dp, mag);
        } else {
            Vec3 aL = accel_recur(node->left,  p_index, parts, nodes);
            Vec3 aR = accel_recur(node->right, p_index, parts, nodes);
            return v3_add(aL, aR);
        }
    }
}
static inline Vec3 calc_accel(int p, const Particle* parts, const KDNode* nodes){
    return accel_recur(0, p, parts, nodes);
}

// ---- Utility: print tree (compatible with the Python format) ---------------
static void print_tree_file(int step, const KDNode* nodes, int nodes_cap, const Particle* parts, int n){
    char fname[64]; snprintf(fname, sizeof(fname), "tree%d.txt", step);
    FILE* f = fopen(fname, "w");
    if (!f) return;
    fprintf(f, "%d\n", n);
    for (int i=0;i<nodes_cap;i++){
        const KDNode* nd = &nodes[i];
        if (nd->num_parts > 0){
            fprintf(f, "L %d\n", nd->num_parts);
            for (int j=0;j<nd->num_parts;j++){
                int p = nd->particles[j];
                fprintf(f, "%.17g %.17g %.17g\n", parts[p].p.x, parts[p].p.y, parts[p].p.z);
            }
        } else {
            fprintf(f, "I %d %.17g %d %d\n", nd->split_dim, nd->split_val, nd->left, nd->right);
        }
    }
    fclose(f);
}

// ---- Simple integrator (like simple_sim in Python) -------------------------
static void simple_sim(Particle* bodies, int n, double dt, int steps, int print_steps){
    System sys; system_init(&sys, n);
    Vec3* acc = (Vec3*)malloc(sizeof(Vec3)*n);

    for (int step=0; step<steps; ++step){
        if (print_steps) printf("%d\n", step);

        for (int i=0;i<n;i++) sys.indices[i] = i;
        system_clear_nodes(&sys);
        build_tree(&sys, 0, n, bodies, 0);

        // per-body acceleration (parallelizable)
        #pragma omp parallel for schedule(static)
        for (int i=0;i<n;i++){
            acc[i] = calc_accel(i, bodies, sys.nodes);
        }

        // integrate (kick-drift Euler, matching Python’s simple update)
        for (int i=0;i<n;i++){
            bodies[i].v = v3_add(bodies[i].v, v3_scale(acc[i], dt));
            bodies[i].p = v3_add(bodies[i].p, v3_scale(bodies[i].v, dt));
        }

        // Optionally dump the tree for debugging/visualization:
        // if (step % 10 == 0) print_tree_file(step, sys.nodes, sys.nodes_cap, bodies, n);
    }

    free(acc);
    system_free(&sys);
}

// ===== Demo main =============================================================
#ifdef DEMO_MAIN
int main(void){
    srand((unsigned)time(NULL));

    int n = 20000;            // adjust as desired
    double dt = 1e-3;
    int steps = 5;

    Particle* bodies = (Particle*)malloc(sizeof(Particle)*n);
    // random ring-like cloud
    for (int i=0;i<n;i++){
        double r = 50.0 + 10.0 * ((double)rand()/RAND_MAX - 0.5);
        double ang = 2.0*M_PI*((double)rand()/RAND_MAX);
        double z = 2.0 * ((double)rand()/RAND_MAX - 0.5);
        bodies[i].p = v3(r*cos(ang), r*sin(ang), z);
        bodies[i].v = v3(0.0, 0.0, 0.0);
        bodies[i].m = 1.0/n;
    }

    simple_sim(bodies, n, dt, steps, 1);

    // print final COM just to produce a tiny output
    Vec3 cm = v3(0,0,0); double m=0.0;
    for (int i=0;i<n;i++){ cm = v3_add(cm, v3_scale(bodies[i].p, bodies[i].m)); m += bodies[i].m; }
    cm = v3_scale(cm, 1.0/m);
    printf("Final CM ~ (%.6f, %.6f, %.6f)\n", cm.x, cm.y, cm.z);

    free(bodies);
    return 0;
}
#else
// Provide a no-op main for easy compile if DEMO_MAIN not set.
int main(void){
    fprintf(stderr, "Built kd_tree library. Define DEMO_MAIN to run a demo.\n");
    return 0;
}
#endif

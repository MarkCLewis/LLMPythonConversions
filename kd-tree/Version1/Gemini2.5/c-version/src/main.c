#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include "particle.h"
#include "kd_tree.h"

int main(int argc, char* argv[]) {
    if (argc != 3) {
        fprintf(stderr, "Usage: %s <num_particles> <num_steps>\n", argv[0]);
        return 1;
    }

    int n = atoi(argv[1]);
    int steps = atoi(argv[2]);
    int num_bodies = n + 1; // n planets + 1 star
    double dt = 1e-3;

    // Seed the random number generator
    srand(time(NULL));

    Particle* bodies = circular_orbits(n);
    if (!bodies) {
        fprintf(stderr, "Failed to allocate particles.\n");
        return 1;
    }

    clock_t start = clock();
    simple_sim(bodies, num_bodies, steps, dt);
    clock_t end = clock();

    double time_spent = (double)(end - start) / CLOCKS_PER_SEC;
    printf("%f\n", time_spent);

    free(bodies);
    return 0;
}

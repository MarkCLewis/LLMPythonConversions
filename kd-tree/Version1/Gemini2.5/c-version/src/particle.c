#include "particle.h"
#include <stdlib.h>
#include <math.h>

#define PI 3.141592653589793

// Helper to get a random double between 0.0 and 1.0
double rand_double() {
    return (double)rand() / (double)RAND_MAX;
}

Particle* circular_orbits(int n) {
    Particle* particles = (Particle*)malloc((n + 1) * sizeof(Particle));
    if (!particles) return NULL;

    // Central star
    particles[0] = (Particle){{0, 0, 0}, {0, 0, 0}, 0.00465047, 1.0};

    for (int i = 0; i < n; i++) {
        double d = 0.1 + (i * 5.0 / n);
        double v = sqrt(1.0 / d);
        double theta = rand_double() * 2.0 * PI;

        double x = d * cos(theta);
        double y = d * sin(theta);
        double vx = -v * sin(theta);
        double vy = v * cos(theta);

        particles[i + 1] = (Particle){{x, y, 0}, {vx, vy, 0}, 1e-14, 1e-7};
    }
    return particles;
}

F64x3 calc_pp_accel(const Particle* pi, const Particle* pj) {
    F64x3 dp = vec_sub(pi->p, pj->p);
    double dist_sqr = vec_dot(dp, dp);
    double dist = sqrt(dist_sqr);
    double magi = -pj->m / (dist_sqr * dist);
    return vec_mul_scalar(dp, magi);
}

// nbody.c — C port of the Python nbody benchmark
// Build:  gcc -O3 -march=native -std=c11 -o nbody nbody.c -lm
// Run:    ./nbody 50000000

#include <stdio.h>
#include <stdlib.h>
#include <math.h>

#define PI 3.14159265358979323
#define SOLAR_MASS (4.0 * PI * PI)
#define DAYS_PER_YEAR 365.24

typedef struct {
    double r[3];   // position
    double v[3];   // velocity
    double m;      // mass
} Body;

// Sun, Jupiter, Saturn, Uranus, Neptune (same values as Python)
static Body bodies[] = {
    {{ 0.0, 0.0, 0.0 },
     { 0.0, 0.0, 0.0 },
     SOLAR_MASS},

    {{ 4.84143144246472090e+00, -1.16032004402742839e+00, -1.03622044471123109e-01 },
     { 1.66007664274403694e-03 * DAYS_PER_YEAR,
       7.69901118419740425e-03 * DAYS_PER_YEAR,
      -6.90460016972063023e-05 * DAYS_PER_YEAR },
     9.54791938424326609e-04 * SOLAR_MASS},

    {{ 8.34336671824457987e+00,  4.12479856412430479e+00, -4.03523417114321381e-01 },
     {-2.76742510726862411e-03 * DAYS_PER_YEAR,
       4.99852801234917238e-03 * DAYS_PER_YEAR,
       2.30417297573763929e-05 * DAYS_PER_YEAR },
     2.85885980666130812e-04 * SOLAR_MASS},

    {{ 1.28943695621391310e+01, -1.51111514016986312e+01, -2.23307578892655734e-01 },
     { 2.96460137564761618e-03 * DAYS_PER_YEAR,
       2.37847173959480950e-03 * DAYS_PER_YEAR,
      -2.96589568540237556e-05 * DAYS_PER_YEAR },
     4.36624404335156298e-05 * SOLAR_MASS},

    {{ 1.53796971148509165e+01, -2.59193146099879641e+01,  1.79258772950371181e-01 },
     { 2.68067772490389322e-03 * DAYS_PER_YEAR,
       1.62824170038242295e-03 * DAYS_PER_YEAR,
      -9.51592254519715870e-05 * DAYS_PER_YEAR },
     5.15138902046611451e-05 * SOLAR_MASS}
};
static const int N = sizeof(bodies) / sizeof(bodies[0]);

static void offset_momentum(int ref) {
    double px = 0.0, py = 0.0, pz = 0.0;
    for (int i = 0; i < N; ++i) {
        px -= bodies[i].v[0] * bodies[i].m;
        py -= bodies[i].v[1] * bodies[i].m;
        pz -= bodies[i].v[2] * bodies[i].m;
    }
    bodies[ref].v[0] = px / bodies[ref].m;
    bodies[ref].v[1] = py / bodies[ref].m;
    bodies[ref].v[2] = pz / bodies[ref].m;
}

static void advance(double dt, int steps) {
    for (int s = 0; s < steps; ++s) {
        // pairwise gravity updates
        for (int i = 0; i < N; ++i) {
            for (int j = i + 1; j < N; ++j) {
                double dx = bodies[i].r[0] - bodies[j].r[0];
                double dy = bodies[i].r[1] - bodies[j].r[1];
                double dz = bodies[i].r[2] - bodies[j].r[2];
                double d2 = dx*dx + dy*dy + dz*dz;
                double inv_d = 1.0 / sqrt(d2);
                double inv_d3 = inv_d * inv_d * inv_d;
                double mag = dt * inv_d3;

                double b1m = bodies[i].m * mag;
                double b2m = bodies[j].m * mag;

                bodies[i].v[0] -= dx * b2m;
                bodies[i].v[1] -= dy * b2m;
                bodies[i].v[2] -= dz * b2m;

                bodies[j].v[0] += dx * b1m;
                bodies[j].v[1] += dy * b1m;
                bodies[j].v[2] += dz * b1m;
            }
        }
        // integrate positions
        for (int i = 0; i < N; ++i) {
            bodies[i].r[0] += dt * bodies[i].v[0];
            bodies[i].r[1] += dt * bodies[i].v[1];
            bodies[i].r[2] += dt * bodies[i].v[2];
        }
    }
}

static void report_energy(void) {
    double e = 0.0;
    for (int i = 0; i < N; ++i) {
        // kinetic
        double vx = bodies[i].v[0], vy = bodies[i].v[1], vz = bodies[i].v[2];
        e += 0.5 * bodies[i].m * (vx*vx + vy*vy + vz*vz);
        // potential with j>i
        for (int j = i + 1; j < N; ++j) {
            double dx = bodies[i].r[0] - bodies[j].r[0];
            double dy = bodies[i].r[1] - bodies[j].r[1];
            double dz = bodies[i].r[2] - bodies[j].r[2];
            double dist = sqrt(dx*dx + dy*dy + dz*dz);
            e -= (bodies[i].m * bodies[j].m) / dist;
        }
    }
    printf("%.9f\n", e);
}

int main(int argc, char **argv) {
    int n = (argc > 1) ? atoi(argv[1]) : 1000;

    // set the reference body (sun) so total momentum is zero
    offset_momentum(0);

    report_energy();
    advance(0.01, n);
    report_energy();

    return 0;
}

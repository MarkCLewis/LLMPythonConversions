// nbody.c - Converted from Python to C for performance
#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <string.h>

#define PI 3.14159265358979323
#define SOLAR_MASS (4 * PI * PI)
#define DAYS_PER_YEAR 365.24

typedef struct {
    double position[3];
    double velocity[3];
    double mass;
} Body;

Body bodies[] = {
    { { 0.0, 0.0, 0.0 }, { 0.0, 0.0, 0.0 }, SOLAR_MASS },  // Sun
    { { 4.84143144246472090e+00, -1.16032004402742839e+00, -1.03622044471123109e-01 },
      { 1.66007664274403694e-03 * DAYS_PER_YEAR,
        7.69901118419740425e-03 * DAYS_PER_YEAR,
        -6.90460016972063023e-05 * DAYS_PER_YEAR },
      9.54791938424326609e-04 * SOLAR_MASS }, // Jupiter
    { { 8.34336671824457987e+00, 4.12479856412430479e+00, -4.03523417114321381e-01 },
      { -2.76742510726862411e-03 * DAYS_PER_YEAR,
        4.99852801234917238e-03 * DAYS_PER_YEAR,
        2.30417297573763929e-05 * DAYS_PER_YEAR },
      2.85885980666130812e-04 * SOLAR_MASS }, // Saturn
    { { 1.28943695621391310e+01, -1.51111514016986312e+01, -2.23307578892655734e-01 },
      { 2.96460137564761618e-03 * DAYS_PER_YEAR,
        2.37847173959480950e-03 * DAYS_PER_YEAR,
        -2.96589568540237556e-05 * DAYS_PER_YEAR },
      4.36624404335156298e-05 * SOLAR_MASS }, // Uranus
    { { 1.53796971148509165e+01, -2.59193146099879641e+01, 1.79258772950371181e-01 },
      { 2.68067772490389322e-03 * DAYS_PER_YEAR,
        1.62824170038242295e-03 * DAYS_PER_YEAR,
        -9.51592254519715870e-05 * DAYS_PER_YEAR },
      5.15138902046611451e-05 * SOLAR_MASS }  // Neptune
};

const int N_BODIES = sizeof(bodies) / sizeof(Body);

void offset_momentum() {
    double px = 0, py = 0, pz = 0;

    for (int i = 0; i < N_BODIES; i++) {
        px += bodies[i].velocity[0] * bodies[i].mass;
        py += bodies[i].velocity[1] * bodies[i].mass;
        pz += bodies[i].velocity[2] * bodies[i].mass;
    }

    bodies[0].velocity[0] = -px / SOLAR_MASS;
    bodies[0].velocity[1] = -py / SOLAR_MASS;
    bodies[0].velocity[2] = -pz / SOLAR_MASS;
}

void advance(double dt, int steps) {
    for (int step = 0; step < steps; step++) {
        for (int i = 0; i < N_BODIES; i++) {
            for (int j = i + 1; j < N_BODIES; j++) {
                double dx = bodies[i].position[0] - bodies[j].position[0];
                double dy = bodies[i].position[1] - bodies[j].position[1];
                double dz = bodies[i].position[2] - bodies[j].position[2];

                double dist_sq = dx * dx + dy * dy + dz * dz;
                double dist = sqrt(dist_sq);
                double mag = dt / (dist_sq * dist);

                double m1 = bodies[i].mass * mag;
                double m2 = bodies[j].mass * mag;

                bodies[i].velocity[0] -= dx * m2;
                bodies[i].velocity[1] -= dy * m2;
                bodies[i].velocity[2] -= dz * m2;

                bodies[j].velocity[0] += dx * m1;
                bodies[j].velocity[1] += dy * m1;
                bodies[j].velocity[2] += dz * m1;
            }
        }

        for (int i = 0; i < N_BODIES; i++) {
            bodies[i].position[0] += dt * bodies[i].velocity[0];
            bodies[i].position[1] += dt * bodies[i].velocity[1];
            bodies[i].position[2] += dt * bodies[i].velocity[2];
        }
    }
}

void report_energy() {
    double e = 0.0;
    for (int i = 0; i < N_BODIES; i++) {
        double vx = bodies[i].velocity[0];
        double vy = bodies[i].velocity[1];
        double vz = bodies[i].velocity[2];
        e += 0.5 * bodies[i].mass * (vx * vx + vy * vy + vz * vz);

        for (int j = i + 1; j < N_BODIES; j++) {
            double dx = bodies[i].position[0] - bodies[j].position[0]()

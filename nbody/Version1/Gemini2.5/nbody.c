// The Computer Language Benchmarks Game
// https://salsa.debian.org/benchmarksgame-team/benchmarksgame/
//
// Originally by Kevin Carson
// Modified by Tupteq, Fredrik Johansson, and Daniel Nanz
// Modified by Maciej Fijalkowski
// Ported to C by an AI assistant

#include <stdio.h>
#include <stdlib.h>
#include <math.h>

// Define constants for the simulation
#define PI 3.14159265358979323
#define SOLAR_MASS (4.0 * PI * PI)
#define DAYS_PER_YEAR 365.24

#define NBODIES 5
#define NPAIRS 10 // Number of pairs of bodies: n*(n-1)/2 = 5*4/2 = 10

// Structure to represent a celestial body
struct body {
    double x, y, z;      // Position coordinates
    double vx, vy, vz;   // Velocity components
    double mass;         // Mass
};

// Global array to hold the state of the celestial bodies
// Initial data matches the Python 'BODIES' dictionary
struct body bodies[NBODIES] = {
    // Sun
    { .x=0.0, .y=0.0, .z=0.0, .vx=0.0, .vy=0.0, .vz=0.0, .mass=SOLAR_MASS },

    // Jupiter
    { .x=4.84143144246472090e+00,
      .y=-1.16032004402742839e+00,
      .z=-1.03622044471123109e-01,
      .vx=1.66007664274403694e-03 * DAYS_PER_YEAR,
      .vy=7.69901118419740425e-03 * DAYS_PER_YEAR,
      .vz=-6.90460016972063023e-05 * DAYS_PER_YEAR,
      .mass=9.54791938424326609e-04 * SOLAR_MASS },

    // Saturn
    { .x=8.34336671824457987e+00,
      .y=4.12479856412430479e+00,
      .z=-4.03523417114321381e-01,
      .vx=-2.76742510726862411e-03 * DAYS_PER_YEAR,
      .vy=4.99852801234917238e-03 * DAYS_PER_YEAR,
      .vz=2.30417297573763929e-05 * DAYS_PER_YEAR,
      .mass=2.85885980666130812e-04 * SOLAR_MASS },

    // Uranus
    { .x=1.28943695621391310e+01,
      .y=-1.51111514016986312e+01,
      .z=-2.23307578892655734e-01,
      .vx=2.96460137564761618e-03 * DAYS_PER_YEAR,
      .vy=2.37847173959480950e-03 * DAYS_PER_YEAR,
      .vz=-2.96589568540237556e-05 * DAYS_PER_YEAR,
      .mass=4.36624404335156298e-05 * SOLAR_MASS },

    // Neptune
    { .x=1.53796971148509165e+01,
      .y=-2.59193146099879641e+01,
      .z=1.79258772950371181e-01,
      .vx=2.68067772490389322e-03 * DAYS_PER_YEAR,
      .vy=1.62824170038242295e-03 * DAYS_PER_YEAR,
      .vz=-9.51592254519715870e-05 * DAYS_PER_YEAR,
      .mass=5.15138902046611451e-05 * SOLAR_MASS }
};

// Pre-calculated pairs of bodies to iterate over
int pairs[NPAIRS][2] = {
    {0, 1}, {0, 2}, {0, 3}, {0, 4},
    {1, 2}, {1, 3}, {1, 4},
    {2, 3}, {2, 4},
    {3, 4}
};


// Advances the simulation by n steps with a time step of dt
void advance(double dt, int n) {
    for (int i = 0; i < n; ++i) {
        // Update velocities first based on gravitational forces between pairs
        for (int j = 0; j < NPAIRS; ++j) {
            struct body *b1 = &bodies[pairs[j][0]];
            struct body *b2 = &bodies[pairs[j][1]];

            double dx = b1->x - b2->x;
            double dy = b1->y - b2->y;
            double dz = b1->z - b2->z;

            double dist_sq = dx * dx + dy * dy + dz * dz;
            double mag = dt / (dist_sq * sqrt(dist_sq));

            b1->vx -= dx * b2->mass * mag;
            b1->vy -= dy * b2->mass * mag;
            b1->vz -= dz * b2->mass * mag;

            b2->vx += dx * b1->mass * mag;
            b2->vy += dy * b1->mass * mag;
            b2->vz += dz * b1->mass * mag;
        }

        // Update positions based on the new velocities
        for (int j = 0; j < NBODIES; ++j) {
            bodies[j].x += dt * bodies[j].vx;
            bodies[j].y += dt * bodies[j].vy;
            bodies[j].z += dt * bodies[j].vz;
        }
    }
}

// Calculates and prints the total energy of the system
void report_energy() {
    double e = 0.0;

    // Sum potential and kinetic energy
    for (int i = 0; i < NPAIRS; ++i) {
        struct body *b1 = &bodies[pairs[i][0]];
        struct body *b2 = &bodies[pairs[i][1]];

        double dx = b1->x - b2->x;
        double dy = b1->y - b2->y;
        double dz = b1->z - b2->z;
        double distance = sqrt(dx * dx + dy * dy + dz * dz);
        e -= (b1->mass * b2->mass) / distance;
    }

    for (int i = 0; i < NBODIES; ++i) {
        struct body *b = &bodies[i];
        e += 0.5 * b->mass * (b->vx * b->vx + b->vy * b->vy + b->vz * b->vz);
    }

    printf("%.9f\n", e);
}

// Offsets the sun's momentum to make the total system momentum zero
void offset_momentum() {
    double px = 0.0, py = 0.0, pz = 0.0;

    for (int i = 0; i < NBODIES; ++i) {
        px -= bodies[i].vx * bodies[i].mass;
        py -= bodies[i].vy * bodies[i].mass;
        pz -= bodies[i].vz * bodies[i].mass;
    }
    
    // The reference body is the sun, at index 0
    bodies[0].vx = px / bodies[0].mass;
    bodies[0].vy = py / bodies[0].mass;
    bodies[0].vz = pz / bodies[0].mass;
}


int main(int argc, char **argv) {
    if (argc != 2) {
        fprintf(stderr, "Usage: %s <number_of_steps>\n", argv[0]);
        return 1;
    }
    int n = atoi(argv[1]);

    offset_momentum();
    report_energy();
    advance(0.01, n);
    report_energy();

    return 0;
}

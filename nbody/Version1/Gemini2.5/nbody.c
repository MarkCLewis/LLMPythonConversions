#include <stdio.h>
#include <stdlib.h>
#include <math.h>

#define PI 3.14159265358979323
#define SOLAR_MASS (4.0 * PI * PI)
#define DAYS_PER_YEAR 365.24
#define NUM_BODIES 5

// A struct to hold the data for each celestial body.
// Using 'double' for high precision.
typedef struct {
    double x, y, z;
    double vx, vy, vz;
    double mass;
} Body;

// Global array to hold all bodies in the system.
Body bodies[NUM_BODIES];

// Calculates the total energy of the system and prints it.
void report_energy() {
    double energy = 0.0;

    // Calculate kinetic energy for each body
    for (int i = 0; i < NUM_BODIES; ++i) {
        energy += 0.5 * bodies[i].mass *
                  (bodies[i].vx * bodies[i].vx +
                   bodies[i].vy * bodies[i].vy +
                   bodies[i].vz * bodies[i].vz);
    }

    // Calculate potential energy between each pair of bodies
    for (int i = 0; i < NUM_BODIES; ++i) {
        for (int j = i + 1; j < NUM_BODIES; ++j) {
            double dx = bodies[i].x - bodies[j].x;
            double dy = bodies[i].y - bodies[j].y;
            double dz = bodies[i].z - bodies[j].z;
            double distance = sqrt(dx * dx + dy * dy + dz * dz);
            energy -= (bodies[i].mass * bodies[j].mass) / distance;
        }
    }
    printf("%.9f\n", energy);
}

// Advances the simulation by n steps with a time delta of dt.
void advance(double dt, int n) {
    for (int i = 0; i < n; ++i) {
        // First, update velocities for all pairs based on gravitational force
        for (int j = 0; j < NUM_BODIES; ++j) {
            for (int k = j + 1; k < NUM_BODIES; ++k) {
                Body* b1 = &bodies[j];
                Body* b2 = &bodies[k];

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
        }

        // Then, update positions for all bodies based on their new velocities
        for (int j = 0; j < NUM_BODIES; ++j) {
            bodies[j].x += dt * bodies[j].vx;
            bodies[j].y += dt * bodies[j].vy;
            bodies[j].z += dt * bodies[j].vz;
        }
    }
}

// Offsets the system's momentum to make the net momentum zero.
// The Sun (bodies[0]) is used as the reference body.
void offset_momentum() {
    double px = 0.0, py = 0.0, pz = 0.0;
    for (int i = 0; i < NUM_BODIES; ++i) {
        px += bodies[i].vx * bodies[i].mass;
        py += bodies[i].vy * bodies[i].mass;
        pz += bodies[i].vz * bodies[i].mass;
    }

    bodies[0].vx = -px / SOLAR_MASS;
    bodies[0].vy = -py / SOLAR_MASS;
    bodies[0].vz = -pz / SOLAR_MASS;
}

int main(int argc, char *argv[]) {
    if (argc < 2) {
        printf("Usage: %s <number_of_steps>\n", argv[0]);
        return 1;
    }
    int n = atoi(argv[1]);

    // Initialize body data
    // Sun
    bodies[0] = (Body){.x = 0.0, .y = 0.0, .z = 0.0, .vx = 0.0, .vy = 0.0, .vz = 0.0, .mass = SOLAR_MASS};
    // Jupiter
    bodies[1] = (Body){
        .x = 4.84143144246472090e+00, .y = -1.16032004402742839e+00, .z = -1.03622044471123109e-01,
        .vx = 1.66007664274403694e-03 * DAYS_PER_YEAR, .vy = 7.69901118419740425e-03 * DAYS_PER_YEAR, .vz = -6.90460016972063023e-05 * DAYS_PER_YEAR,
        .mass = 9.54791938424326609e-04 * SOLAR_MASS};
    // Saturn
    bodies[2] = (Body){
        .x = 8.34336671824457987e+00, .y = 4.12479856412430479e+00, .z = -4.03523417114321381e-01,
        .vx = -2.76742510726862411e-03 * DAYS_PER_YEAR, .vy = 4.99852801234917238e-03 * DAYS_PER_YEAR, .vz = 2.30417297573763929e-05 * DAYS_PER_YEAR,
        .mass = 2.85885980666130812e-04 * SOLAR_MASS};
    // Uranus
    bodies[3] = (Body){
        .x = 1.28943695621391310e+01, .y = -1.51111514016986312e+01, .z = -2.23307578892655734e-01,
        .vx = 2.96460137564761618e-03 * DAYS_PER_YEAR, .vy = 2.37847173959480950e-03 * DAYS_PER_YEAR, .vz = -2.96589568540237556e-05 * DAYS_PER_YEAR,
        .mass = 4.36624404335156298e-05 * SOLAR_MASS};
    // Neptune
    bodies[4] = (Body){
        .x = 1.53796971148509165e+01, .y = -2.59193146099879641e+01, .z = 1.79258772950371181e-01,
        .vx = 2.68067772490389322e-03 * DAYS_PER_YEAR, .vy = 1.62824170038242295e-03 * DAYS_PER_YEAR, .vz = -9.51592254519715870e-05 * DAYS_PER_YEAR,
        .mass = 5.15138902046611451e-05 * SOLAR_MASS};

    offset_momentum();
    report_energy();
    advance(0.01, n);
    report_energy();

    return 0;
}

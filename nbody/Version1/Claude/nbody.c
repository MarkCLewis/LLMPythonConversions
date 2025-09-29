/*
 * The Computer Language Benchmarks Game
 * https://salsa.debian.org/benchmarksgame-team/benchmarksgame/
 *
 * Converted from Python to C by Claude
 * Original by Kevin Carson
 * Modified by Tupteq, Fredrik Johansson, and Daniel Nanz
 * Modified by Maciej Fijalkowski
 */

#include <stdio.h>
#include <stdlib.h>
#include <math.h>

#define PI 3.14159265358979323
#define SOLAR_MASS (4 * PI * PI)
#define DAYS_PER_YEAR 365.24
#define N_BODIES 5

typedef struct {
    double x, y, z;
    double vx, vy, vz;
    double mass;
} body;

// Initialize the solar system bodies
body solar_bodies[N_BODIES] = {
    // Sun
    {
        0.0, 0.0, 0.0,
        0.0, 0.0, 0.0,
        SOLAR_MASS
    },
    // Jupiter
    {
        4.84143144246472090e+00, -1.16032004402742839e+00, -1.03622044471123109e-01,
        1.66007664274403694e-03 * DAYS_PER_YEAR, 7.69901118419740425e-03 * DAYS_PER_YEAR, -6.90460016972063023e-05 * DAYS_PER_YEAR,
        9.54791938424326609e-04 * SOLAR_MASS
    },
    // Saturn
    {
        8.34336671824457987e+00, 4.12479856412430479e+00, -4.03523417114321381e-01,
        -2.76742510726862411e-03 * DAYS_PER_YEAR, 4.99852801234917238e-03 * DAYS_PER_YEAR, 2.30417297573763929e-05 * DAYS_PER_YEAR,
        2.85885980666130812e-04 * SOLAR_MASS
    },
    // Uranus
    {
        1.28943695621391310e+01, -1.51111514016986312e+01, -2.23307578892655734e-01,
        2.96460137564761618e-03 * DAYS_PER_YEAR, 2.37847173959480950e-03 * DAYS_PER_YEAR, -2.96589568540237556e-05 * DAYS_PER_YEAR,
        4.36624404335156298e-05 * SOLAR_MASS
    },
    // Neptune
    {
        1.53796971148509165e+01, -2.59193146099879641e+01, 1.79258772950371181e-01,
        2.68067772490389322e-03 * DAYS_PER_YEAR, 1.62824170038242295e-03 * DAYS_PER_YEAR, -9.51592254519715870e-05 * DAYS_PER_YEAR,
        5.15138902046611451e-05 * SOLAR_MASS
    }
};

// Advance the simulation by one step
void advance(body bodies[], double dt, int n) {
    for (int step = 0; step < n; step++) {
        // Calculate interactions between all pairs of bodies
        for (int i = 0; i < N_BODIES - 1; i++) {
            for (int j = i + 1; j < N_BODIES; j++) {
                double dx = bodies[i].x - bodies[j].x;
                double dy = bodies[i].y - bodies[j].y;
                double dz = bodies[i].z - bodies[j].z;
                
                double distance_squared = dx * dx + dy * dy + dz * dz;
                double distance = sqrt(distance_squared);
                double mag = dt / (distance_squared * distance);
                
                double body_i_mass_mag = bodies[i].mass * mag;
                double body_j_mass_mag = bodies[j].mass * mag;
                
                // Update velocities based on gravitational forces
                bodies[i].vx -= dx * body_j_mass_mag;
                bodies[i].vy -= dy * body_j_mass_mag;
                bodies[i].vz -= dz * body_j_mass_mag;
                
                bodies[j].vx += dx * body_i_mass_mag;
                bodies[j].vy += dy * body_i_mass_mag;
                bodies[j].vz += dz * body_i_mass_mag;
            }
        }
        
        // Update positions based on velocities
        for (int i = 0; i < N_BODIES; i++) {
            bodies[i].x += dt * bodies[i].vx;
            bodies[i].y += dt * bodies[i].vy;
            bodies[i].z += dt * bodies[i].vz;
        }
    }
}

// Calculate and report the total energy of the system
void report_energy(body bodies[]) {
    double energy = 0.0;
    
    // Calculate potential energy between all pairs
    for (int i = 0; i < N_BODIES - 1; i++) {
        for (int j = i + 1; j < N_BODIES; j++) {
            double dx = bodies[i].x - bodies[j].x;
            double dy = bodies[i].y - bodies[j].y;
            double dz = bodies[i].z - bodies[j].z;
            
            double distance = sqrt(dx * dx + dy * dy + dz * dz);
            energy -= (bodies[i].mass * bodies[j].mass) / distance;
        }
    }
    
    // Add kinetic energy for each body
    for (int i = 0; i < N_BODIES; i++) {
        energy += 0.5 * bodies[i].mass * (
            bodies[i].vx * bodies[i].vx +
            bodies[i].vy * bodies[i].vy +
            bodies[i].vz * bodies[i].vz
        );
    }
    
    printf("%.9f\n", energy);
}

// Adjust the system to ensure the center of mass is stationary
void offset_momentum(body bodies[]) {
    double px = 0.0, py = 0.0, pz = 0.0;
    
    // Calculate total momentum
    for (int i = 0; i < N_BODIES; i++) {
        px -= bodies[i].vx * bodies[i].mass;
        py -= bodies[i].vy * bodies[i].mass;
        pz -= bodies[i].vz * bodies[i].mass;
    }
    
    // Adjust sun's velocity to offset the total momentum
    bodies[0].vx = px / bodies[0].mass;
    bodies[0].vy = py / bodies[0].mass;
    bodies[0].vz = pz / bodies[0].mass;
}

int main(int argc, char *argv[]) {
    if (argc != 2) {
        fprintf(stderr, "Usage: %s <iterations>\n", argv[0]);
        return 1;
    }
    
    int iterations = atoi(argv[1]);
    
    offset_momentum(solar_bodies);
    report_energy(solar_bodies);
    advance(solar_bodies, 0.01, iterations);
    report_energy(solar_bodies);
    
    return 0;
}
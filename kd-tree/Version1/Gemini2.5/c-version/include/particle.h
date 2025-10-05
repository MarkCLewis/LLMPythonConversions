#ifndef PARTICLE_H
#define PARTICLE_H

#include "vector.h"

typedef struct {
    F64x3 p; // Position
    F64x3 v; // Velocity
    double r; // Radius
    double m; // Mass
} Particle;

Particle* circular_orbits(int n);
F64x3 calc_pp_accel(const Particle* pi, const Particle* pj);

#endif // PARTICLE_H

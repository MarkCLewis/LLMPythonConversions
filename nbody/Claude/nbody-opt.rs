// The Computer Language Benchmarks Game
// https://salsa.debian.org/benchmarksgame-team/benchmarksgame/
//
// Converted from Python to Rust with optimizations by Claude

#![feature(core_intrinsics)]
use std::env;
use std::intrinsics;

const PI: f64 = std::f64::consts::PI;
const SOLAR_MASS: f64 = 4.0 * PI * PI;
const DAYS_PER_YEAR: f64 = 365.24;
const N_BODIES: usize = 5;

// Body struct to represent a celestial body with position and velocity as arrays
// for better memory layout and SIMD optimization potential
#[derive(Clone, Copy)]
struct Body {
    position: [f64; 3],
    velocity: [f64; 3],
    mass: f64,
}

// Pre-computed pairs of bodies for interaction calculations
type BodyPair = (usize, usize);

fn main() {
    // Parse command line arguments
    let args: Vec<String> = env::args().collect();
    if args.len() != 2 {
        eprintln!("Usage: {} <iterations>", args[0]);
        std::process::exit(1);
    }
    
    let n = match args[1].parse::<usize>() {
        Ok(n) => n,
        Err(_) => {
            eprintln!("Error: iterations must be a positive integer");
            std::process::exit(1);
        }
    };
    
    // Initialize bodies
    let mut bodies = [
        // Sun
        Body {
            position: [0.0, 0.0, 0.0],
            velocity: [0.0, 0.0, 0.0],
            mass: SOLAR_MASS
        },
        // Jupiter
        Body {
            position: [
                4.84143144246472090e+00, 
                -1.16032004402742839e+00, 
                -1.03622044471123109e-01
            ],
            velocity: [
                1.66007664274403694e-03 * DAYS_PER_YEAR,
                7.69901118419740425e-03 * DAYS_PER_YEAR,
                -6.90460016972063023e-05 * DAYS_PER_YEAR
            ],
            mass: 9.54791938424326609e-04 * SOLAR_MASS
        },
        // Saturn
        Body {
            position: [
                8.34336671824457987e+00,
                4.12479856412430479e+00,
                -4.03523417114321381e-01
            ],
            velocity: [
                -2.76742510726862411e-03 * DAYS_PER_YEAR,
                4.99852801234917238e-03 * DAYS_PER_YEAR,
                2.30417297573763929e-05 * DAYS_PER_YEAR
            ],
            mass: 2.85885980666130812e-04 * SOLAR_MASS
        },
        // Uranus
        Body {
            position: [
                1.28943695621391310e+01,
                -1.51111514016986312e+01,
                -2.23307578892655734e-01
            ],
            velocity: [
                2.96460137564761618e-03 * DAYS_PER_YEAR,
                2.37847173959480950e-03 * DAYS_PER_YEAR,
                -2.96589568540237556e-05 * DAYS_PER_YEAR
            ],
            mass: 4.36624404335156298e-05 * SOLAR_MASS
        },
        // Neptune
        Body {
            position: [
                1.53796971148509165e+01,
                -2.59193146099879641e+01,
                1.79258772950371181e-01
            ],
            velocity: [
                2.68067772490389322e-03 * DAYS_PER_YEAR,
                1.62824170038242295e-03 * DAYS_PER_YEAR,
                -9.51592254519715870e-05 * DAYS_PER_YEAR
            ],
            mass: 5.15138902046611451e-05 * SOLAR_MASS
        }
    ];
    
    // Pre-compute body pairs
    let body_pairs = generate_body_pairs();
    
    // Run the simulation
    offset_momentum(&mut bodies);
    report_energy(&bodies, &body_pairs);
    
    // Choose the most appropriate algorithm based on iteration count
    if n > 100_000 {
        advance_optimized(&mut bodies, 0.01, n, &body_pairs);
    } else {
        advance(&mut bodies, 0.01, n, &body_pairs);
    }
    
    report_energy(&bodies, &body_pairs);
}

// Generate all pairs of body indices
#[inline]
fn generate_body_pairs() -> Vec<BodyPair> {
    let mut pairs = Vec::with_capacity((N_BODIES * (N_BODIES - 1)) / 2);
    for i in 0..N_BODIES-1 {
        for j in i+1..N_BODIES {
            pairs.push((i, j));
        }
    }
    pairs
}

// Advance the simulation by one step
fn advance(bodies: &mut [Body; N_BODIES], dt: f64, n: usize, pairs: &[BodyPair]) {
    for _ in 0..n {
        // Update velocities based on gravitational forces
        for &(i, j) in pairs {
            let dx = bodies[i].position[0] - bodies[j].position[0];
            let dy = bodies[i].position[1] - bodies[j].position[1];
            let dz = bodies[i].position[2] - bodies[j].position[2];
            
            let distance_squared = dx * dx + dy * dy + dz * dz;
            let distance = unsafe { intrinsics::sqrtf64(distance_squared) };
            let mag = dt / (distance_squared * distance);
            
            let body_i_mass_mag = bodies[i].mass * mag;
            let body_j_mass_mag = bodies[j].mass * mag;
            
            bodies[i].velocity[0] -= dx * body_j_mass_mag;
            bodies[i].velocity[1] -= dy * body_j_mass_mag;
            bodies[i].velocity[2] -= dz * body_j_mass_mag;
            
            bodies[j].velocity[0] += dx * body_i_mass_mag;
            bodies[j].velocity[1] += dy * body_i_mass_mag;
            bodies[j].velocity[2] += dz * body_i_mass_mag;
        }
        
        // Update positions based on velocities
        for body in bodies.iter_mut() {
            body.position[0] += dt * body.velocity[0];
            body.position[1] += dt * body.velocity[1];
            body.position[2] += dt * body.velocity[2];
        }
    }
}

// Optimized version for large numbers of iterations
fn advance_optimized(bodies: &mut [Body; N_BODIES], dt: f64, n: usize, pairs: &[BodyPair]) {
    // Extract position, velocity and mass components into separate arrays
    // for better memory access patterns and potential SIMD optimization
    let mut positions = [[0.0; 3]; N_BODIES];
    let mut velocities = [[0.0; 3]; N_BODIES];
    let mut masses = [0.0; N_BODIES];
    
    for i in 0..N_BODIES {
        positions[i] = bodies[i].position;
        velocities[i] = bodies[i].velocity;
        masses[i] = bodies[i].mass;
    }
    
    // Main simulation loop
    for _ in 0..n {
        // Update velocities based on gravitational forces
        for &(i, j) in pairs {
            let dx = positions[i][0] - positions[j][0];
            let dy = positions[i][1] - positions[j][1];
            let dz = positions[i][2] - positions[j][2];
            
            let distance_squared = dx * dx + dy * dy + dz * dz;
            // Use the intrinsics for potential hardware acceleration
            let distance = unsafe { intrinsics::sqrtf64(distance_squared) };
            let mag = dt / (distance_squared * distance);
            
            let mi_mag = masses[i] * mag;
            let mj_mag = masses[j] * mag;
            
            velocities[i][0] -= dx * mj_mag;
            velocities[i][1] -= dy * mj_mag;
            velocities[i][2] -= dz * mj_mag;
            
            velocities[j][0] += dx * mi_mag;
            velocities[j][1] += dy * mi_mag;
            velocities[j][2] += dz * mi_mag;
        }
        
        // Update positions based on velocities
        for i in 0..N_BODIES {
            positions[i][0] += dt * velocities[i][0];
            positions[i][1] += dt * velocities[i][1];
            positions[i][2] += dt * velocities[i][2];
        }
    }
    
    // Copy back updated positions and velocities
    for i in 0..N_BODIES {
        bodies[i].position = positions[i];
        bodies[i].velocity = velocities[i];
    }
}

// Calculate and report the total energy of the system
fn report_energy(bodies: &[Body; N_BODIES], pairs: &[BodyPair]) {
    let mut energy = 0.0;
    
    // Calculate potential energy
    for &(i, j) in pairs {
        let dx = bodies[i].position[0] - bodies[j].position[0];
        let dy = bodies[i].position[1] - bodies[j].position[1];
        let dz = bodies[i].position[2] - bodies[j].position[2];
        
        let distance = (dx * dx + dy * dy + dz * dz).sqrt();
        energy -= (bodies[i].mass * bodies[j].mass) / distance;
    }
    
    // Add kinetic energy
    for body in bodies.iter() {
        energy += 0.5 * body.mass * (
            body.velocity[0] * body.velocity[0] +
            body.velocity[1] * body.velocity[1] +
            body.velocity[2] * body.velocity[2]
        );
    }
    
    println!("{:.9}", energy);
}

// Adjust the system to ensure the center of mass is stationary
fn offset_momentum(bodies: &mut [Body; N_BODIES]) {
    let mut px = 0.0;
    let mut py = 0.0;
    let mut pz = 0.0;
    
    // Calculate total momentum
    for body in bodies.iter() {
        px -= body.velocity[0] * body.mass;
        py -= body.velocity[1] * body.mass;
        pz -= body.velocity[2] * body.mass;
    }
    
    // Adjust sun's velocity to offset the total momentum
    bodies[0].velocity[0] = px / bodies[0].mass;
    bodies[0].velocity[1] = py / bodies[0].mass;
    bodies[0].velocity[2] = pz / bodies[0].mass;
}
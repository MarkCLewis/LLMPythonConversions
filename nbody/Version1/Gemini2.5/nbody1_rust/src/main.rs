use std::env;

// Define global constants for the simulation.
const PI: f64 = 3.141592653589793;
const SOLAR_MASS: f64 = 4.0 * PI * PI;
const DAYS_PER_YEAR: f64 = 365.24;

// A struct to represent a celestial body with position, velocity, and mass.
// Using fixed-size arrays `[f64; 3]` is more efficient than a dynamic list.
#[derive(Clone, Copy, Debug)]
struct Body {
    position: [f64; 3],
    velocity: [f64; 3],
    mass: f64,
}

/// Advances the simulation by `n` steps with a time delta of `dt`.
fn advance(dt: f64, n: i32, bodies: &mut Vec<Body>, pairs: &[(usize, usize)]) {
    for _ in 0..n {
        // First, update the velocities of all body pairs based on gravitational force.
        for &(i, j) in pairs.iter() {
            // `split_at_mut` is a safe way to get two mutable references into a single vector,
            // which is necessary to modify two bodies at the same time.
            let (slice1, slice2) = bodies.split_at_mut(j);
            let body1 = &mut slice1[i];
            let body2 = &mut slice2[0];

            let dx = body1.position[0] - body2.position[0];
            let dy = body1.position[1] - body2.position[1];
            let dz = body1.position[2] - body2.position[2];

            let dist_sq = dx * dx + dy * dy + dz * dz;
            let mag = dt / (dist_sq * dist_sq.sqrt());

            let b1m = body1.mass * mag;
            let b2m = body2.mass * mag;

            // Update velocities based on the gravitational pull of the other body.
            body1.velocity[0] -= dx * b2m;
            body1.velocity[1] -= dy * b2m;
            body1.velocity[2] -= dz * b2m;

            body2.velocity[0] += dx * b1m;
            body2.velocity[1] += dy * b1m;
            body2.velocity[2] += dz * b1m;
        }

        // Second, update the positions of all bodies based on their new velocities.
        for body in bodies.iter_mut() {
            body.position[0] += dt * body.velocity[0];
            body.position[1] += dt * body.velocity[1];
            body.position[2] += dt * body.velocity[2];
        }
    }
}

/// Calculates and prints the total energy of the system.
fn report_energy(bodies: &[Body], pairs: &[(usize, usize)]) {
    let mut energy = 0.0;

    // The total energy is the sum of potential and kinetic energies.
    for &(i, j) in pairs.iter() {
        let body1 = &bodies[i];
        let body2 = &bodies[j];
        
        let dx = body1.position[0] - body2.position[0];
        let dy = body1.position[1] - body2.position[1];
        let dz = body1.position[2] - body2.position[2];
        
        let dist_sq = dx * dx + dy * dy + dz * dz;
        // Potential energy
        energy -= (body1.mass * body2.mass) / dist_sq.sqrt();
    }
    
    for body in bodies.iter() {
        // Kinetic energy
        energy += 0.5 * body.mass * (
            body.velocity[0] * body.velocity[0] +
            body.velocity[1] * body.velocity[1] +
            body.velocity[2] * body.velocity[2]
        );
    }
    
    // Print the result with 9 decimal places, matching the Python script.
    println!("{:.9}", energy);
}

/// Offsets the sun's momentum to make the total system momentum zero.
fn offset_momentum(bodies: &mut Vec<Body>) {
    let mut px = 0.0;
    let mut py = 0.0;
    let mut pz = 0.0;

    for body in bodies.iter() {
        px += body.velocity[0] * body.mass;
        py += body.velocity[1] * body.mass;
        pz += body.velocity[2] * body.mass;
    }

    // The sun is the first body in our vector (index 0).
    bodies[0].velocity[0] = -px / SOLAR_MASS;
    bodies[0].velocity[1] = -py / SOLAR_MASS;
    bodies[0].velocity[2] = -pz / SOLAR_MASS;
}


fn main() {
    // Read the number of simulation steps from the command line.
    let n = env::args().nth(1)
        .expect("Usage: nbody_rust <steps>")
        .parse::<i32>()
        .expect("Steps must be an integer");

    // Initialize the solar system bodies.
    let mut bodies = vec![
        Body { // Sun
            position: [0.0; 3], velocity: [0.0; 3], mass: SOLAR_MASS },
        Body { // Jupiter
            position: [4.8414314424647209, -1.16032004402742839, -0.103622044471123109],
            velocity: [
                1.6600766427440369e-3 * DAYS_PER_YEAR,
                7.699011184197404e-3 * DAYS_PER_YEAR,
                -6.90460016972063e-5 * DAYS_PER_YEAR,
            ],
            mass: 9.547919384243266e-4 * SOLAR_MASS,
        },
        Body { // Saturn
            position: [8.34336671824457987, 4.12479856412430479, -0.403523417114321381],
            velocity: [
                -2.7674251072686241e-3 * DAYS_PER_YEAR,
                4.998528012349173e-3 * DAYS_PER_YEAR,
                2.3041729757376393e-5 * DAYS_PER_YEAR,
            ],
            mass: 2.858859806661308e-4 * SOLAR_MASS,
        },
        Body { // Uranus
            position: [12.894369562139131, -15.111151401698631, -0.223307578892655734],
            velocity: [
                2.964601375647616e-3 * DAYS_PER_YEAR,
                2.3784717395948095e-3 * DAYS_PER_YEAR,
                -2.9658956854023756e-5 * DAYS_PER_YEAR,
            ],
            mass: 4.366244043351563e-5 * SOLAR_MASS,
        },
        Body { // Neptune
            position: [15.379697114850916, -25.919314609987964, 0.179258772950371181],
            velocity: [
                2.680677724903893e-3 * DAYS_PER_YEAR,
                1.628241700382423e-3 * DAYS_PER_YEAR,
                -9.515922545197158e-5 * DAYS_PER_YEAR,
            ],
            mass: 5.1513890204661145e-5 * SOLAR_MASS,
        },
    ];

    // Generate the pairs of body indices for interaction calculations.
    let mut pairs = Vec::new();
    for i in 0..bodies.len() {
        for j in i + 1..bodies.len() {
            pairs.push((i, j));
        }
    }

    offset_momentum(&mut bodies);
    report_energy(&bodies, &pairs);
    advance(0.01, n, &mut bodies, &pairs);
    report_energy(&bodies, &pairs);
}

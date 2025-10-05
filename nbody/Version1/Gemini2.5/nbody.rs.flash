// The Computer Language Benchmarks Game
// https://salsa.debian.org/benchmarksgame-team/benchmarksgame/
//
// Contributed by Jeremy Zerbe
// Based on the C++ version by Mark M. Attwood
// Converted from Python by an AI assistant

use std::env;

const PI: f64 = 3.141592653589793;
const SOLAR_MASS: f64 = 4.0 * PI * PI;
const DAYS_PER_YEAR: f64 = 365.24;

#[derive(Clone, Copy, Debug)]
struct Body {
    position: [f64; 3],
    velocity: [f64; 3],
    mass: f64,
}

impl Body {
    // Helper function to create a new Body instance
    fn new(position: [f64; 3], velocity: [f64; 3], mass: f64) -> Body {
        Body {
            position,
            velocity,
            mass,
        }
    }
}

// Create the initial state of the solar system
fn initial_system() -> Vec<Body> {
    vec![
        // Sun
        Body::new(
            [0.0, 0.0, 0.0],
            [0.0, 0.0, 0.0],
            SOLAR_MASS,
        ),
        // Jupiter
        Body::new(
            [
                4.8414314424647209,
                -1.16032004402742839,
                -0.103622044471123109,
            ],
            [
                1.660076642744037e-3 * DAYS_PER_YEAR,
                7.699011184197404e-3 * DAYS_PER_YEAR,
                -6.90460016972063e-5 * DAYS_PER_YEAR,
            ],
            9.547919384243266e-4 * SOLAR_MASS,
        ),
        // Saturn
        Body::new(
            [
                8.343366718244579,
                4.124798564124305,
                -0.4035234171143214,
            ],
            [
                -2.767425107268624e-3 * DAYS_PER_YEAR,
                4.998528012349172e-3 * DAYS_PER_YEAR,
                2.3041729757376393e-5 * DAYS_PER_YEAR,
            ],
            2.858859806661308e-4 * SOLAR_MASS,
        ),
        // Uranus
        Body::new(
            [
                12.894369562139131,
                -15.111151401698631,
                -0.22330757889265573,
            ],
            [
                2.964601375647616e-3 * DAYS_PER_YEAR,
                2.3784717395948095e-3 * DAYS_PER_YEAR,
                -2.9658956854023756e-5 * DAYS_PER_YEAR,
            ],
            4.366244043351563e-5 * SOLAR_MASS,
        ),
        // Neptune
        Body::new(
            [
                15.379697114850916,
                -25.919314609987964,
                0.17925877295037118,
            ],
            [
                2.680677724903893e-3 * DAYS_PER_YEAR,
                1.628241700382423e-3 * DAYS_PER_YEAR,
                -9.515922545197158e-5 * DAYS_PER_YEAR,
            ],
            5.1513890204661145e-5 * SOLAR_MASS,
        ),
    ]
}

/// Advance the simulation by n steps of size dt.
fn advance(dt: f64, n: i32, bodies: &mut [Body]) {
    for _ in 0..n {
        // Update velocities based on gravitational forces between pairs of bodies
        for i in 0..bodies.len() {
            // Use `split_at_mut` to safely get mutable references to two
            // different elements in the `bodies` slice.
            let (first_slice, second_slice) = bodies.split_at_mut(i + 1);
            let body1 = &mut first_slice[i];

            for body2 in second_slice.iter_mut() {
                let dx = body1.position[0] - body2.position[0];
                let dy = body1.position[1] - body2.position[1];
                let dz = body1.position[2] - body2.position[2];

                let d_squared = dx * dx + dy * dy + dz * dz;
                let mag = dt / (d_squared * d_squared.sqrt());

                let b1m = body1.mass * mag;
                let b2m = body2.mass * mag;

                body1.velocity[0] -= dx * b2m;
                body1.velocity[1] -= dy * b2m;
                body1.velocity[2] -= dz * b2m;

                body2.velocity[0] += dx * b1m;
                body2.velocity[1] += dy * b1m;
                body2.velocity[2] += dz * b1m;
            }
        }

        // Update positions based on new velocities
        for body in bodies.iter_mut() {
            body.position[0] += dt * body.velocity[0];
            body.position[1] += dt * body.velocity[1];
            body.position[2] += dt * body.velocity[2];
        }
    }
}

/// Calculate the total energy of the system.
fn report_energy(bodies: &[Body]) {
    let mut energy = 0.0;

    // Sum potential and kinetic energy
    for i in 0..bodies.len() {
        let body1 = &bodies[i];
        
        // Kinetic energy
        energy += 0.5 * body1.mass * (body1.velocity[0].powi(2)
                                    + body1.velocity[1].powi(2)
                                    + body1.velocity[2].powi(2));

        // Potential energy (calculated between pairs)
        for j in (i + 1)..bodies.len() {
            let body2 = &bodies[j];
            let dx = body1.position[0] - body2.position[0];
            let dy = body1.position[1] - body2.position[1];
            let dz = body1.position[2] - body2.position[2];
            let distance = (dx * dx + dy * dy + dz * dz).sqrt();
            energy -= (body1.mass * body2.mass) / distance;
        }
    }

    println!("{:.9}", energy);
}

/// Offset the system's momentum to be zero.
fn offset_momentum(bodies: &mut [Body]) {
    let mut px = 0.0;
    let mut py = 0.0;
    let mut pz = 0.0;

    for body in bodies.iter() {
        px += body.velocity[0] * body.mass;
        py += body.velocity[1] * body.mass;
        pz += body.velocity[2] * body.mass;
    }

    // The sun is the reference body at index 0
    let sun = &mut bodies[0];
    sun.velocity[0] = -px / SOLAR_MASS;
    sun.velocity[1] = -py / SOLAR_MASS;
    sun.velocity[2] = -pz / SOLAR_MASS;
}


fn main() {
    // Get the number of simulation steps from command-line arguments
    let n = env::args().nth(1)
        .and_then(|s| s.parse::<i32>().ok())
        .unwrap_or(1000); // Default to 1000 steps if no argument is given

    let mut system = initial_system();

    offset_momentum(&mut system);
    report_energy(&system);
    advance(0.01, n, &mut system);
    report_energy(&system);
}

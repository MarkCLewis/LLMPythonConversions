// The Computer Language Benchmarks Game
// https://salsa.debian.org/benchmarksgame-team/benchmarksgame/
//
// Converted from Python to Rust by Claude

use std::env;

const PI: f64 = 3.14159265358979323;
const SOLAR_MASS: f64 = 4.0 * PI * PI;
const DAYS_PER_YEAR: f64 = 365.24;
const N_BODIES: usize = 5;

// Body struct to represent a celestial body
struct Body {
    x: f64, y: f64, z: f64,
    vx: f64, vy: f64, vz: f64,
    mass: f64,
}

// Create pairs of body indices
fn combinations(n: usize) -> Vec<(usize, usize)> {
    let mut result = Vec::new();
    for i in 0..n-1 {
        for j in i+1..n {
            result.push((i, j));
        }
    }
    result
}

// Advance the simulation by one step
fn advance(bodies: &mut [Body], dt: f64, n: usize) {
    // Pre-calculate body pairs
    let pairs = combinations(N_BODIES);
    
    for _ in 0..n {
        // Update velocities based on gravitational forces
        for &(i, j) in &pairs {
            let dx = bodies[i].x - bodies[j].x;
            let dy = bodies[i].y - bodies[j].y;
            let dz = bodies[i].z - bodies[j].z;
            
            let distance_squared = dx * dx + dy * dy + dz * dz;
            let mag = dt / (distance_squared * distance_squared.sqrt());
            
            let body_i_mass_mag = bodies[i].mass * mag;
            let body_j_mass_mag = bodies[j].mass * mag;
            
            bodies[i].vx -= dx * body_j_mass_mag;
            bodies[i].vy -= dy * body_j_mass_mag;
            bodies[i].vz -= dz * body_j_mass_mag;
            
            bodies[j].vx += dx * body_i_mass_mag;
            bodies[j].vy += dy * body_i_mass_mag;
            bodies[j].vz += dz * body_i_mass_mag;
        }
        
        // Update positions based on velocities
        for body in bodies.iter_mut() {
            body.x += dt * body.vx;
            body.y += dt * body.vy;
            body.z += dt * body.vz;
        }
    }
}

// Calculate and report the total energy of the system
fn report_energy(bodies: &[Body]) {
    let mut energy = 0.0;
    let pairs = combinations(N_BODIES);
    
    // Calculate potential energy
    for &(i, j) in &pairs {
        let dx = bodies[i].x - bodies[j].x;
        let dy = bodies[i].y - bodies[j].y;
        let dz = bodies[i].z - bodies[j].z;
        
        let distance = (dx * dx + dy * dy + dz * dz).sqrt();
        energy -= (bodies[i].mass * bodies[j].mass) / distance;
    }
    
    // Add kinetic energy
    for body in bodies.iter() {
        energy += 0.5 * body.mass * (
            body.vx * body.vx +
            body.vy * body.vy +
            body.vz * body.vz
        );
    }
    
    println!("{:.9}", energy);
}

// Adjust the system to ensure the center of mass is stationary
fn offset_momentum(bodies: &mut [Body]) {
    let mut px = 0.0;
    let mut py = 0.0;
    let mut pz = 0.0;
    
    // Calculate total momentum
    for body in bodies.iter() {
        px -= body.vx * body.mass;
        py -= body.vy * body.mass;
        pz -= body.vz * body.mass;
    }
    
    // Adjust sun's velocity to offset the total momentum
    bodies[0].vx = px / bodies[0].mass;
    bodies[0].vy = py / bodies[0].mass;
    bodies[0].vz = pz / bodies[0].mass;
}

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
            x: 0.0, y: 0.0, z: 0.0,
            vx: 0.0, vy: 0.0, vz: 0.0,
            mass: SOLAR_MASS
        },
        
        // Jupiter
        Body {
            x: 4.84143144246472090e+00, 
            y: -1.16032004402742839e+00, 
            z: -1.03622044471123109e-01,
            vx: 1.66007664274403694e-03 * DAYS_PER_YEAR,
            vy: 7.69901118419740425e-03 * DAYS_PER_YEAR,
            vz: -6.90460016972063023e-05 * DAYS_PER_YEAR,
            mass: 9.54791938424326609e-04 * SOLAR_MASS
        },
        
        // Saturn
        Body {
            x: 8.34336671824457987e+00,
            y: 4.12479856412430479e+00,
            z: -4.03523417114321381e-01,
            vx: -2.76742510726862411e-03 * DAYS_PER_YEAR,
            vy: 4.99852801234917238e-03 * DAYS_PER_YEAR,
            vz: 2.30417297573763929e-05 * DAYS_PER_YEAR,
            mass: 2.85885980666130812e-04 * SOLAR_MASS
        },
        
        // Uranus
        Body {
            x: 1.28943695621391310e+01,
            y: -1.51111514016986312e+01,
            z: -2.23307578892655734e-01,
            vx: 2.96460137564761618e-03 * DAYS_PER_YEAR,
            vy: 2.37847173959480950e-03 * DAYS_PER_YEAR,
            vz: -2.96589568540237556e-05 * DAYS_PER_YEAR,
            mass: 4.36624404335156298e-05 * SOLAR_MASS
        },
        
        // Neptune
        Body {
            x: 1.53796971148509165e+01,
            y: -2.59193146099879641e+01,
            z: 1.79258772950371181e-01,
            vx: 2.68067772490389322e-03 * DAYS_PER_YEAR,
            vy: 1.62824170038242295e-03 * DAYS_PER_YEAR,
            vz: -9.51592254519715870e-05 * DAYS_PER_YEAR,
            mass: 5.15138902046611451e-05 * SOLAR_MASS
        }
    ];
    
    // Run the simulation
    offset_momentum(&mut bodies);
    report_energy(&bodies);
    advance(&mut bodies, 0.01, n);
    report_energy(&bodies);
}
use std::env;
use std::time::Instant;

mod kd_tree;
mod particle;

fn main() {
    let args: Vec<String> = env::args().collect();
    if args.len() < 3 {
        eprintln!("Usage: {} <num_particles> <num_steps>", args[0]);
        return;
    }

    let n: usize = args[1].parse().expect("Failed to parse number of particles");
    let steps: usize = args[2].parse().expect("Failed to parse number of steps");
    let dt = 1e-3;

    let mut bodies = particle::circular_orbits(n);

    let start = Instant::now();
    kd_tree::simple_sim(&mut bodies, dt, steps);
    let duration = start.elapsed();

    println!("{:.6}", duration.as_secs_f64());
}

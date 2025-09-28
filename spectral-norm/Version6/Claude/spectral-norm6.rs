// The Computer Language Benchmarks Game
// https://salsa.debian.org/benchmarksgame-team/benchmarksgame/
//
// Converted from Python to Rust by Claude

use std::env;
use std::thread;
use std::sync::Arc;
use rayon::prelude::*;

/// Calculate A[i,j] = 1/((i+j)*(i+j+1)/2+i+1)
#[inline]
fn eval_a(i: usize, j: usize) -> f64 {
    let ij = i + j;
    1.0 / ((ij * (ij + 1)) / 2 + i + 1) as f64
}

/// Calculate A * u
fn eval_a_times_u(u: &[f64]) -> Vec<f64> {
    (0..u.len())
        .into_par_iter()
        .map(|i| {
            (0..u.len())
                .map(|j| eval_a(i, j) * u[j])
                .sum()
        })
        .collect()
}

/// Calculate A^T * u
fn eval_at_times_u(u: &[f64]) -> Vec<f64> {
    (0..u.len())
        .into_par_iter()
        .map(|i| {
            (0..u.len())
                .map(|j| eval_a(j, i) * u[j])
                .sum()
        })
        .collect()
}

/// Calculate A^T * A * u
fn eval_ata_times_u(u: &[f64]) -> Vec<f64> {
    let au = eval_a_times_u(u);
    eval_at_times_u(&au)
}

fn main() {
    // Parse command line argument
    let n = env::args().nth(1)
        .and_then(|arg| arg.parse::<usize>().ok())
        .unwrap_or_else(|| {
            eprintln!("Usage: {} <n>", env::args().next().unwrap());
            std::process::exit(1);
        });
    
    // Initialize vector u with ones
    let mut u = vec![1.0; n];
    
    // Power iteration: 10 times
    for _ in 0..10 {
        let v = eval_ata_times_u(&u);
        u = eval_ata_times_u(&v);
    }
    
    // Last iteration to get v
    let v = eval_ata_times_u(&u);
    
    // Calculate the result
    let vbv: f64 = u.iter().zip(v.iter()).map(|(&ue, &ve)| ue * ve).sum();
    let vv: f64 = v.iter().map(|&ve| ve * ve).sum();
    
    let result = (vbv / vv).sqrt();
    println!("{:.9}", result);
}
// The Computer Language Benchmarks Game
// https://salsa.debian.org/benchmarksgame-team/benchmarksgame/
//
// Contributed by Sebastien Loisel
// Fixed by Isaac Gouy
// Sped up by Josh Goldfoot
// Dirtily sped up by Simon Descarpentries
// Used list comprehension by Vadim Zelenin
// 2to3
// Translated to Rust and parallelized by an AI assistant

use rayon::prelude::*;
use std::env;

/// Calculates an element of the infinite matrix A.
#[inline]
fn eval_a(i: usize, j: usize) -> f64 {
    let ij = i + j;
    1.0 / ((ij * (ij + 1) / 2 + i + 1) as f64)
}

/// Multiply vector u by matrix A.
fn eval_a_times_u(u: &[f64]) -> Vec<f64> {
    (0..u.len())
        .into_par_iter()
        .map(|i| {
            u.iter()
                .enumerate()
                .map(|(j, &u_j)| eval_a(i, j) * u_j)
                .sum()
        })
        .collect()
}

/// Multiply vector u by the transpose of matrix A.
fn eval_at_times_u(u: &[f64]) -> Vec<f64> {
    (0..u.len())
        .into_par_iter()
        .map(|i| {
            u.iter()
                .enumerate()
                .map(|(j, &u_j)| eval_a(j, i) * u_j)
                .sum()
        })
        .collect()
}

/// Multiply vector u by matrix A transposed times A.
fn eval_ata_times_u(u: &[f64]) -> Vec<f64> {
    let v = eval_a_times_u(u);
    eval_at_times_u(&v)
}

fn main() {
    // 1. Get problem size 'n' from command-line arguments
    let n = env::args_os()
        .nth(1)
        .and_then(|s| s.into_string().ok())
        .and_then(|s| s.parse().ok())
        .unwrap_or(100);

    // 2. Initialize the starting vectors 'u' and 'v'
    let mut u = vec![1.0; n];
    // CORRECTED LINE: Initialize 'v' to satisfy the compiler.
    let mut v = vec![0.0; n];

    // 3. Perform 10 iterations of the power method
    // This approximates the dominant eigenvector of the matrix A*transpose(A)
    for _ in 0..10 {
        v = eval_ata_times_u(&u);
        u = eval_ata_times_u(&v);
    }

    // 4. Calculate the spectral norm using the resulting vectors
    let v_bv = u
        .par_iter()
        .zip(v.par_iter())
        .map(|(&ui, &vi)| ui * vi)
        .sum::<f64>();

    let vv = v.par_iter().map(|&vi| vi * vi).sum::<f64>();

    // 5. Print the final result, formatted to 9 decimal places
    println!("{:.9}", (v_bv / vv).sqrt());
}

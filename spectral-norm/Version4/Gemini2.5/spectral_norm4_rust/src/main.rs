use rayon::prelude::*;
use std::env;

/// The core evaluation function, equivalent to `eval_A` in Python.
/// It calculates an element of an infinite matrix used in the benchmark.
/// The `#[inline]` attribute suggests the compiler place this code directly
/// into its call sites to reduce function call overhead.
#[inline]
fn eval_a(i: usize, j: usize) -> f64 {
    let ij = i + j;
    // The formula is 1.0 / A(i, j), but we calculate A(i, j) here.
    (ij * (ij + 1) / 2 + i + 1) as f64
}

/// Calculates one element of the vector v = A * u.
/// Corresponds to the inner part of `A_sum` in the Python version.
fn multiply_av_elem(u: &[f64], i: usize) -> f64 {
    (0..u.len())
        .map(|j| u[j] / eval_a(i, j))
        .sum()
}

/// Calculates one element of the vector v = A_transpose * u.
/// Corresponds to the inner part of `At_sum` in the Python version.
fn multiply_atv_elem(u: &[f64], i: usize) -> f64 {
    (0..u.len())
        .map(|j| u[j] / eval_a(j, i))
        .sum()
}

/// Computes the matrix-vector multiplication: A_transpose * A * u.
/// This function is parallelized using Rayon.
fn multiply_at_av(u: &[f64]) -> Vec<f64> {
    let n = u.len();

    // Calculate tmp = A * u in parallel.
    // `.into_par_iter()` converts the range into a parallel iterator,
    // distributing the calculations across available CPU cores.
    let tmp: Vec<f64> = (0..n)
        .into_par_iter()
        .map(|i| multiply_av_elem(u, i))
        .collect();

    // Calculate result = A_transpose * tmp in parallel.
    let v: Vec<f64> = (0..n)
        .into_par_iter()
        .map(|i| multiply_atv_elem(&tmp, i))
        .collect();

    v
}

fn main() {
    // Read the problem size N from command-line arguments.
    let n = env::args_os()
        .nth(1)
        .and_then(|s| s.into_string().ok())
        .and_then(|s| s.parse().ok())
        .unwrap_or(100);

    // Initialize vector u with all 1.0s.
    let mut u = vec![1.0; n];
    // Initialize v to satisfy the compiler's initialization checks.
    let mut v = vec![0.0; n];

    // The power method: 10 iterations to approximate the dominant eigenvector.
    for _ in 0..10 {
        v = multiply_at_av(&u);
        u = multiply_at_av(&v);
    }

    // Calculate dot products u . v and v . v.
    // Using iterators is idiomatic and often allows for compiler optimizations like auto-vectorization.
    let v_bv: f64 = u.iter().zip(v.iter()).map(|(&ui, &vi)| ui * vi).sum();
    let vv: f64 = v.iter().map(|&vi| vi * vi).sum();

    // Calculate and print the spectral norm.
    let result = (v_bv / vv).sqrt();
    println!("{:.9}", result);
}

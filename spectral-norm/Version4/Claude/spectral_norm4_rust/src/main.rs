// The Computer Language Benchmarks Game
// https://salsa.debian.org/benchmarksgame-team/benchmarksgame/
//
// Converted from Python to Rust by Claude

use std::env;
use std::thread;
use std::sync::{Arc, Barrier};
use std::cmp::min;
// For CPU cores detection
extern crate num_cpus;

/// Calculate A[i,j] = 1/((i+j)*(i+j+1)/2+i+1)
#[inline]
fn eval_a(i: usize, j: usize) -> f64 {
    let ij = i + j;
    1.0 / ((ij * (ij + 1)) / 2 + i + 1) as f64
}

/// Calculate sum of u[j] / A[i,j] for all j
fn a_sum(u: &[f64], i: usize) -> f64 {
    u.iter().enumerate().map(|(j, &u_j)| u_j * eval_a(i, j)).sum()
}

/// Calculate sum of u[j] / A[j,i] for all j
fn at_sum(u: &[f64], i: usize) -> f64 {
    u.iter().enumerate().map(|(j, &u_j)| u_j * eval_a(j, i)).sum()
}

/// Multiply vector by A and then by A^T in parallel
fn multiply_atav(u: &[f64], v: &mut [f64], tmp: &mut [f64], n: usize, n_threads: usize) {
    // Function to distribute work among threads
    let parallel_work = |input: &[f64], output: &mut [f64], worker: fn(&[f64], usize) -> f64| {
        let chunk_size = (n + n_threads - 1) / n_threads;
        let barrier = Arc::new(Barrier::new(n_threads));
        
        let mut handles = vec![];
        
        for tid in 0..n_threads {
            let input = input.to_owned();
            let barrier = Arc::clone(&barrier);
            
            let start = tid * chunk_size;
            let end = std::cmp::min(start + chunk_size, n);
            
            let handle = thread::spawn(move || {
                let mut results = vec![0.0; end - start];
                
                for (idx, i) in (start..end).enumerate() {
                    results[idx] = worker(&input, i);
                }
                
                barrier.wait();
                (start, results)
            });
            
            handles.push(handle);
        }
        
        // Collect results
        for handle in handles {
            let (start, results) = handle.join().unwrap();
            for (idx, &val) in results.iter().enumerate() {
                output[start + idx] = val;
            }
        }
    };
    
    // First multiply by A
    parallel_work(u, tmp, a_sum);
    
    // Then multiply by A^T
    parallel_work(tmp, v, at_sum);
}

fn main() {
    // Parse command line argument
    let n = env::args().nth(1)
        .and_then(|arg| arg.parse::<usize>().ok())
        .unwrap_or_else(|| {
            eprintln!("Usage: {} <n>", env::args().next().unwrap());
            std::process::exit(1);
        });
    
    // Number of threads to use
    let n_threads = std::cmp::min(4, num_cpus::get());
    
    // Initialize vectors
    let mut u = vec![1.0; n];
    let mut v = vec![0.0; n];
    let mut tmp = vec![0.0; n];
    
    // Perform 10 iterations
    for _ in 0..10 {
        multiply_atav(&u, &mut v, &mut tmp, n, n_threads);
        multiply_atav(&v, &mut u, &mut tmp, n, n_threads);
    }
    
    // Calculate the result
    let mut vbv = 0.0;
    let mut vv = 0.0;
    
    for i in 0..n {
        vbv += u[i] * v[i];
        vv += v[i] * v[i];
    }
    
    let result = (vbv / vv).sqrt();
    println!("{:.9}", result);
}
// The Computer Language Benchmarks Game
// https://salsa.debian.org/benchmarksgame-team/benchmarksgame/
//
// Converted from Python to Rust by Claude

use std::env;
use std::thread;
use std::sync::Arc;

/// Calculate A[i,j] = 1/((i+j)*(i+j+1)/2+i+1)
#[inline]
fn eval_a(i: usize, j: usize) -> f64 {
    let ij = i + j;
    1.0 / ((ij * (ij + 1)) / 2 + i + 1) as f64
}

/// Calculate A * u
fn eval_a_times_u(u: &[f64], n_threads: usize) -> Vec<f64> {
    let n = u.len();
    let mut result = vec![0.0; n];
    let u = Arc::new(u.to_vec());
    
    let chunks = divide_work(n, n_threads);
    let mut handles = vec![];
    
    for (start, end) in chunks {
        let u_ref = Arc::clone(&u);
        let handle = thread::spawn(move || {
            let mut partial_result = vec![0.0; end - start];
            for (idx, i) in (start..end).enumerate() {
                let mut sum = 0.0;
                for j in 0..u_ref.len() {
                    sum += eval_a(i, j) * u_ref[j];
                }
                partial_result[idx] = sum;
            }
            (start, partial_result)
        });
        handles.push(handle);
    }
    
    for handle in handles {
        let (start, partial_result) = handle.join().unwrap();
        for (idx, val) in partial_result.iter().enumerate() {
            result[start + idx] = *val;
        }
    }
    
    result
}

/// Calculate A^T * u
fn eval_at_times_u(u: &[f64], n_threads: usize) -> Vec<f64> {
    let n = u.len();
    let mut result = vec![0.0; n];
    let u = Arc::new(u.to_vec());
    
    let chunks = divide_work(n, n_threads);
    let mut handles = vec![];
    
    for (start, end) in chunks {
        let u_ref = Arc::clone(&u);
        let handle = thread::spawn(move || {
            let mut partial_result = vec![0.0; end - start];
            for (idx, i) in (start..end).enumerate() {
                let mut sum = 0.0;
                for j in 0..u_ref.len() {
                    sum += eval_a(j, i) * u_ref[j];
                }
                partial_result[idx] = sum;
            }
            (start, partial_result)
        });
        handles.push(handle);
    }
    
    for handle in handles {
        let (start, partial_result) = handle.join().unwrap();
        for (idx, val) in partial_result.iter().enumerate() {
            result[start + idx] = *val;
        }
    }
    
    result
}

/// Calculate A^T * A * u
fn eval_ata_times_u(u: &[f64], n_threads: usize) -> Vec<f64> {
    let au = eval_a_times_u(u, n_threads);
    eval_at_times_u(&au, n_threads)
}

/// Helper function to divide work among threads
fn divide_work(n: usize, n_threads: usize) -> Vec<(usize, usize)> {
    let chunk_size = (n + n_threads - 1) / n_threads;
    let mut chunks = Vec::with_capacity(n_threads);
    
    for i in 0..n_threads {
        let start = i * chunk_size;
        if start >= n {
            break;
        }
        let end = std::cmp::min(start + chunk_size, n);
        chunks.push((start, end));
    }
    
    chunks
}

fn main() {
    // Parse command line argument
    let n = env::args().nth(1)
        .and_then(|arg| arg.parse::<usize>().ok())
        .unwrap_or_else(|| {
            eprintln!("Usage: {} <n>", env::args().next().unwrap());
            std::process::exit(1);
        });
    
    // Determine number of threads to use (up to 4, like the original Python code)
    let n_threads = std::cmp::min(4, num_cpus::get());
    
    // Initialize vector u with ones
    let mut u = vec![1.0; n];
    
    // Power iteration: 10 times
    for _ in 0..10 {
        let v = eval_ata_times_u(&u, n_threads);
        u = eval_ata_times_u(&v, n_threads);
    }
    
    // Last iteration to get v
    let v = eval_ata_times_u(&u, n_threads);
    
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
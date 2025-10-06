// The Computer Language Benchmarks Game
// https://salsa.debian.org/benchmarksgame-team/benchmarksgame/
//
// Converted from Python to Rust by Claude
// Based on the Python version contributed by Joerg Baumann

use std::env;
use std::sync::{Arc, Mutex};
use std::thread;
use std::cmp;

// Calculate factorial
fn factorial(n: usize) -> usize {
    (1..=n).product()
}

// Worker function for each thread
fn task(n: usize, start: usize, size: usize) -> (i64, i32) {
    // Arrays for permutation operations
    let mut p: Vec<u8> = (0..n as u8).collect();
    let mut count: Vec<u8> = vec![0; n];

    // Apply initial permutation based on start index
    let mut remainder = start;
    for i in (1..n).rev() {
        let d = remainder / factorial(i);
        remainder %= factorial(i);
        count[i] = d as u8;

        // Rotate the first i+1 elements d times
        for _ in 0..d {
            let first = p[0];
            p[..i].rotate_left(1);
            p[i] = first;
        }
    }

    let mut checksum: i64 = 0;
    let mut max_flips: i32 = 0;
    let mut alternating_factor: i64 = 1;

    // Main loop for permutation generation and flip counting
    for idx in 0..size {
        // Get first element and calculate flips
        let first = p[0];
        if first > 0 {
            // Count flips
            let mut pp = p.clone();
            let mut flips = 1;
            
            while pp[0] > 0 {
                // Reverse first+1 elements
                let first_val = pp[0] as usize;
                pp[..=first_val].reverse();
                flips += 1;
            }
            
            // Update max flips and checksum
            max_flips = cmp::max(max_flips, flips);
            checksum += flips as i64 * alternating_factor;
        }
        
        // Flip sign for alternating sum
        alternating_factor = -alternating_factor;
        
        // Generate next permutation if not the last
        if idx < size - 1 {
            // Swap first two elements
            p.swap(0, 1);
            
            // Generate next permutation using optimized approach
            let mut i = 2;
            while i < n && count[i] as usize >= i {
                count[i] = 0;
                i += 1;
            }
            
            if i < n {
                count[i] += 1;
                
                // Rotate the first i+1 elements
                let first = p[0];
                p[..i].rotate_left(1);
                p[i] = first;
            }
        }
    }
    
    (checksum, max_flips)
}

// Generate and print all permutations
fn generate_permutations(n: usize) {
    let mut p: Vec<u8> = (0..n as u8).collect();
    let mut count: Vec<u8> = vec![0; n];
    
    let total = factorial(n);
    for _ in 0..total {
        // Print current permutation
        for &val in &p {
            print!("{}", val + 1);
        }
        println!();
        
        // Generate next permutation
        let mut i = 1;
        while i < n && count[i] as usize >= i {
            count[i] = 0;
            i += 1;
        }
        
        if i < n {
            count[i] += 1;
            
            // Rotate the first i+1 elements
            let first = p[0];
            p[..i].rotate_left(1);
            p[i] = first;
        }
    }
}

fn main() {
    // Parse command line argument
    let args: Vec<String> = env::args().collect();
    if args.len() < 2 {
        eprintln!("Usage: {} N", args[0]);
        return;
    }
    
    let n: i32 = args[1].parse().unwrap();
    
    if n < 0 {
        // Print all permutations for -n
        generate_permutations(-n as usize);
    } else if n > 0 {
        let n = n as usize;
        let total = factorial(n);
        
        // Get number of CPU cores
        let cpu_count = num_cpus::get();
        
        // Calculate task size and count
        let mut task_size = (total + cpu_count - 1) / cpu_count;
        if task_size < 20000 {
            task_size = total;
        }
        
        // Ensure task_size is even
        task_size = (task_size + 1) & !1;
        
        let task_count = cmp::min((total + task_size - 1) / task_size, cpu_count);
        
        // Results collection
        let checksum = Arc::new(Mutex::new(0i64));
        let max_flips = Arc::new(Mutex::new(0i32));
        
        // Create and run threads
        let mut handles = Vec::with_capacity(task_count);
        
        for i in 0..task_count {
            let checksum_clone = Arc::clone(&checksum);
            let max_flips_clone = Arc::clone(&max_flips);
            let this_size = if i == task_count - 1 {
                total - i * task_size
            } else {
                task_size
            };
            
            let handle = thread::spawn(move || {
                let (task_checksum, task_max_flips) = task(n, i * task_size, this_size);
                
                // Update shared results
                let mut cs = checksum_clone.lock().unwrap();
                *cs += task_checksum;
                
                let mut mf = max_flips_clone.lock().unwrap();
                *mf = cmp::max(*mf, task_max_flips);
            });
            
            handles.push(handle);
        }
        
        // Wait for all threads to complete
        for handle in handles {
            handle.join().unwrap();
        }
        
        // Print results
        let final_checksum = *checksum.lock().unwrap();
        let final_max_flips = *max_flips.lock().unwrap();
        
        println!("{}", final_checksum);
        println!("Pfannkuchen({}) = {}", n, final_max_flips);
    }
}
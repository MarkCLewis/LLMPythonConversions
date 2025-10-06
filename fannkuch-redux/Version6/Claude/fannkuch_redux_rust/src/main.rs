// The Computer Language Benchmarks Game
// https://salsa.debian.org/benchmarksgame-team/benchmarksgame/
//
// Converted from Python to Rust by Claude
// Based on the Python version initially contributed by Isaac Gouy
// and modified by Justin Peel

use std::env;
use std::thread;
use std::cmp;

// Calculate factorial (used for determining total permutations)
fn factorial(n: usize) -> usize {
    (1..=n).product()
}

// Worker function for a specific range of permutations
fn fannkuch_task(n: usize, start_idx: usize, chunk_size: usize) -> (i32, i32) {
    let mut max_flips_count = 0;
    let mut checksum = 0;
    
    // Initialize permutation and count arrays
    let mut perm1: Vec<usize> = (0..n).collect();
    let mut perm: Vec<usize> = vec![0; n];
    let mut count: Vec<usize> = (0..n).collect();
    
    // Skip to starting permutation for this thread
    let mut remaining = start_idx;
    
    // Apply necessary rotations to reach starting permutation
    for i in 1..n {
        let d = remaining / factorial(n - i);
        remaining = remaining % factorial(n - i);
        count[i] = d;
        
        // Apply d rotations of elements 0..i
        for _ in 0..d {
            let first = perm1[0];
            for j in 0..i {
                perm1[j] = perm1[j + 1];
            }
            perm1[i] = first;
        }
    }
    
    let mut perm_sign = (start_idx % 2 == 0) as i32;  // 1 if even, 0 if odd
    let mut permutation_count = 0;
    
    // Process permutations for this chunk
    while permutation_count < chunk_size {
        // Check if first element is not 0
        let k = perm1[0];
        if k != 0 {
            // Copy the permutation for flipping
            perm.copy_from_slice(&perm1);
            
            let mut flips_count = 1;
            let mut k = k;
            let mut kk = perm[k];
            
            // Perform flips until first element is 0
            while kk != 0 {
                // Reverse the first k+1 elements
                perm[0..=k].reverse();
                
                flips_count += 1;
                k = kk;
                kk = perm[kk];
            }
            
            // Update max flips if needed
            max_flips_count = cmp::max(max_flips_count, flips_count);
            
            // Update checksum (add or subtract based on permSign)
            if perm_sign == 1 {
                checksum += flips_count;
            } else {
                checksum -= flips_count;
            }
        }
        
        // Generate next permutation
        permutation_count += 1;
        
        if permutation_count >= chunk_size {
            break;
        }
        
        if perm_sign == 1 {
            // Swap first two elements
            perm1.swap(0, 1);
            perm_sign = 0;
        } else {
            // Swap second and third elements
            perm1.swap(1, 2);
            perm_sign = 1;
            
            // Find next position to rotate
            let mut r = 2;
            while r < n - 1 {
                if count[r] != 0 {
                    break;
                }
                count[r] = r;
                
                // Rotate elements 0..r+1
                let perm0 = perm1[0];
                for i in 0..=r {
                    perm1[i] = perm1[i + 1];
                }
                perm1[r + 1] = perm0;
                
                r += 1;
            }
            
            // Check if we've gone through all permutations
            if r == n - 1 {
                if count[r] == 0 {
                    break;  // Done with all permutations
                }
            }
            
            // Decrement count at position r
            count[r] -= 1;
        }
    }
    
    (checksum, max_flips_count)
}

fn main() {
    // Parse command line argument
    let args: Vec<String> = env::args().collect();
    if args.len() < 2 {
        eprintln!("Usage: {} N", args[0]);
        return;
    }
    
    let n: usize = args[1].parse().expect("Invalid number");
    
    // Get number of available CPU cores
    let num_threads = num_cpus::get();
    
    // Calculate total number of permutations
    let total_permutations = factorial(n);
    
    // Limit thread count based on problem size
    let thread_count = cmp::min(num_threads, total_permutations);
    
    // Calculate base chunk size
    let chunk_size = total_permutations / thread_count;
    let remainder = total_permutations % thread_count;
    
    // Spawn worker threads
    let mut handles = vec![];
    let mut start_idx = 0;
    
    for i in 0..thread_count {
        // Calculate chunk size for this thread (distribute remainder)
        let this_chunk = chunk_size + if i < remainder { 1 } else { 0 };
        
        // Start thread
        let handle = thread::spawn(move || {
            fannkuch_task(n, start_idx, this_chunk)
        });
        
        handles.push(handle);
        start_idx += this_chunk;
    }
    
    // Collect results from all threads
    let mut total_checksum = 0;
    let mut max_flips = 0;
    
    for handle in handles {
        let (checksum, flips) = handle.join().unwrap();
        total_checksum += checksum;
        max_flips = cmp::max(max_flips, flips);
    }
    
    // Print results
    println!("{}", total_checksum);
    println!("Pfannkuchen({}) = {}", n, max_flips);
}
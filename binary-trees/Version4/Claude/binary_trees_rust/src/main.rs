// The Computer Language Benchmarks Game
// https://salsa.debian.org/benchmarksgame-team/benchmarksgame/
//
// Converted from Python to Rust by Claude
// Based on the Python version contributed by Antoine Pitrou
// modified by Dominique Wahli, Daniel Nanz, Joerg Baumann, and Jonathan Ultis

use std::env;
use std::sync::mpsc;
use std::thread;

// Tree node using Rust's Option type for null representation
type TreeNode = Option<Box<(TreeNode, TreeNode)>>;

// Create a binary tree of specified depth
fn make_tree(depth: i32) -> TreeNode {
    if depth > 0 {
        Some(Box::new((make_tree(depth - 1), make_tree(depth - 1))))
    } else {
        None
    }
}

// Check a tree and return node count
fn check_tree(tree: &TreeNode) -> i32 {
    match tree {
        None => 0,
        Some(node) => {
            let (left, right) = &**node;
            1 + check_tree(left) + check_tree(right)
        }
    }
}

// Make and check a single tree
fn make_check(depth: i32) -> i32 {
    check_tree(&make_tree(depth))
}

// Process a chunk of iterations for a specific depth
fn process_chunk(depth: i32, iterations: i32) -> i64 {
    let mut sum: i64 = 0;
    for _ in 0..iterations {
        sum += make_check(depth) as i64;
    }
    sum
}

fn main() {
    // Parse command line arguments
    let args: Vec<String> = env::args().collect();
    let n = args.get(1).and_then(|s| s.parse::<i32>().ok()).unwrap_or(10);
    
    let min_depth = 4;
    let max_depth = std::cmp::max(min_depth + 2, n);
    let stretch_depth = max_depth + 1;
    
    // Determine number of CPU cores
    let num_cpus = num_cpus::get();
    
    // Stretch tree
    {
        let stretch_check = make_check(stretch_depth);
        println!("stretch tree of depth {}\t check: {}", stretch_depth, stretch_check);
    }
    
    // Create long-lived tree
    let long_lived_tree = make_tree(max_depth);
    
    // Process trees of different depths
    let mmd = max_depth + min_depth;
    for depth in (min_depth..stretch_depth).step_by(2) {
        let iterations = 1 << (mmd - depth);
        
        // Prepare for parallel execution
        let chunk_size = if iterations >= num_cpus {
            iterations / num_cpus as i32
        } else {
            iterations
        };
        
        let (tx, rx) = mpsc::channel();
        let mut remaining = iterations;
        let mut threads = 0;
        
        // Spawn worker threads
        while remaining > 0 {
            let this_iterations = std::cmp::min(chunk_size, remaining);
            remaining -= this_iterations;
            
            let thread_tx = tx.clone();
            let thread_depth = depth;
            
            thread::spawn(move || {
                let sum = process_chunk(thread_depth, this_iterations);
                thread_tx.send(sum).unwrap();
            });
            
            threads += 1;
        }
        
        // Collect results
        let mut total: i64 = 0;
        for _ in 0..threads {
            total += rx.recv().unwrap();
        }
        
        println!("{}\t trees of depth {}\t check: {}", iterations, depth, total);
    }
    
    // Check long-lived tree
    println!("long lived tree of depth {}\t check: {}", 
             max_depth, check_tree(&long_lived_tree));
}
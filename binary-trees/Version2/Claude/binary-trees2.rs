// The Computer Language Benchmarks Game
// https://salsa.debian.org/benchmarksgame-team/benchmarksgame/
//
// Converted from Python to Rust by Claude
// Based on the Python version contributed by Antoine Pitrou
// and modified by Dominique Wahli and Heinrich Acker

use std::env;
use std::thread;
use std::sync::{Arc, Mutex};

// Tree node structure - using Option for null representation
type TreeNode = Option<Box<(TreeNode, TreeNode)>>;

// Create a binary tree of specified depth
fn make_tree(depth: usize) -> TreeNode {
    if depth == 0 {
        return None;
    }
    
    // Create left and right subtrees with reduced depth
    Some(Box::new((make_tree(depth - 1), make_tree(depth - 1))))
}

// Count nodes in a binary tree
fn check_tree(node: &TreeNode) -> usize {
    match node {
        None => 0,
        Some(boxed_node) => {
            // Count this node plus all nodes in left and right subtrees
            1 + check_tree(&boxed_node.0) + check_tree(&boxed_node.1)
        }
    }
}

// Process trees of a specific depth with a given number of iterations
fn process_trees(depth: usize, iterations: usize) -> usize {
    let mut check_sum = 0;
    
    for _ in 0..iterations {
        let tree = make_tree(depth);
        check_sum += check_tree(&tree);
        // Tree is automatically freed when it goes out of scope
    }
    
    check_sum
}

fn main() {
    // Parse command line argument
    let args: Vec<String> = env::args().collect();
    if args.len() != 2 {
        eprintln!("Usage: {} <n>", args[0]);
        return;
    }
    
    let n: usize = args[1].parse().expect("Invalid number");
    let min_depth = 4;
    let max_depth = std::cmp::max(min_depth + 2, n);
    let stretch_depth = max_depth + 1;
    
    // Stretch depth tree
    {
        let stretch_tree = make_tree(stretch_depth);
        println!("stretch tree of depth {}\t check: {}", 
                 stretch_depth, check_tree(&stretch_tree));
        // Tree is freed when this block ends
    }
    
    // Create long-lived tree
    let long_lived_tree = make_tree(max_depth);
    
    // Process trees of various depths in parallel
    let num_depths = ((max_depth - min_depth) / 2) + 1;
    let results = Arc::new(Mutex::new(Vec::with_capacity(num_depths)));
    
    // Spawn threads for each depth
    let mut handles = Vec::with_capacity(num_depths);
    
    for i in 0..num_depths {
        let depth = min_depth + i * 2;
        let iterations = 1 << (max_depth - depth + min_depth);
        let results_clone = Arc::clone(&results);
        
        // Create thread to process this depth
        let handle = thread::spawn(move || {
            let check_sum = process_trees(depth, iterations);
            
            // Store results with depth information for later sorting
            let mut results = results_clone.lock().unwrap();
            results.push((depth, iterations, check_sum));
        });
        
        handles.push(handle);
    }
    
    // Wait for all threads to complete
    for handle in handles {
        handle.join().unwrap();
    }
    
    // Print results in order of increasing depth
    let mut results = results.lock().unwrap();
    results.sort_by_key(|&(depth, _, _)| depth);
    
    for &(depth, iterations, check_sum) in results.iter() {
        println!("{}\t trees of depth {}\t check: {}", 
                 iterations, depth, check_sum);
    }
    
    // Check long-lived tree
    println!("long lived tree of depth {}\t check: {}", 
             max_depth, check_tree(&long_lived_tree));
}
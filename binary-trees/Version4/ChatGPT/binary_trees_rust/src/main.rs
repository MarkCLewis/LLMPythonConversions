// The Computer Language Benchmarks Game
// https://salsa.debian.org/benchmarksgame-team/benchmarksgame/
//
// Translated from Python version by ChatGPT
//
// Run with: cargo run --release -- 21

use std::env;
use rayon::prelude::*;

#[derive(Debug)]
struct Node {
    left: Option<Box<Node>>,
    right: Option<Box<Node>>,
}

// Build a binary tree of given depth
fn make_tree(depth: i32) -> Option<Box<Node>> {
    if depth <= 0 {
        Some(Box::new(Node { left: None, right: None }))
    } else {
        Some(Box::new(Node {
            left: make_tree(depth - 1),
            right: make_tree(depth - 1),
        }))
    }
}

// Traverse and check the tree
fn check_tree(node: &Option<Box<Node>>) -> i32 {
    match node {
        None => 1,
        Some(n) => {
            if n.left.is_none() {
                1
            } else {
                1 + check_tree(&n.left) + check_tree(&n.right)
            }
        }
    }
}

// Make a tree and immediately check it
fn make_check(depth: i32) -> i32 {
    check_tree(&make_tree(depth))
}

fn main() {
    let args: Vec<String> = env::args().collect();
    if args.len() < 2 {
        eprintln!("Usage: {} <max_depth>", args[0]);
        return;
    }

    let n: i32 = args[1].parse().unwrap();
    let min_depth = 4;
    let max_depth = if n > min_depth + 2 { n } else { min_depth + 2 };
    let stretch_depth = max_depth + 1;

    // Stretch tree
    let stretch_tree = make_tree(stretch_depth);
    println!(
        "stretch tree of depth {}\t check: {}",
        stretch_depth,
        check_tree(&stretch_tree)
    );

    // Long-lived tree
    let long_lived_tree = make_tree(max_depth);

    let mmd = max_depth + min_depth;

    for depth in (min_depth..stretch_depth).step_by(2) {
        let iterations = 1 << (mmd - depth);

        // Parallelize tree creation/checking
        let check: i64 = (0..iterations)
            .into_par_iter()
            .map(|_| make_check(depth) as i64)
            .sum();

        println!(
            "{}\t trees of depth {}\t check: {}",
            iterations, depth, check
        );
    }

    println!(
        "long lived tree of depth {}\t check: {}",
        max_depth,
        check_tree(&long_lived_tree)
    );
}

// The Computer Language Benchmarks Game
// https://salsa.debian.org/benchmarksgame-team/benchmarksgame/
//
// Translated from Python version by ChatGPT

use std::env;

#[derive(Debug)]
struct Node {
    left: Option<Box<Node>>,
    right: Option<Box<Node>>,
}

// Create a binary tree of given depth
fn make_tree(depth: i32) -> Option<Box<Node>> {
    if depth <= 0 {
        None
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
        Some(n) => 1 + check_tree(&n.left) + check_tree(&n.right),
    }
}

fn main() {
    let args: Vec<String> = env::args().collect();
    if args.len() < 2 {
        eprintln!("Usage: {} <max_depth>", args[0]);
        return;
    }

    let min_depth = 4;
    let mut max_depth: i32 = args[1].parse().unwrap();
    if max_depth < min_depth + 2 {
        max_depth = min_depth + 2;
    }
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

    let mut iterations = 1 << max_depth;
    let mut depth = min_depth;
    while depth <= stretch_depth {
        let mut check: i64 = 0;
        for _ in 1..=iterations {
            let tree = make_tree(depth);
            check += check_tree(&tree) as i64;
        }
        println!("{}\t trees of depth {}\t check: {}", iterations, depth, check);
        iterations >>= 2;
        depth += 2;
    }

    println!(
        "long lived tree of depth {}\t check: {}",
        max_depth,
        check_tree(&long_lived_tree)
    );
}

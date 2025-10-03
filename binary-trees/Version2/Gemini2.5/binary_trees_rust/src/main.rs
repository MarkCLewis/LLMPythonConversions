use rayon::prelude::*;
use std::cmp::max;
use std::env;

// A tree node contains optional left and right children.
// Box<T> is a smart pointer for heap allocation, necessary for recursive structs.
struct TreeNode {
    left: Option<Box<TreeNode>>,
    right: Option<Box<TreeNode>>,
}

// Recursively builds a binary tree of a given depth.
// A tree of depth 0 is a single leaf node (with no children).
// A tree of depth > 0 has a left and a right subtree of depth - 1.
fn make_tree(depth: i32) -> Box<TreeNode> {
    if depth > 0 {
        Box::new(TreeNode {
            left: Some(make_tree(depth - 1)),
            right: Some(make_tree(depth - 1)),
        })
    } else {
        Box::new(TreeNode { left: None, right: None })
    }
}

// Recursively walks the tree and computes a checksum.
fn check_tree(node: &TreeNode) -> i32 {
    // If 'left' is Some, it's an inner node. If it's None, it's a leaf.
    if let Some(left_node) = &node.left {
        // We assume if left exists, right does too, as per make_tree's logic.
        let right_node = node.right.as_ref().unwrap();
        1 + check_tree(left_node) + check_tree(right_node)
    } else {
        // Leaf node
        1
    }
}

fn main() {
    // Read the tree depth from the command-line arguments. Default to 10.
    let n = env::args_os()
        .nth(1)
        .and_then(|s| s.into_string().ok())
        .and_then(|s| s.parse().ok())
        .unwrap_or(10);

    let min_depth = 4;
    let max_depth = max(min_depth + 2, n);
    let stretch_depth = max_depth + 1;

    // 1. Create a "stretch" tree of great depth to check memory allocation.
    let stretch_tree = make_tree(stretch_depth);
    println!(
        "stretch tree of depth {}\t check: {}",
        stretch_depth,
        check_tree(&stretch_tree)
    );
    // The stretch_tree is dropped here, freeing its memory.

    // 2. Create a long-lived tree that will stay in memory until the end.
    let long_lived_tree = make_tree(max_depth);

    // 3. Create, check, and deallocate many smaller trees in a loop.
    // This loop is the core of the benchmark.
    for depth in (min_depth..=max_depth).step_by(2) {
        let iterations = 1 << (max_depth - depth + min_depth);

        // This is the performance-critical section.
        // We use rayon's parallel iterator (.into_par_iter()) to distribute
        // the work across all available CPU cores.
        let check: i32 = (0..iterations)
            .into_par_iter()
            .map(|_| check_tree(&make_tree(depth)))
            .sum();

        println!("{}\t trees of depth {}\t check: {}", iterations, depth, check);
    }

    // 4. Finally, check the long-lived tree that was allocated at the start.
    println!(
        "long lived tree of depth {}\t check: {}",
        max_depth,
        check_tree(&long_lived_tree)
    );
}

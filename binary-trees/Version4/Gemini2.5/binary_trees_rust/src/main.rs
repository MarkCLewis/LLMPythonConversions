use rayon::prelude::*;
use std::cmp::max;
use std::env;

// A simple binary tree structure.
// 'Option<Box<TreeNode>>' is the idiomatic way to represent a nullable,
// heap-allocated child node in Rust. 'Box' is a smart pointer for heap allocation.
struct TreeNode {
    left: Option<Box<TreeNode>>,
    right: Option<Box<TreeNode>>,
}

// Builds a perfect binary tree of a given depth.
fn make_tree(depth: i32) -> Box<TreeNode> {
    if depth > 0 {
        Box::new(TreeNode {
            left: Some(make_tree(depth - 1)),
            right: Some(make_tree(depth - 1)),
        })
    } else {
        Box::new(TreeNode {
            left: None,
            right: None,
        })
    }
}

// Recursively traverses the tree and counts all nodes.
fn check_tree(node: &TreeNode) -> i32 {
    // The 'match' statement checks if the children exist.
    // If they do (Some(...)), it recurses. Otherwise, it's a leaf.
    match (&node.left, &node.right) {
        (Some(left_box), Some(right_box)) => {
            // Dereferencing the Box smart pointer to get a reference to the TreeNode
            1 + check_tree(&left_box) + check_tree(&right_box)
        }
        _ => 1, // Leaf node
    }
}

// A helper function that combines making and checking a tree.
fn make_check(depth: i32) -> i32 {
    check_tree(&make_tree(depth))
}

fn main() {
    // Read the command-line argument for 'n', defaulting to 10 if not provided or invalid.
    let n: i32 = env::args()
        .nth(1)
        .and_then(|s| s.parse().ok())
        .unwrap_or(10);

    let min_depth = 4;
    let max_depth = max(min_depth + 2, n);

    // 1. Stretch Tree: create, check, and discard one very deep tree.
    let stretch_depth = max_depth + 1;
    let check = make_check(stretch_depth);
    println!("stretch tree of depth {}\t check: {}", stretch_depth, check);

    // 2. Long-Lived Tree: create one tree that lives for the duration of the program.
    let long_lived_tree = make_tree(max_depth);

    // 3. Main Loop: create and check many trees of varying depths in parallel.
    let mmd = max_depth + min_depth;
    for depth in (min_depth..=max_depth).step_by(2) {
        let iterations = 1 << (mmd - depth); // Same as 2^(mmd - depth)
        
        // Use rayon to parallelize the work.
        // `into_par_iter()` creates a parallel iterator.
        // `map()` applies the `make_check` function to each item in parallel.
        // `sum()` aggregates the results.
        let check_sum: i32 = (0..iterations)
            .into_par_iter()
            .map(|_| make_check(depth))
            .sum();

        println!("{}\t trees of depth {}\t check: {}", iterations, depth, check_sum);
    }

    // 4. Final Check: check the long-lived tree that was created earlier.
    println!(
        "long lived tree of depth {}\t check: {}",
        max_depth,
        check_tree(&long_lived_tree)
    );
}

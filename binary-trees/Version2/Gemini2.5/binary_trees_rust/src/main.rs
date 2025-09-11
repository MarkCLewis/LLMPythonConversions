use rayon::prelude::*;
use std::env;

// An explicit tree structure is more idiomatic in Rust.
// `Box` is used for heap allocation, necessary for recursive types.
// `Option` allows children to be absent (for leaf nodes).
struct TreeNode {
    left: Option<Box<TreeNode>>,
    right: Option<Box<TreeNode>>,
}

/// Recursively builds a binary tree of a given depth.
fn make_tree(depth: i32) -> TreeNode {
    if depth > 0 {
        TreeNode {
            left: Some(Box::new(make_tree(depth - 1))),
            right: Some(Box::new(make_tree(depth - 1))),
        }
    } else {
        TreeNode {
            left: None,
            right: None,
        }
    }
}

/// Recursively traverses the tree and returns a checksum (the count of all nodes).
fn check_tree(node: &TreeNode) -> i32 {
    // Match on the left child to determine if it's a leaf node.
    match &node.left {
        Some(left_node) => {
            // It's not a leaf, so add 1 for the current node
            // and recurse into the left and right children.
            // The right node is guaranteed to exist if the left one does.
            1 + check_tree(left_node) + check_tree(node.right.as_ref().unwrap())
        }
        // If there's no left child, it's a leaf node.
        None => 1,
    }
}

fn main() {
    // Read the max_depth from command-line arguments, default to 10.
    let n = env::args_os()
        .nth(1)
        .and_then(|s| s.into_string().ok())
        .and_then(|s| s.parse().ok())
        .unwrap_or(10);

    let min_depth = 4;
    let max_depth = n.max(min_depth + 2);

    // 1. Stretch Tree: Create a tree one level deeper than max_depth.
    let stretch_depth = max_depth + 1;
    let stretch_tree = make_tree(stretch_depth);
    println!(
        "stretch tree of depth {}\t check: {}",
        stretch_depth,
        check_tree(&stretch_tree)
    );

    // 2. Long-lived tree: This tree is created once and kept in memory.
    let long_lived_tree = make_tree(max_depth);

    // 3. Main loop: Create and check many trees of varying depths.
    for depth in (min_depth..=max_depth).step_by(2) {
        let iterations = 1 << (max_depth - depth + min_depth);

        // This is the most performance-critical part.
        // We use rayon's `into_par_iter()` to create and check trees in parallel.
        // The `map` operation runs on multiple CPU cores simultaneously.
        // The `sum()` at the end aggregates the results from all threads.
        let check: i32 = (0..iterations)
            .into_par_iter()
            .map(|_| check_tree(&make_tree(depth)))
            .sum();

        println!("{}\t trees of depth {}\t check: {}", iterations, depth, check);
    }

    // 4. Final check of the long-lived tree.
    println!(
        "long lived tree of depth {}\t check: {}",
        max_depth,
        check_tree(&long_lived_tree)
    );
}

use rayon::prelude::*;
use std::cmp::max;
use std::env;

// A binary tree node.
// `Box` is used for heap allocation, allowing recursive types.
// `Option` allows for a child to be absent (i.e., `None`).
struct Tree {
    left: Option<Box<Tree>>,
    right: Option<Box<Tree>>,
}

/// Recursively builds a binary tree of a given depth.
fn make_tree(depth: i32) -> Tree {
    if depth > 0 {
        let d = depth - 1;
        Tree {
            left: Some(Box::new(make_tree(d))),
            right: Some(Box::new(make_tree(d))),
        }
    } else {
        // At depth 0, create a leaf node with no children.
        Tree { left: None, right: None }
    }
}

/// Recursively traverses the tree and counts all nodes.
fn check_tree(node: &Tree) -> i32 {
    // Check if there's a left child. In this problem, trees are symmetric,
    // so if there's a left child, there's also a right one.
    if let Some(left) = &node.left {
        // The unwrap() is safe because we know the tree is symmetric.
        let right = node.right.as_ref().unwrap();
        // Count this node + all nodes in the left and right subtrees.
        1 + check_tree(left) + check_tree(right)
    } else {
        // This is a leaf node.
        1
    }
}

/// A helper function that builds a tree and immediately checks it.
fn make_check(depth: i32) -> i32 {
    let tree = make_tree(depth);
    check_tree(&tree)
}

fn main() {
    // Read the command-line argument 'n', defaulting to 10 if not provided or invalid.
    let n = env::args_os()
        .nth(1)
        .and_then(|s| s.into_string().ok())
        .and_then(|s| s.parse().ok())
        .unwrap_or(10);

    // Define the minimum and maximum depths for the trees.
    let min_depth = 4;
    let max_depth = max(min_depth + 2, n);
    let stretch_depth = max_depth + 1;

    // 1. Create a "stretch" tree, check it, and print the result.
    // This is a single, deep tree to test memory allocation.
    let check = make_check(stretch_depth);
    println!("stretch tree of depth {}\t check: {}", stretch_depth, check);

    // 2. Create a long-lived tree that will be checked at the end.
    let long_lived_tree = make_tree(max_depth);

    // 3. Create and check a forest of smaller trees in parallel.
    let mmd = max_depth + min_depth;
    // Iterate from min_depth up to max_depth, stepping by 2.
    for depth in (min_depth..stretch_depth).step_by(2) {
        // Calculate the number of trees to create for this depth.
        let iterations = 1 << (mmd - depth);

        // Use Rayon's `par_iter` to perform the checks in parallel.
        let check_sum = (0..iterations)
            .into_par_iter()
            .map(|_| make_check(depth))
            .sum::<i32>();

        println!("{}\t trees of depth {}\t check: {}", iterations, depth, check_sum);
    }

    // 4. Check the long-lived tree and print the final result.
    println!(
        "long lived tree of depth {}\t check: {}",
        max_depth,
        check_tree(&long_lived_tree)
    );
}

use std::env;

/// The core fannkuch-redux algorithm, translated from Python.
fn fannkuch(n: usize) -> i32 {
    // Basic range checks. The original Python code would error on n < 3.
    if n < 1 {
        println!("0");
        return 0;
    }

    let mut max_flips_count = 0;
    let mut perm_sign = true;
    let mut checksum = 0;

    // A mutable vector for the current permutation.
    let mut perm1: Vec<usize> = (0..n).collect();
    // State for the permutation generation algorithm.
    let mut count: Vec<usize> = perm1.clone();
    let nm = n - 1;

    loop {
        // --- Flip Calculation ---
        // If the first element `k` is not 0, we need to perform flips.
        let k = perm1[0];
        if k != 0 {
            // Create a mutable copy to perform the flips on.
            let mut perm = perm1.clone();
            let mut flips_count = 1;
            
            // This loop reverses sub-slices of the permutation until the
            // element at index 0 is 0.
            let mut current_k = k;
            loop {
                // The value at the current index `k` tells us the index of
                // the next element to process. If it's 0, we are done.
                let next_k = perm[current_k];
                if next_k == 0 {
                    break;
                }
                
                // Rust's `reverse()` on a slice is a direct and efficient
                // equivalent of Python's `perm[:k+1] = perm[k::-1]`.
                perm[..=current_k].reverse();
                flips_count += 1;
                current_k = next_k;
            }

            // Update the max flips and checksum.
            if max_flips_count < flips_count {
                max_flips_count = flips_count;
            }
            checksum += if perm_sign { flips_count } else { -flips_count };
        }

        // --- Permutation Generation ---
        // This logic uses a simplified version of the Steinhaus-Johnson-Trotter
        // algorithm to generate the next permutation in the sequence.
        if perm_sign {
            perm1.swap(0, 1);
            perm_sign = false;
        } else {
            perm1.swap(1, 2);
            perm_sign = true;

            let mut r = 0;
            let mut found_r = false;
            // Corresponds to Python's `for r in range(2, n - 1)`
            for i in 2..nm {
                if count[i] > 0 {
                    r = i;
                    found_r = true;
                    break;
                }
                count[i] = i;
                // This rotation is equivalent to the three-line slice
                // manipulation in the original Python code.
                perm1[0..=i].rotate_left(1);
            }

            // This block simulates Python's `for...else` construct.
            // It runs only if the `for` loop completed without a `break`.
            if !found_r {
                r = nm;
                if count[r] == 0 {
                    println!("{}", checksum);
                    return max_flips_count;
                }
            }
            count[r] -= 1;
        }
    }
}

fn main() {
    // Get the input 'n' from command-line arguments.
    let n: usize = env::args()
        .nth(1)
        .expect("Usage: fannkuch-redux <n>")
        .parse()
        .expect("Input must be a positive integer.");

    let max_flips = fannkuch(n);
    println!("Pfannkuchen({}) = {}", n, max_flips);
}

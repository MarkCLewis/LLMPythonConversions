use rayon::prelude::*;
use std::env;

// Factorial table for n up to 20
static FACTORIAL_TABLE: [usize; 21] = [
    1,
    1,
    2,
    6,
    24,
    120,
    720,
    5040,
    40320,
    362880,
    3628800,
    39916800,
    479001600,
    6227020800,
    87178291200,
    1307674368000,
    20922789888000,
    355687428096000,
    6402373705728000,
    121645100408832000,
    2432902008176640000,
];

/// Counts the number of flips required to bring the first element to the front.
fn count_flips(p: &mut [u8], n: usize) -> usize {
    let mut flips = 0;
    let mut first = p[0];

    while first != 0 {
        let k = first as usize;
        // Reverse the sub-slice from the beginning to the element `k`.
        p[..=k].reverse();
        flips += 1;
        first = p[0];
    }
    flips
}

/// Processes a chunk of permutations and returns the partial checksum and max flips.
fn task(n: usize, start_idx: usize, size: usize) -> (i64, usize) {
    let mut p: Vec<u8> = (0..n as u8).collect();
    let mut count = vec![0; n];

    // --- Initialization: Jump to the start_idx permutation using factoradic ---
    let mut remainder = start_idx;
    for i in (1..n).rev() {
        let f = FACTORIAL_TABLE[i];
        let (q, r) = (remainder / f, remainder % f);
        count[i] = q;
        remainder = r;
        if q > 0 {
            // This is equivalent to Python's multiple left rotations.
            p[..=i].rotate_left(q);
        }
    }

    let mut checksum = 0i64;
    let mut max_flips = 0usize;
    let mut p_temp = vec![0; n];

    // The algorithm generates permutations in pairs.
    // 1. The "base" permutation `p`.
    // 2. The same permutation with the first two elements swapped.
    let num_pairs = size / 2;
    for i in 0..num_pairs {
        // --- Process 1st permutation of the pair ---
        p_temp.copy_from_slice(&p);
        let flips1 = count_flips(&mut p_temp, n);
        max_flips = max_flips.max(flips1);
        checksum += flips1 as i64;

        // --- Process 2nd permutation of the pair ---
        p.swap(0, 1);
        p_temp.copy_from_slice(&p);
        let flips2 = count_flips(&mut p_temp, n);
        max_flips = max_flips.max(flips2);
        checksum -= flips2 as i64;

        // **THE FIX**: Only advance state if it's NOT the last iteration.
        // The panic happens when trying to advance past the final permutation.
        if i < num_pairs - 1 {
            // --- Advance state to the next base permutation ---
            let mut i = 2;
            while count[i] >= i {
                count[i] = 0;
                i += 1;
            }
            count[i] += 1;
            p[..=i].rotate_left(1);
        }
    }

    (checksum, max_flips)
}

/// Main function to orchestrate the fannkuch-redux benchmark.
fn fannkuch(n: usize) {
    if n == 0 {
        println!("0\nPfannkuchen(0) = 0");
        return;
    }
    if n > 20 {
        println!("Error: n cannot be greater than 20 due to factorial limits.");
        return;
    }
    
    let total_perms = FACTORIAL_TABLE[n];
    let task_count = num_cpus::get();
    
    // Calculate chunk size, ensuring it's even as per the original algorithm's logic.
    let mut chunk_size = (total_perms + task_count - 1) / task_count;
    if chunk_size % 2 != 0 {
        chunk_size += 1;
    }

    // A performance heuristic from the original Python code.
    let (chunk_size, task_count) = if chunk_size < 20_000 {
        (total_perms, 1)
    } else {
        (total_perms / chunk_size, (total_perms + chunk_size -1) / chunk_size)
    };

    // Create a list of tasks (chunks) to be processed.
    let chunks: Vec<(usize, usize)> = (0..task_count)
        .map(|i| (i * chunk_size, chunk_size.min(total_perms - i * chunk_size)))
        .collect();

    // Process chunks in parallel using rayon.
    let (total_checksum, max_total_flips) = chunks
        .into_par_iter()
        .map(|(start, size)| task(n, start, size))
        .reduce(
            || (0, 0),
            |(checksum_a, max_a), (checksum_b, max_b)| {
                (checksum_a + checksum_b, max_a.max(max_b))
            },
        );

    println!("{}", total_checksum);
    println!("Pfannkuchen({}) = {}", n, max_total_flips);
}

fn main() {
    let n = env::args()
        .nth(1)
        .expect("Please provide a number n as a command-line argument.")
        .parse()
        .expect("Argument must be a valid integer.");

    fannkuch(n);
}

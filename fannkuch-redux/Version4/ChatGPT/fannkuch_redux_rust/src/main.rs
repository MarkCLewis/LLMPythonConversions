use rayon::prelude::*;
use std::env;

#[inline]
fn factorials(n: usize) -> Vec<u64> {
    let mut f = vec![1u64; n + 1];
    for i in 1..=n {
        f[i] = f[i - 1] * i as u64;
    }
    f
}

/// Initialize permutation `perm` from factoradic index `idx`.
/// Also fills `count` for the permutation generator and returns the starting sign (+1/-1).
#[inline]
fn init_perm_from_index(n: usize, mut idx: u64, perm: &mut [u8], count: &mut [u8]) -> i32 {
    for i in 0..n {
        perm[i] = i as u8;
    }

    // Build permutation by rotating the prefix per factoradic digit
    // Track parity: rotating by an odd distance flips sign; even keeps it.
    let mut sign = 1i32;
    for i in (1..n).rev() {
        let d = (idx % (i as u64 + 1)) as usize;
        idx /= i as u64 + 1;
        count[i] = d as u8;

        if d != 0 {
            // rotate left perm[0..=i] by d
            if d & 1 == 1 {
                sign = -sign;
            } else {
                sign = sign;
            }
            // small fixed-size rotate
            let mut tmp = [0u8; 32]; // n ≤ 20 in the benchmark
            tmp[..=i].copy_from_slice(&perm[..=i]);
            for k in 0..=i {
                perm[k] = tmp[(k + d) % (i + 1)];
            }
        }
    }
    count[0] = 0;
    sign
}

/// Count flips for current permutation.
#[inline]
fn flips_count(perm: &[u8]) -> i32 {
    let first = perm[0];
    if first == 0 {
        return 0;
    }
    let n = perm.len();
    let mut p = [0u8; 32];
    p[..n].copy_from_slice(&perm[..n]);

    let mut flips = 0i32;
    while p[0] != 0 {
        let f = p[0] as usize;
        // reverse p[0..=f]
        let mut lo = 0;
        let mut hi = f;
        while lo < hi {
            p.swap(lo, hi);
            lo += 1;
            hi -= 1;
        }
        flips += 1;
    }
    flips
}

/// Advance to next permutation and update sign (alternates).
#[inline]
fn next_permutation(n: usize, perm: &mut [u8], count: &mut [u8], sign: &mut i32) {
    if *sign == 1 {
        perm.swap(0, 1);
        *sign = -1;
        return;
    }

    // rotate first three
    let t = perm[1];
    perm[1] = perm[2];
    perm[2] = t;
    *sign = 1;

    let mut i = 2usize;
    while i < n {
        if count[i] as usize >= i {
            // restore identity for this prefix and carry
            count[i] = 0;
            // move perm[0] to position i (rotate left by 1)
            let t0 = perm[0];
            for k in 0..i {
                perm[k] = perm[k + 1];
            }
            perm[i] = t0;
            i += 1;
        } else {
            // rotate left prefix (i+1) by 1 and increment counter
            let t0 = perm[0];
            for k in 0..i {
                perm[k] = perm[k + 1];
            }
            perm[i] = t0;
            count[i] += 1;
            break;
        }
    }
}

/// Process a chunk of permutations [start, start+size)
#[inline]
fn process_chunk(n: usize, start: u64, size: u64) -> (i64, i32) {
    let mut perm = [0u8; 32];
    let mut count = [0u8; 32];
    let mut sign = init_perm_from_index(n, start, &mut perm, &mut count);

    let mut local_max = 0i32;
    let mut checksum = 0i64;

    for _ in 0..size {
        let flips = flips_count(&perm[..n]);
        if flips > local_max {
            local_max = flips;
        }
        checksum += if sign > 0 { flips as i64 } else { -(flips as i64) };

        next_permutation(n, &mut perm, &mut count, &mut sign);
    }

    (checksum, local_max)
}

fn main() {
    // Read n
    let n: usize = env::args()
        .nth(1)
        .and_then(|s| s.parse().ok())
        .unwrap_or(7);

    assert!(n >= 1 && n <= 20, "n must be in 1..20 for benchmark");

    // Precompute factorials and total permutations
    let fact = factorials(n);
    let total = fact[n];

    // Choose chunking similar to the Python version:
    // use (#threads) chunks when large enough; otherwise single chunk to reduce overheads.
    let threads = rayon::current_num_threads() as u64;
    let mut chunk = (total + threads - 1) / threads;
    if chunk < 20_000 {
        chunk = total;
    }

    // Parallel over chunk start indices
    let mut starts: Vec<u64> = (0..total).step_by(chunk as usize).collect();

    // Ensure last chunk is bounded
    let results: Vec<(i64, i32)> = starts
        .par_iter()
        .map(|&s| {
            let size = if s + chunk > total { total - s } else { chunk };
            process_chunk(n, s, size)
        })
        .collect();

    let (checksum, maxflips) = results.into_iter().fold((0i64, 0i32), |acc, r| {
        (acc.0 + r.0, acc.1.max(r.1))
    });

    // Output format compatible with Benchmarks Game
    println!("{}", checksum);
    println!("Pfannkuchen({}) = {}", n, maxflips);
}

use std::env;

/// Reverse prefix perm[0..=k]
#[inline]
fn reverse_prefix(perm: &mut [usize], mut k: usize) {
    let mut i = 0usize;
    while i < k {
        perm.swap(i, k);
        i += 1;
        if k == 0 { break; }
        k -= 1;
    }
}

/// Compute fannkuch for n; returns (checksum, max_flips)
fn fannkuch(n: usize) -> (i64, i32) {
    // perm1: current permutation in generator order
    // count: rotation counters for each prefix
    let mut perm1: Vec<usize> = (0..n).collect();
    let mut count: Vec<i32> = (0..n as i32).collect();

    let mut perm: Vec<usize> = vec![0; n];

    let mut perm_sign = true; // True/False like Python
    let mut max_flips = 0i32;
    let mut checksum = 0i64;
    let nm = n - 1;

    loop {
        // Work (flip counting)
        let k0 = perm1[0];
        if k0 != 0 {
            perm.copy_from_slice(&perm1);
            let mut flips = 1i32;
            let mut k = k0;
            let mut kk = perm[k];
            while kk != 0 {
                reverse_prefix(&mut perm, k);
                flips += 1;
                k = kk;
                kk = perm[kk];
            }
            if flips > max_flips {
                max_flips = flips;
            }
            checksum += if perm_sign { flips as i64 } else { -(flips as i64) };
        }

        // Next permutation (incremental change, matches fannkuch-redux6.py)
        if perm_sign {
            perm1.swap(0, 1);
            perm_sign = false;
        } else {
            perm1.swap(1, 2);
            perm_sign = true;

            let mut r = 2usize;
            let mut broke = false;
            while r < n - 1 {
                if count[r] != 0 {
                    broke = true;
                    break;
                }
                count[r] = r as i32;
                // rotate-left perm1[0..=r] by 1
                let p0 = perm1[0];
                perm1.copy_within(1..=r, 0);
                perm1[r] = p0;
                r += 1;
            }
            if !broke {
                r = nm;
                if count[r] == 0 {
                    // Done
                    return (checksum, max_flips);
                }
            }
            count[r] -= 1;
        }
    }
}

fn main() {
    let n: usize = env::args()
        .nth(1)
        .and_then(|s| s.parse().ok())
        .unwrap_or(7);

    let (checksum, max_flips) = fannkuch(n);
    println!("{}", checksum);
    println!("Pfannkuchen({}) = {}", n, max_flips);
}

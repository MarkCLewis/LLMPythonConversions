use rayon::prelude::*;
use std::env;

/// A[i,j] = 1.0 / ( (i+j)*(i+j+1)/2 + i + 1 )
#[inline]
fn a_den(i: usize, j: usize) -> f64 {
    let ij = (i + j) as u64;
    ((ij * (ij + 1)) / 2 + i as u64 + 1) as f64
}

#[inline]
fn a_times_u(u: &[f64], out: &mut [f64]) {
    let n = u.len();
    out.par_iter_mut().enumerate().for_each(|(i, oi)| {
        let mut sum = 0.0f64;
        for j in 0..n {
            sum += u[j] / a_den(i, j);
        }
        *oi = sum;
    });
}

#[inline]
fn at_times_u(u: &[f64], out: &mut [f64]) {
    let n = u.len();
    out.par_iter_mut().enumerate().for_each(|(i, oi)| {
        let mut sum = 0.0f64;
        for j in 0..n {
            sum += u[j] / a_den(j, i);
        }
        *oi = sum;
    });
}

#[inline]
fn at_a_times_u(u: &[f64], out: &mut [f64], tmp: &mut [f64]) {
    a_times_u(u, tmp);     // tmp = A * u
    at_times_u(tmp, out);  // out = A^T * tmp
}

fn main() {
    let n: usize = env::args()
        .nth(1)
        .and_then(|s| s.parse().ok())
        .unwrap_or(5500);

    let mut u = vec![1.0f64; n];
    let mut v = vec![0.0f64; n];
    let mut tmp = vec![0.0f64; n];

    // 10 power iterations: v = (A^T A) u; u = (A^T A) v
    for _ in 0..10 {
        at_a_times_u(&u, &mut v, &mut tmp);
        at_a_times_u(&v, &mut u, &mut tmp);
    }

    // Rayleigh quotient sqrt((u·v)/(v·v))
    let (v_b_v, v_v) = u
        .par_iter()
        .zip(v.par_iter())
        .map(|(ui, vi)| (ui * vi, vi * vi))
        .reduce(|| (0.0, 0.0), |(a1, a2), (b1, b2)| (a1 + b1, a2 + b2));

    println!("{:.9}", (v_b_v / v_v).sqrt());
}

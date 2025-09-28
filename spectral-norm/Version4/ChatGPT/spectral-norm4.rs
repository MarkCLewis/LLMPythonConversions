use rayon::prelude::*;
use std::env;

/// Denominator used in the benchmark matrix:
/// A[i,j] = 1.0 / ( (i+j)*(i+j+1)/2 + i + 1 )
#[inline]
fn a_den(i: usize, j: usize) -> f64 {
    let ij = (i + j) as u64;
    ((ij * (ij + 1)) / 2 + i as u64 + 1) as f64
}

#[inline]
fn multiply_av(v: &[f64], out: &mut [f64]) {
    let n = v.len();
    out.par_iter_mut()
        .enumerate()
        .for_each(|(i, out_i)| {
            let mut sum = 0.0f64;
            // tight inner loop; compiler vectorizes nicely in --release
            for j in 0..n {
                sum += v[j] / a_den(i, j);
            }
            *out_i = sum;
        });
}

#[inline]
fn multiply_atv(v: &[f64], out: &mut [f64]) {
    let n = v.len();
    out.par_iter_mut()
        .enumerate()
        .for_each(|(i, out_i)| {
            let mut sum = 0.0f64;
            for j in 0..n {
                sum += v[j] / a_den(j, i);
            }
            *out_i = sum;
        });
}

#[inline]
fn multiply_atav(v: &[f64], out: &mut [f64], tmp: &mut [f64]) {
    multiply_av(v, tmp);      // tmp = A * v
    multiply_atv(tmp, out);   // out = A^T * tmp
}

fn main() {
    // n (matrix dimension / vector length)
    let n: usize = env::args()
        .nth(1)
        .and_then(|s| s.parse().ok())
        .unwrap_or(5500);

    let mut u = vec![1.0f64; n];
    let mut v = vec![0.0f64; n];
    let mut tmp = vec![0.0f64; n];

    // 10 power iterations of (A^T A)
    for _ in 0..10 {
        multiply_atav(&u, &mut v, &mut tmp);
        multiply_atav(&v, &mut u, &mut tmp);
    }

    // Rayleigh quotient sqrt((u·v) / (v·v))
    let (v_b_v, v_v) = u.par_iter()
        .zip(v.par_iter())
        .map(|(ui, vi)| (ui * vi, vi * vi))
        .reduce(|| (0.0, 0.0), |(a1, a2), (b1, b2)| (a1 + b1, a2 + b2));

    let result = (v_b_v / v_v).sqrt();
    println!("{:.9}", result);
}

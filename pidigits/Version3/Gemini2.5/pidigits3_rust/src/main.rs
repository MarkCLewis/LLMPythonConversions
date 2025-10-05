//! The Computer Language Benchmarks Game
//! https://salsa.debian.org/benchmarksgame-team/benchmarksgame/
//!
//! Translated from Jeremy Zerfas's Python program.

use rug::Integer;
use std::env;

/// Calculates floor((num * nth + acc) / den)
fn extract_digit(nth: u32, num: &Integer, acc: &Integer, den: &Integer) -> u32 {
    let mut tmp = Integer::from(num * nth);
    tmp += acc;
    tmp /= den;
    tmp.to_u32().unwrap()
}

/// Eliminates a digit's contribution to bring the stream to the next digit.
fn eliminate_digit(d: u32, num: &mut Integer, acc: &mut Integer, den: &Integer) {
    *acc -= den * d;
    *acc *= 10;
    *num *= 10;
}

/// Produces the next term in the series.
fn next_term(k: u32, num: &mut Integer, acc: &mut Integer, den: &mut Integer) {
    let k2 = k * 2 + 1;

    // FIX: Reborrow `num` as an immutable reference for the multiplication.
    *acc += &*num * 2;
    *acc *= k2;
    *den *= k2;
    *num *= k;
}

fn main() {
    let n: u32 = env::args_os()
        .nth(1)
        .and_then(|s| s.into_string().ok())
        .and_then(|s| s.parse::<u32>().ok())
        .expect("Please provide the number of digits to generate.");

    let mut acc = Integer::from(0);
    let mut den = Integer::from(1);
    let mut num = Integer::from(1);

    let mut i: u32 = 0;
    let mut k: u32 = 0;

    while i < n {
        k += 1;
        next_term(k, &mut num, &mut acc, &mut den);

        if num > acc {
            continue;
        }

        let d = extract_digit(3, &num, &acc, &den);
        if d != extract_digit(4, &num, &acc, &den) {
            continue;
        }

        print!("{}", (b'0' + d as u8) as char);
        i += 1;

        if i % 10 == 0 {
            println!("\t:{}", i);
        }

        eliminate_digit(d, &mut num, &mut acc, &den);
    }
}

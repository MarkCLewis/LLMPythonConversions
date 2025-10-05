// The Computer Language Benchmarks Game
// https://salsa.debian.org/benchmarksgame-team/benchmarksgame/
//
// Translated from Mr Ledrug's C program by Jeremy Zerfas.
// Transliterated from GMP to built-in by Isaac Gouy.
// Translated from Python to Rust by Gemini.

use num::bigint::BigInt;
use num::traits::{One, Zero};
use std::env;
use std::io::{self, Write};

/// Advances the state of the spigot algorithm to the next term.
fn next_term(k: i32, acc: &mut BigInt, num: &mut BigInt, den: &mut BigInt) {
    let k2 = (k as i64) * 2 + 1;
    // Apply the fix here by reborrowing `num`
    *acc += &*num * 2_i64;
    *acc *= k2;
    *den *= k2;
    *num *= k as i64;
}

/// Extracts a potential digit from the current state.
fn extract_digit(nth: i64, num: &BigInt, acc: &BigInt, den: &BigInt) -> BigInt {
    (num * nth + acc) / den
}

/// "Consumes" a digit `d` from the state, preparing for the next one.
fn eliminate_digit(d: &BigInt, acc: &mut BigInt, num: &mut BigInt, den: &BigInt) {
    *acc -= den * d;
    *acc *= 10_i64;
    *num *= 10_i64;
}

fn main() {
    // 1. Get the number of digits `n` from command line arguments.
    let n: i32 = env::args()
        .nth(1)
        .expect("Usage: pidigits <number_of_digits>")
        .parse()
        .expect("The argument must be a valid integer.");

    let mut stdout = io::stdout();

    // 2. Initialize the state variables for the algorithm.
    let mut acc: BigInt = Zero::zero();
    let mut den: BigInt = One::one();
    let mut num: BigInt = One::one();

    let mut i = 0; // Digits generated so far
    let mut k = 0; // Term counter

    // 3. Main loop to generate `n` digits.
    while i < n {
        k += 1;
        next_term(k, &mut acc, &mut num, &mut den);

        // Wait until the state is ready for digit extraction.
        if num > acc {
            continue;
        }

        // 4. Check if the next digit is "safe" to produce.
        // The algorithm is safe when the digit extracted with n=3 is the same as n=4.
        let d = extract_digit(3, &num, &acc, &den);
        if d != extract_digit(4, &num, &acc, &den) {
            continue;
        }

        // 5. The digit is safe. Print it and update the state.
        write!(stdout, "{}", d).unwrap();
        i += 1;

        // Format output with a newline and count every 10 digits.
        if i % 10 == 0 {
            writeln!(stdout, "\t:{}", i).unwrap();
        }

        eliminate_digit(&d, &mut acc, &mut num, &den);
    }

    // Pad the last line with spaces if it's not a full 10 digits.
    // This is required by the benchmarks game rules.
    let remainder = n % 10;
    if remainder != 0 {
        for _ in 0..(10 - remainder) {
            write!(stdout, " ").unwrap();
        }
        writeln!(stdout, "\t:{}", n).unwrap();
    }
}

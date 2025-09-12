// The Computer Language Benchmarks Game
// https://salsa.debian.org/benchmarksgame-team/benchmarksgame/
//
// Translated from Jeremy Zerfas's Python program.

use rug::Integer;
use std::env;
use std::io::{self, Write};

fn main() {
    // --- 1. Setup and Initialization ---
    // Read the number of digits to generate from command-line arguments.
    let n = env::args()
        .nth(1)
        .expect("Usage: pidigits <number_of_digits>")
        .parse::<usize>()
        .expect("The argument must be a positive integer.");

    let mut i = 0; // Digits generated
    let mut k = 0u64; // Term counter

    // Initialize the core state variables of the algorithm using rug's Integer type.
    // This is the Rust equivalent of the __gmpz_init_set_ui calls.
    let mut acc = Integer::from(0);
    let mut den = Integer::from(1);
    let mut num = Integer::from(1);

    // Reusable temporary variables to reduce memory allocations in the main loop.
    let mut tmp1 = Integer::new();
    let mut tmp2 = Integer::new();

    let mut stdout = io::stdout();

    // --- 2. Main loop to generate digits ---
    while i < n {
        k += 1;
        let k2 = k * 2 + 1;

        // --- next_term(k) ---
        // This logic updates the state variables according to the algorithm's formula.
        // It's equivalent to the next_Term() function in the Python code.
        // The methods add_mul_mut and regular multiplication are highly optimized.
        acc.add_mul_mut(&num, 2); // acc = acc + num * 2
        acc *= k2;
        den *= k2;
        num *= k;

        // If the integer part is not stable, continue to the next term.
        if num > &acc {
            continue;
        }

        // --- extract_digit(nth) ---
        // Try to extract a stable digit. This corresponds to the extract_Digit() function.
        // Calculate d = floor((num * 3 + acc) / den)
        tmp1.assign(&num * 3);
        tmp1 += &acc;
        tmp2.assign(tmp1.div_floor(&den)); // tmp2 = result of the floor division
        let d = tmp2.to_u64().unwrap(); // Convert the result to a native integer

        // Check for stability by comparing with extract_digit(4)
        tmp1.assign(&num * 4);
        tmp1 += &acc;
        tmp2.assign(tmp1.div_floor(&den));
        let d_check = tmp2.to_u64().unwrap();

        if d != d_check {
            continue;
        }

        // --- We found a stable digit ---
        // Print the digit. Using write_all with a byte slice is faster than formatting.
        stdout.write_all(&[b'0' + d as u8]).unwrap();
        i += 1;

        // Print progress and a newline every 10 digits.
        if i % 10 == 0 {
            writeln!(stdout, "\t:{}", i).unwrap();
        }

        // --- eliminate_digit(d) ---
        // Remove the extracted digit from the accumulator to prepare for the next iteration.
        // This is the equivalent of the eliminate_Digit() function.
        acc.sub_mul_mut(&den, d); // acc = acc - den * d
        acc *= 10;
        num *= 10;
    }

    // --- 3. Final Formatting ---
    // Pad the final line with spaces if n is not a multiple of 10.
    // This ensures the output is correctly formatted according to benchmark rules.
    let remainder = n % 10;
    if remainder != 0 {
        for _ in 0..(10 - remainder) {
            stdout.write_all(b" ").unwrap();
        }
        writeln!(stdout, "\t:{}", n).unwrap();
    }
}

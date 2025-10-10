// The Computer Language Benchmarks Game
// https://salsa.debian.org/benchmarksgame-team/benchmarksgame/
//
// Converted from Python to Rust by Claude

use gmp::mpz::Mpz;
use std::env;
use std::io::{self, Write};

// Extract a digit from the current state
fn extract_digit(nth: u32, num: &Mpz, acc: &Mpz, den: &Mpz) -> u32 {
    // Joggling between temporary variables to avoid excessive allocations
    let tmp1 = num * nth;
    let tmp2 = &tmp1 + acc;
    let digit = &tmp2 / den;
    
    digit.to_u32().unwrap()
}

// Remove a digit from the current state
fn eliminate_digit(d: u32, acc: &mut Mpz, den: &Mpz, num: &mut Mpz) {
    // Update accumulator: acc = acc - den * d
    *acc -= den * d;
    
    // Multiply by 10 for the next iteration
    *acc *= 10u32;
    *num *= 10u32;
}

// Calculate the next term in the series
fn next_term(k: u32, acc: &mut Mpz, den: &mut Mpz, num: &mut Mpz) {
    let k2 = k * 2 + 1;
    
    // Update accumulator: acc = acc + num * 2
    *acc += num * 2u32;
    
    // Multiply by k2
    *acc *= k2;
    *den *= k2;
    *num *= k;
}

fn main() {
    // Parse command line argument
    let args: Vec<String> = env::args().collect();
    if args.len() != 2 {
        eprintln!("Usage: {} <number of digits>", args[0]);
        std::process::exit(1);
    }
    
    let n: usize = match args[1].parse() {
        Ok(val) => val,
        Err(_) => {
            eprintln!("Error: Number of digits must be a positive integer");
            std::process::exit(1);
        }
    };
    
    // Initialize GMP variables
    let mut acc = Mpz::from(0u32);
    let mut den = Mpz::from(1u32);
    let mut num = Mpz::from(1u32);
    
    let mut i = 0;
    let mut k = 0;
    let stdout = io::stdout();
    let mut handle = stdout.lock();
    
    // Main loop to calculate digits of pi
    while i < n {
        k += 1;
        next_term(k, &mut acc, &mut den, &mut num);
        
        if num > acc {
            continue;
        }
        
        let d3 = extract_digit(3, &num, &acc, &den);
        if d3 != extract_digit(4, &num, &acc, &den) {
            continue;
        }
        
        // We found a valid digit
        write!(handle, "{}", d3).unwrap();
        i += 1;
        if i % 10 == 0 {
            writeln!(handle, "\t:{}", i).unwrap();
        }
        
        eliminate_digit(d3, &mut acc, &den, &mut num);
    }
    
    // Add a newline if we didn't end on a multiple of 10
    if n % 10 != 0 {
        for _ in 0..(10 - (n % 10)) {
            write!(handle, " ").unwrap();
        }
        writeln!(handle, "\t:{}", n).unwrap();
    }
}
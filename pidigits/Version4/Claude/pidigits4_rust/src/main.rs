// The Computer Language Benchmarks Game
// https://salsa.debian.org/benchmarksgame-team/benchmarksgame/
//
// Converted from Python to Rust by Claude

use num_bigint::BigUint;
use std::env;
use std::io::{self, Write};

// Global variables holding the BigUint values
struct State {
    tmp1: BigUint,
    tmp2: BigUint,
    acc: BigUint,
    den: BigUint,
    num: BigUint,
}

impl State {
    // Create a new state with initial values
    fn new() -> Self {
        State {
            tmp1: BigUint::from(0u32),
            tmp2: BigUint::from(0u32),
            acc: BigUint::from(0u32),
            den: BigUint::from(1u32),
            num: BigUint::from(1u32),
        }
    }
    
    // Extract a digit
    fn extract_digit(&mut self, nth: u32) -> u32 {
        // tmp1 = num * nth
        self.tmp1 = &self.num * nth;
        
        // tmp2 = tmp1 + acc
        self.tmp2 = &self.tmp1 + &self.acc;
        
        // tmp1 = tmp2 / den
        self.tmp1 = &self.tmp2 / &self.den;
        
        // Convert the result to u32
        self.tmp1.to_i32().unwrap()
    }
    
    // Eliminate a digit
    fn eliminate_digit(&mut self, d: u32) {
        // acc = acc - den * d
        self.acc = &self.acc - &self.den * d;
        
        // acc = acc * 10
        self.acc *= 10u32;
        
        // num = num * 10
        self.num *= 10u32;
    }
    
    // Calculate the next term
    fn next_term(&mut self, k: u32) {
        let k2 = k * 2 + 1;
        
        // acc = acc + num * 2
        self.acc = &self.acc + &self.num * 2u32;
        
        // acc = acc * k2
        self.acc *= k2;
        
        // den = den * k2
        self.den *= k2;
        
        // num = num * k
        self.num *= k;
    }
}

fn main() {
    // Parse command line arguments
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
    
    // Initialize state
    let mut state = State::new();
    
    // Main loop to calculate digits of pi
    let mut i = 0;
    let mut k = 0;
    let stdout = io::stdout();
    let mut handle = stdout.lock();
    
    while i < n {
        k += 1;
        state.next_term(k);
        
        // Check if num > acc
        if state.num > state.acc {
            continue;
        }
        
        // Extract and compare digits
        let d = state.extract_digit(3);
        if d != state.extract_digit(4) {
            continue;
        }
        
        // Print the digit
        write!(handle, "{}", d).unwrap();
        i += 1;
        
        // Print tab and counter every 10 digits
        if i % 10 == 0 {
            writeln!(handle, "\t:{}", i).unwrap();
        }
        
        // Eliminate the digit
        state.eliminate_digit(d);
    }
    
    // Add a final newline if needed
    if n % 10 != 0 {
        // Print spaces to align the tab
        for _ in 0..(10 - (n % 10)) {
            write!(handle, " ").unwrap();
        }
        writeln!(handle, "\t:{}", n).unwrap();
    }
}

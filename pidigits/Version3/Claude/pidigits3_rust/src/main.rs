// The Computer Language Benchmarks Game
// https://salsa.debian.org/benchmarksgame-team/benchmarksgame/
//
// Converted from Python to Rust with optimizations by Claude

use gmp_mpfr_sys::gmp;
use std::env;
use std::io::{self, Write};
use std::mem::MaybeUninit;

// We use the low-level bindings for maximum performance
// These are wrappers around the actual GMP integers
struct Mpz(gmp::mpz_t);

impl Mpz {
    fn new() -> Self {
        let mut z = MaybeUninit::uninit();
        unsafe {
            gmp::mpz_init(z.as_mut_ptr());
            Mpz(z.assume_init())
        }
    }

    fn set_ui(&mut self, n: u32) {
        unsafe {
            gmp::mpz_set_ui(&mut self.0, n as libc::c_ulong);
        }
    }

    fn get_ui(&self) -> u32 {
        unsafe {
            gmp::mpz_get_ui(&self.0) as u32
        }
    }
}

// Implement Drop to automatically free GMP resources
impl Drop for Mpz {
    fn drop(&mut self) {
        unsafe {
            gmp::mpz_clear(&mut self.0);
        }
    }
}

// Extract a digit from the current state
unsafe fn extract_digit(nth: u32, num: &Mpz, acc: &Mpz, den: &Mpz, tmp1: &mut Mpz, tmp2: &mut Mpz) -> u32 {
    // Joggling between tmp1 and tmp2, so GMP won't have to use temp buffers
    gmp::mpz_mul_ui(&mut tmp1.0, &num.0, nth as libc::c_ulong);
    gmp::mpz_add(&mut tmp2.0, &tmp1.0, &acc.0);
    gmp::mpz_tdiv_q(&mut tmp1.0, &tmp2.0, &den.0);
    
    gmp::mpz_get_ui(&tmp1.0) as u32
}

// Remove a digit from the current state
unsafe fn eliminate_digit(d: u32, acc: &mut Mpz, den: &Mpz, num: &mut Mpz) {
    gmp::mpz_submul_ui(&mut acc.0, &den.0, d as libc::c_ulong);
    gmp::mpz_mul_ui(&mut acc.0, &acc.0, 10);
    gmp::mpz_mul_ui(&mut num.0, &num.0, 10);
}

// Calculate the next term in the series
unsafe fn next_term(k: u32, acc: &mut Mpz, den: &mut Mpz, num: &mut Mpz) {
    let k2 = k * 2 + 1;
    
    gmp::mpz_addmul_ui(&mut acc.0, &num.0, 2);
    gmp::mpz_mul_ui(&mut acc.0, &acc.0, k2 as libc::c_ulong);
    gmp::mpz_mul_ui(&mut den.0, &den.0, k2 as libc::c_ulong);
    gmp::mpz_mul_ui(&mut num.0, &num.0, k as libc::c_ulong);
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
    let mut tmp1 = Mpz::new();
    let mut tmp2 = Mpz::new();
    let mut acc = Mpz::new();
    let mut den = Mpz::new();
    let mut num = Mpz::new();
    
    tmp1.set_ui(0);
    tmp2.set_ui(0);
    acc.set_ui(0);
    den.set_ui(1);
    num.set_ui(1);
    
    let mut i = 0;
    let mut k = 0;
    
    // Create a buffer for output to reduce system calls
    let stdout = io::stdout();
    let mut handle = stdout.lock();
    let mut buf = [0u8; 1];
    
    // Main loop to calculate digits of pi
    unsafe {
        while i < n {
            k += 1;
            next_term(k, &mut acc, &mut den, &mut num);
            
            if gmp::mpz_cmp(&num.0, &acc.0) > 0 {
                continue;
            }
            
            let d3 = extract_digit(3, &num, &acc, &den, &mut tmp1, &mut tmp2);
            let d4 = extract_digit(4, &num, &acc, &den, &mut tmp1, &mut tmp2);
            
            if d3 != d4 {
                continue;
            }
            
            // We found a valid digit
            buf[0] = b'0' + d3 as u8;
            handle.write_all(&buf).unwrap();
            
            i += 1;
            if i % 10 == 0 {
                write!(handle, "\t:{}\n", i).unwrap();
            }
            
            eliminate_digit(d3, &mut acc, &den, &mut num);
        }
    }
    
    // Add a newline if we didn't end on a multiple of 10
    if n % 10 != 0 {
        for _ in 0..(10 - (n % 10)) {
            handle.write_all(b" ").unwrap();
        }
        writeln!(handle, "\t:{}", n).unwrap();
    }
}
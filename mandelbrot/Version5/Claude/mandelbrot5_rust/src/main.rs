// The Computer Language Benchmarks Game
// https://salsa.debian.org/benchmarksgame-team/benchmarksgame/
//
// Converted from Python to Rust by Claude

use std::env;
use std::io::{self, Write};
use std::sync::Arc;
use rayon::prelude::*;

// Calculate one row of the Mandelbrot set
fn do_row(y: usize, size: usize) -> Vec<u8> {
    let two_over_size = 2.0 / size as f64;
    let fy = Complex64 { 
        re: 0.0, 
        im: 2.0 * y as f64 / size as f64 - 1.5 
    };
    
    // Calculate how many bytes we need for this row
    let mut result = Vec::with_capacity((size + 7) / 8);
    
    for x in (7..size).step_by(8) {
        let mut byte_acc: u8 = 0;
        
        for offset in (0..=7).rev() {
            if x >= offset {
                let cx = two_over_size * (x - offset) as f64;
                let c = Complex64 { re: cx, im: fy.im };
                
                let mut z = Complex64 { re: 0.0, im: 0.0 };
                let mut i = 0;
                
                while i < 50 {
                    z = z * z + c;
                    
                    if z.norm_sqr() >= 4.0 {
                        break;
                    }
                    
                    i += 1;
                }
                
                if i == 50 {
                    byte_acc |= 1 << offset;
                }
            }
        }
        
        result.push(byte_acc);
    }
    
    // Handle the last byte if the width is not a multiple of 8
    if size % 8 != 0 {
        let mut byte_acc: u8 = 0;
        let x = size / 8 * 8;
        
        for offset in (0..size % 8).rev() {
            let cx = two_over_size * (x + (7 - offset)) as f64;
            let c = Complex64 { re: cx, im: fy.im };
            
            let mut z = Complex64 { re: 0.0, im: 0.0 };
            let mut i = 0;
            
            while i < 50 {
                z = z * z + c;
                
                if z.norm_sqr() >= 4.0 {
                    break;
                }
                
                i += 1;
            }
            
            if i == 50 {
                byte_acc |= 1 << offset;
            }
        }
        
        result.push(byte_acc);
    }
    
    result
}

// A simple complex number implementation for this specific task
#[derive(Clone, Copy, Debug)]
struct Complex64 {
    re: f64,
    im: f64,
}

impl Complex64 {
    // Calculate the squared norm (magnitude squared)
    fn norm_sqr(&self) -> f64 {
        self.re * self.re + self.im * self.im
    }
}

// Implement addition
impl std::ops::Add for Complex64 {
    type Output = Self;
    
    fn add(self, other: Self) -> Self {
        Self {
            re: self.re + other.re,
            im: self.im + other.im,
        }
    }
}

// Implement multiplication
impl std::ops::Mul for Complex64 {
    type Output = Self;
    
    fn mul(self, other: Self) -> Self {
        Self {
            re: self.re * other.re - self.im * other.im,
            im: self.re * other.im + self.im * other.re,
        }
    }
}

fn main() -> io::Result<()> {
    // Parse command-line arguments
    let args: Vec<String> = env::args().collect();
    if args.len() != 2 {
        eprintln!("Usage: {} <size>", args[0]);
        std::process::exit(1);
    }
    
    let size: usize = args[1].parse().expect("Size must be a positive integer");
    
    // Write the PBM header
    let stdout = io::stdout();
    let mut stdout = stdout.lock();
    
    writeln!(stdout, "P4\n{} {}", size, size)?;
    
    // Process rows in parallel using Rayon
    let rows: Vec<Vec<u8>> = (0..size)
        .into_par_iter()
        .map(|y| do_row(y, size))
        .collect();
    
    // Write all rows to stdout
    for row in rows {
        stdout.write_all(&row)?;
    }
    
    Ok(())
}
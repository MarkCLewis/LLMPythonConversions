// The Computer Language Benchmarks Game
// https://salsa.debian.org/benchmarksgame-team/benchmarksgame/
//
// Converted from Python to Rust by Claude

use std::env;
use std::io::{self, Write};
use std::sync::{Arc, Mutex};
use num_complex::Complex64;
use rayon::prelude::*;

// Calculate the pixels for a given row
fn calculate_pixels(y: usize, n: usize) -> Vec<u8> {
    let c1 = 2.0 / n as f64;
    let c0 = Complex64::new(-1.5, y as f64 * c1 - 1.0);
    
    // Pre-calculate bit positions
    let pixel_bits: [u8; 8] = [128, 64, 32, 16, 8, 4, 2, 1];
    
    // Calculate how many bytes we need for this row
    let bytes_per_row = (n + 7) / 8;
    let mut row = vec![0u8; bytes_per_row];
    
    for byte_x in 0..bytes_per_row {
        let mut byte_acc: u8 = 0;
        let x_base = byte_x * 8;
        
        for bit in 0..8 {
            let x = x_base + bit;
            if x >= n {
                // Skip if we're past the image width
                continue;
            }
            
            // Starting point for this pixel
            let c = c0 + Complex64::new(x as f64 * c1, 0.0);
            let mut z = c;
            
            // The original Python code runs 7 iterations of 7 iterations (49 total)
            let mut escaped = false;
            for _ in 0..7 {
                for _ in 0..7 {
                    z = z * z + c;
                    if z.norm_sqr() >= 4.0 {
                        escaped = true;
                        break;
                    }
                }
                if escaped {
                    break;
                }
            }
            
            // If the point didn't escape, set the corresponding bit
            if !escaped {
                byte_acc |= pixel_bits[bit];
            }
        }
        
        row[byte_x] = byte_acc;
    }
    
    // Clear any unused bits in the last byte
    if n % 8 != 0 {
        row[bytes_per_row-1] &= 0xff << (8 - n % 8);
    }
    
    row
}

// Calculate a single row and return the row index and data
fn compute_row(row_data: (usize, usize)) -> (usize, Vec<u8>) {
    let (y, n) = row_data;
    let pixels = calculate_pixels(y, n);
    (y, pixels)
}

// Main function to generate the Mandelbrot set
fn mandelbrot(n: usize) -> io::Result<()> {
    // Write the PBM header
    let stdout = io::stdout();
    let mut stdout_handle = stdout.lock();
    writeln!(stdout_handle, "P4\n{} {}", n, n)?;
    
    // Configure thread pool for optimal performance
    rayon::ThreadPoolBuilder::new()
        .num_threads(num_cpus::get())
        .build_global()
        .unwrap();
    
    // Create row tasks
    let rows: Vec<(usize, usize)> = (0..n).map(|y| (y, n)).collect();
    
    // Store results in order
    let results = Arc::new(Mutex::new(vec![None; n]));
    
    // Compute rows in parallel using Rayon
    let next_row = Arc::new(Mutex::new(0));
    
    // Process rows in parallel
    rows.into_par_iter()
        .map(compute_row)
        .for_each(|(y, row_data)| {
            // Store the result
            let mut results_lock = results.lock().unwrap();
            results_lock[y] = Some(row_data);
            
            // Try to write rows to stdout in order
            let mut next_row_lock = next_row.lock().unwrap();
            
            // Write as many consecutive rows as possible
            while *next_row_lock < n {
                if let Some(ref data) = results_lock[*next_row_lock] {
                    // Release locks while writing to stdout to avoid blocking other threads
                    drop(next_row_lock);
                    drop(results_lock);
                    
                    let _ = stdout_handle.write_all(data);
                    
                    // Acquire locks again
                    results_lock = results.lock().unwrap();
                    next_row_lock = next_row.lock().unwrap();
                    
                    // Mark this row as processed by setting it to None to free memory
                    results_lock[*next_row_lock] = None;
                    *next_row_lock += 1;
                } else {
                    // Wait for this row to be computed
                    break;
                }
            }
        });
    
    // Flush stdout
    stdout_handle.flush()?;
    
    Ok(())
}

fn main() -> io::Result<()> {
    // Parse command line arguments
    let args: Vec<String> = env::args().collect();
    if args.len() != 2 {
        eprintln!("Usage: {} <size>", args[0]);
        std::process::exit(1);
    }
    
    let n = args[1].parse::<usize>().expect("Size must be a positive integer");
    mandelbrot(n)?;
    
    Ok(())
}
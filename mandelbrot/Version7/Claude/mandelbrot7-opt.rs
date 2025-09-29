// The Computer Language Benchmarks Game
// https://salsa.debian.org/benchmarksgame-team/benchmarksgame/
//
// Converted from Python to Rust with SIMD optimizations by Claude

#![feature(stdsimd)]

use std::env;
use std::io::{self, Write};
use std::sync::{Arc, Mutex};
use rayon::prelude::*;

#[cfg(target_arch = "x86_64")]
use std::arch::x86_64::*;

// Calculate the pixels for a given row using the optimal available implementation
fn calculate_pixels(y: usize, n: usize) -> Vec<u8> {
    #[cfg(target_arch = "x86_64")]
    {
        if is_x86_feature_detected!("avx2") {
            return unsafe { calculate_pixels_avx2(y, n) };
        } else if is_x86_feature_detected!("sse2") {
            return unsafe { calculate_pixels_sse2(y, n) };
        }
    }
    
    // Fallback to scalar implementation
    calculate_pixels_scalar(y, n)
}

// Scalar implementation (no SIMD)
fn calculate_pixels_scalar(y: usize, n: usize) -> Vec<u8> {
    let c1 = 2.0 / n as f64;
    let y0 = y as f64 * c1 - 1.0;
    
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
            
            let cr = x as f64 * c1 - 1.5;
            let ci = y0;
            
            let mut zr = 0.0;
            let mut zi = 0.0;
            let mut zr2 = 0.0;
            let mut zi2 = 0.0;
            
            // The original Python code runs 7 iterations of 7 iterations (49 total)
            let mut i = 0;
            while i < 49 && zr2 + zi2 <= 4.0 {
                zi = 2.0 * zr * zi + ci;
                zr = zr2 - zi2 + cr;
                zr2 = zr * zr;
                zi2 = zi * zi;
                i += 1;
            }
            
            // If the point didn't escape, set the corresponding bit
            if i == 49 {
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

// SSE2 implementation (process 2 pixels at once)
#[cfg(target_arch = "x86_64")]
#[target_feature(enable = "sse2")]
unsafe fn calculate_pixels_sse2(y: usize, n: usize) -> Vec<u8> {
    let c1 = 2.0 / n as f64;
    let y0 = y as f64 * c1 - 1.0;
    
    // Pre-calculate bit positions
    let pixel_bits: [u8; 8] = [128, 64, 32, 16, 8, 4, 2, 1];
    
    // Calculate how many bytes we need for this row
    let bytes_per_row = (n + 7) / 8;
    let mut row = vec![0u8; bytes_per_row];
    
    for byte_x in 0..bytes_per_row {
        let mut byte_acc: u8 = 0;
        let x_base = byte_x * 8;
        
        // Process 2 pixels at a time with SSE2
        for bit in (0..8).step_by(2) {
            let x = x_base + bit;
            if x >= n {
                // Skip if we're past the image width
                continue;
            }
            
            // Prepare constants
            let cr0 = x as f64 * c1 - 1.5;
            let cr1 = if x + 1 < n { (x + 1) as f64 * c1 - 1.5 } else { 0.0 };
            
            let cr = _mm_set_pd(cr1, cr0);
            let ci = _mm_set1_pd(y0);
            let mut zr = _mm_setzero_pd();
            let mut zi = _mm_setzero_pd();
            let two = _mm_set1_pd(2.0);
            let four = _mm_set1_pd(4.0);
            
            // Count iterations
            let mut iterations = [49, 49];  // Assume both points are in the set
            
            for i in 0..49 {
                // Calculate z = z^2 + c
                let zr2 = _mm_mul_pd(zr, zr);
                let zi2 = _mm_mul_pd(zi, zi);
                let zrzi = _mm_mul_pd(zr, zi);
                
                // Check if |z|^2 > 4
                let mag2 = _mm_add_pd(zr2, zi2);
                let mask = _mm_cmple_pd(mag2, four);
                let mask_bits = _mm_movemask_pd(mask);
                
                // If both pixels have escaped, break
                if mask_bits == 0 {
                    iterations[0] = i;
                    iterations[1] = i;
                    break;
                }
                
                // Check individual pixels
                if mask_bits & 1 == 0 && iterations[0] == 49 {
                    iterations[0] = i;
                }
                
                if mask_bits & 2 == 0 && iterations[1] == 49 {
                    iterations[1] = i;
                }
                
                // Compute next iteration
                zi = _mm_add_pd(_mm_mul_pd(two, zrzi), ci);
                zr = _mm_add_pd(_mm_sub_pd(zr2, zi2), cr);
            }
            
            // Set bits for points that didn't escape
            if iterations[0] == 49 {
                byte_acc |= pixel_bits[bit];
            }
            
            if x + 1 < n && iterations[1] == 49 {
                byte_acc |= pixel_bits[bit + 1];
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

// AVX2 implementation (process 4 pixels at once)
#[cfg(target_arch = "x86_64")]
#[target_feature(enable = "avx2")]
unsafe fn calculate_pixels_avx2(y: usize, n: usize) -> Vec<u8> {
    let c1 = 2.0 / n as f64;
    let y0 = y as f64 * c1 - 1.0;
    
    // Pre-calculate bit positions
    let pixel_bits: [u8; 8] = [128, 64, 32, 16, 8, 4, 2, 1];
    
    // Calculate how many bytes we need for this row
    let bytes_per_row = (n + 7) / 8;
    let mut row = vec![0u8; bytes_per_row];
    
    for byte_x in 0..bytes_per_row {
        let mut byte_acc: u8 = 0;
        let x_base = byte_x * 8;
        
        // Process 4 pixels at a time with AVX2
        for bit in (0..8).step_by(4) {
            let x = x_base + bit;
            if x >= n {
                // Skip if we're past the image width
                continue;
            }
            
            // Prepare constants
            let cr0 = x as f64 * c1 - 1.5;
            let cr1 = if x + 1 < n { (x + 1) as f64 * c1 - 1.5 } else { 0.0 };
            let cr2 = if x + 2 < n { (x + 2) as f64 * c1 - 1.5 } else { 0.0 };
            let cr3 = if x + 3 < n { (x + 3) as f64 * c1 - 1.5 } else { 0.0 };
            
            let cr = _mm256_set_pd(cr3, cr2, cr1, cr0);
            let ci = _mm256_set1_pd(y0);
            let mut zr = _mm256_setzero_pd();
            let mut zi = _mm256_setzero_pd();
            let two = _mm256_set1_pd(2.0);
            let four = _mm256_set1_pd(4.0);
            
            // Count iterations
            let mut iterations = [49, 49, 49, 49];  // Assume all points are in the set
            
            for i in 0..49 {
                // Calculate z = z^2 + c
                let zr2 = _mm256_mul_pd(zr, zr);
                let zi2 = _mm256_mul_pd(zi, zi);
                let zrzi = _mm256_mul_pd(zr, zi);
                
                // Check if |z|^2 > 4
                let mag2 = _mm256_add_pd(zr2, zi2);
                let mask = _mm256_cmp_pd(mag2, four, _CMP_LE_OQ);
                let mask_bits = _mm256_movemask_pd(mask);
                
                // If all pixels have escaped, break
                if mask_bits == 0 {
                    iterations[0] = i;
                    iterations[1] = i;
                    iterations[2] = i;
                    iterations[3] = i;
                    break;
                }
                
                // Check individual pixels
                if mask_bits & 1 == 0 && iterations[0] == 49 {
                    iterations[0] = i;
                }
                if mask_bits & 2 == 0 && iterations[1] == 49 {
                    iterations[1] = i;
                }
                if mask_bits & 4 == 0 && iterations[2] == 49 {
                    iterations[2] = i;
                }
                if mask_bits & 8 == 0 && iterations[3] == 49 {
                    iterations[3] = i;
                }
                
                // Compute next iteration
                zi = _mm256_add_pd(_mm256_mul_pd(two, zrzi), ci);
                zr = _mm256_add_pd(_mm256_sub_pd(zr2, zi2), cr);
            }
            
            // Set bits for points that didn't escape
            for i in 0..4 {
                if x + i < n && iterations[i] == 49 {
                    byte_acc |= pixel_bits[bit + i];
                }
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
    println!("P4\n{} {}", n, n);
    
    // Get standard output
    let stdout = io::stdout();
    let stdout = Arc::new(Mutex::new(stdout.lock()));
    
    // Configure thread pool for optimal performance
    rayon::ThreadPoolBuilder::new()
        .num_threads(num_cpus::get())
        .build_global()
        .unwrap();
    
    // Create row tasks
    let rows: Vec<(usize, usize)> = (0..n).map(|y| (y, n)).collect();
    
    // Create ordered buffer for output
    let results = Arc::new(Mutex::new(vec![None; n]));
    let next_row = Arc::new(Mutex::new(0));
    
    // Process rows in parallel
    rows.into_par_iter()
        .map(compute_row)
        .for_each(|(y, row_data)| {
            // Store the result
            let mut results_lock = results.lock().unwrap();
            results_lock[y] = Some(row_data);
            drop(results_lock);
            
            // Try to write rows to stdout in order
            let mut stdout_locked = false;
            let mut stdout_guard = None;
            
            let mut next_row_lock = next_row.lock().unwrap();
            let mut results_lock = results.lock().unwrap();
            
            // Write as many consecutive rows as possible
            while *next_row_lock < n {
                if let Some(ref data) = results_lock[*next_row_lock] {
                    // Get stdout lock if we don't have it
                    if !stdout_locked {
                        drop(results_lock);
                        stdout_guard = Some(stdout.lock().unwrap());
                        stdout_locked = true;
                        results_lock = results.lock().unwrap();
                    }
                    
                    // Write the row
                    if let Some(ref mut out) = stdout_guard {
                        let _ = out.write_all(data);
                    }
                    
                    // Mark this row as processed by setting it to None to free memory
                    results_lock[*next_row_lock] = None;
                    *next_row_lock += 1;
                } else {
                    // Wait for this row to be computed
                    break;
                }
            }
            
            // Explicit drop of guards to ensure locks are released
            drop(next_row_lock);
            drop(results_lock);
            drop(stdout_guard);
        });
    
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
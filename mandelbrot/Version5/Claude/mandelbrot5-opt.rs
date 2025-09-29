// The Computer Language Benchmarks Game
// https://salsa.debian.org/benchmarksgame-team/benchmarksgame/
//
// Converted from Python to Rust with SIMD optimizations by Claude

use std::env;
use std::io::{self, Write};
use std::arch::x86_64::*;
use rayon::prelude::*;

// Calculate one row of the Mandelbrot set
fn do_row(y: usize, size: usize) -> Vec<u8> {
    if is_x86_feature_detected!("avx2") {
        unsafe { do_row_avx2(y, size) }
    } else if is_x86_feature_detected!("sse2") {
        unsafe { do_row_sse2(y, size) }
    } else {
        do_row_scalar(y, size)
    }
}

// Scalar implementation (no SIMD)
fn do_row_scalar(y: usize, size: usize) -> Vec<u8> {
    let two_over_size = 2.0 / size as f64;
    let y_offset = 2.0 * y as f64 / size as f64 - 1.5;
    
    // Calculate how many bytes we need for this row
    let byte_count = (size + 7) / 8;
    let mut result = vec![0u8; byte_count];
    
    for x_byte in 0..byte_count {
        let mut byte_acc: u8 = 0;
        let x_base = x_byte * 8;
        
        for bit in 0..8 {
            let x = x_base + bit;
            if x < size {
                let cr = x as f64 * two_over_size - 2.0;
                let ci = y_offset;
                
                let mut zr = 0.0;
                let mut zi = 0.0;
                let mut i = 0;
                
                while i < 50 && zr * zr + zi * zi <= 4.0 {
                    let new_zr = zr * zr - zi * zi + cr;
                    zi = 2.0 * zr * zi + ci;
                    zr = new_zr;
                    i += 1;
                }
                
                if i == 50 {
                    byte_acc |= 0x80 >> bit;
                }
            }
        }
        
        result[x_byte] = byte_acc;
    }
    
    result
}

// SSE2 implementation (process 2 pixels at once)
#[target_feature(enable = "sse2")]
unsafe fn do_row_sse2(y: usize, size: usize) -> Vec<u8> {
    let two_over_size = 2.0 / size as f64;
    let y_offset = 2.0 * y as f64 / size as f64 - 1.5;
    
    let byte_count = (size + 7) / 8;
    let mut result = vec![0u8; byte_count];
    
    // Process 2 pixels at a time using SSE2
    for x in (0..size).step_by(2) {
        let cr0 = x as f64 * two_over_size - 2.0;
        let cr1 = if x + 1 < size { (x + 1) as f64 * two_over_size - 2.0 } else { 0.0 };
        
        // Set up initial values
        let cr = _mm_set_pd(cr1, cr0);
        let ci = _mm_set1_pd(y_offset);
        let mut zr = _mm_setzero_pd();
        let mut zi = _mm_setzero_pd();
        let four = _mm_set1_pd(4.0);
        let two = _mm_set1_pd(2.0);
        
        // Count iterations for each pixel
        let mut iterations = [0, 0];
        
        for i in 0..50 {
            // Calculate zr² and zi²
            let zr2 = _mm_mul_pd(zr, zr);
            let zi2 = _mm_mul_pd(zi, zi);
            
            // Check if |z|² > 4.0 for both pixels
            let mag_squared = _mm_add_pd(zr2, zi2);
            let mask = _mm_cmple_pd(mag_squared, four);
            let mask_bits = _mm_movemask_pd(mask);
            
            // If both pixels have escaped, break
            if mask_bits == 0 {
                break;
            }
            
            // Update iterations for pixels that haven't escaped yet
            if mask_bits & 1 != 0 {
                iterations[0] = i + 1;
            }
            if mask_bits & 2 != 0 && x + 1 < size {
                iterations[1] = i + 1;
            }
            
            // z = z² + c
            let zrzi = _mm_mul_pd(zr, zi);
            zi = _mm_add_pd(_mm_mul_pd(two, zrzi), ci);
            zr = _mm_add_pd(_mm_sub_pd(zr2, zi2), cr);
        }
        
        // Set the appropriate bits in the result array
        for (i, &iter_count) in iterations.iter().enumerate() {
            if x + i < size && iter_count == 50 {
                let byte_index = (x + i) / 8;
                let bit_index = 7 - (x + i) % 8;
                result[byte_index] |= 1 << bit_index;
            }
        }
    }
    
    result
}

// AVX2 implementation (process 4 pixels at once)
#[target_feature(enable = "avx2")]
unsafe fn do_row_avx2(y: usize, size: usize) -> Vec<u8> {
    let two_over_size = 2.0 / size as f64;
    let y_offset = 2.0 * y as f64 / size as f64 - 1.5;
    
    let byte_count = (size + 7) / 8;
    let mut result = vec![0u8; byte_count];
    
    // Process 4 pixels at a time using AVX2
    for x in (0..size).step_by(4) {
        let cr0 = x as f64 * two_over_size - 2.0;
        let cr1 = if x + 1 < size { (x + 1) as f64 * two_over_size - 2.0 } else { 0.0 };
        let cr2 = if x + 2 < size { (x + 2) as f64 * two_over_size - 2.0 } else { 0.0 };
        let cr3 = if x + 3 < size { (x + 3) as f64 * two_over_size - 2.0 } else { 0.0 };
        
        // Set up initial values
        let cr = _mm256_set_pd(cr3, cr2, cr1, cr0);
        let ci = _mm256_set1_pd(y_offset);
        let mut zr = _mm256_setzero_pd();
        let mut zi = _mm256_setzero_pd();
        let four = _mm256_set1_pd(4.0);
        let two = _mm256_set1_pd(2.0);
        
        // Count iterations for each pixel
        let mut iterations = [0, 0, 0, 0];
        
        for i in 0..50 {
            // Calculate zr² and zi²
            let zr2 = _mm256_mul_pd(zr, zr);
            let zi2 = _mm256_mul_pd(zi, zi);
            
            // Check if |z|² > 4.0 for all pixels
            let mag_squared = _mm256_add_pd(zr2, zi2);
            let mask = _mm256_cmp_pd(mag_squared, four, _CMP_LE_OQ);
            let mask_bits = _mm256_movemask_pd(mask);
            
            // If all pixels have escaped, break
            if mask_bits == 0 {
                break;
            }
            
            // Update iterations for pixels that haven't escaped yet
            if mask_bits & 1 != 0 {
                iterations[0] = i + 1;
            }
            if mask_bits & 2 != 0 && x + 1 < size {
                iterations[1] = i + 1;
            }
            if mask_bits & 4 != 0 && x + 2 < size {
                iterations[2] = i + 1;
            }
            if mask_bits & 8 != 0 && x + 3 < size {
                iterations[3] = i + 1;
            }
            
            // z = z² + c
            let zrzi = _mm256_mul_pd(zr, zi);
            zi = _mm256_add_pd(_mm256_mul_pd(two, zrzi), ci);
            zr = _mm256_add_pd(_mm256_sub_pd(zr2, zi2), cr);
        }
        
        // Set the appropriate bits in the result array
        for (i, &iter_count) in iterations.iter().enumerate() {
            if x + i < size && iter_count == 50 {
                let byte_index = (x + i) / 8;
                let bit_index = 7 - (x + i) % 8;
                result[byte_index] |= 1 << bit_index;
            }
        }
    }
    
    result
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
    
    // Configure Rayon thread pool for optimal performance
    rayon::ThreadPoolBuilder::new()
        .num_threads(num_cpus::get())
        .build_global()
        .unwrap();
    
    // Process rows in parallel
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
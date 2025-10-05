// We use the `num-complex` crate for complex number calculations
// and `rayon` for easy and efficient data parallelism.
use num_complex::Complex;
use rayon::prelude::*;
use std::io::{self, BufWriter, Write};

/// Computes the Mandelbrot set and writes a binary PBM image to standard output.
fn main() {
    // 1. Parse the image size 'n' from the first command-line argument.
    let n = std::env::args()
        .nth(1)
        .expect("Usage: mandelbrot_rust <size>")
        .parse::<usize>()
        .expect("Image size must be an integer.");

    // 2. Print the PBM (Portable Bitmap) header.
    //    'P4' indicates a binary bitmap format.
    //    The next line is the width and height of the image.
    println!("P4\n{0} {0}", n);

    // 3. Pre-calculate the width of each row in bytes.
    //    Since each byte holds 8 pixels (1 bit per pixel), we divide by 8, rounding up.
    let bytes_per_row = (n + 7) / 8;

    // 4. Generate the image data in parallel using Rayon.
    //    We create an iterator for each row `y` from 0 to n-1.
    //    `.into_par_iter()` turns this into a parallel iterator,
    //    distributing the work across all available CPU cores.
    let pixels: Vec<u8> = (0..n)
        .into_par_iter()
        .flat_map(|y| {
            // This closure is executed in parallel for each row `y`.
            // It calculates all the bytes for that single row.
            let mut row_data = vec![0; bytes_per_row];
            let inv_n = 2.0 / n as f64;
            let y0 = y as f64 * inv_n - 1.0;

            // Iterate through each byte of the current row.
            for (x_byte_idx, byte) in row_data.iter_mut().enumerate() {
                let x8 = x_byte_idx * 8;
                let mut current_byte = 0_u8;

                // For each byte, compute the 8 pixels it represents.
                for i in 0..8 {
                    let x = x8 + i;
                    // Stop if we've gone past the image width for the last byte.
                    if x >= n {
                        break;
                    }

                    let x0 = x as f64 * inv_n - 1.5;
                    let c = Complex::new(x0, y0);
                    let mut z = Complex::new(0.0, 0.0);

                    // A point is in the set if `z = z*z + c` does not escape
                    // to infinity. We check for a fixed number of iterations.
                    // (The Python version used two nested loops of range(7), so 49 iterations).
                    let mut in_set = true;
                    for _ in 0..49 {
                        z = z * z + c;
                        // Use `norm_sqr()` to check |z|² > 4, which is faster
                        // than `norm()` because it avoids a square root.
                        if z.norm_sqr() > 4.0 {
                            in_set = false;
                            break;
                        }
                    }
                    
                    // If the point remained bounded (in the set), set its corresponding bit to 1.
                    if in_set {
                        current_byte |= 1 << (7 - i);
                    }
                }
                *byte = current_byte;
            }
            // `flat_map` expects an iterator, so we return the row data.
            // Rayon will collect these rows in the correct order automatically.
            row_data
        })
        .collect();

    // 5. Write the final pixel buffer to standard output.
    //    Using a `BufWriter` is more efficient as it minimizes system calls.
    let mut stdout = BufWriter::new(io::stdout().lock());
    stdout
        .write_all(&pixels)
        .expect("Failed to write pixel data to stdout.");
}

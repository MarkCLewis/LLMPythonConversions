use rayon::prelude::*;
use std::env;
use std::io::{self, Write};

#[inline]
fn pixel_byte(y: usize, xb: usize, n: usize) -> u8 {
    // Matches mandelbrot7.py mapping and iteration schedule:
    // c1 = 2.0 / n
    // c0 = (-1.5) + i*(y*c1 - 1.0)
    // c  = (x*c1) + c0
    // z starts as c; iterate 7 blocks × 7 inner iterations (49 total)
    let c1 = 2.0f64 / n as f64;
    let ci = y as f64 * c1 - 1.0;
    let c0r = -1.5f64;
    let x0 = xb * 8;

    let mut byte: u8 = 0;
    for bit in 0..8 {
        let x = x0 + bit;
        if x >= n {
            break; // off-image bits remain 0
        }

        let cr = c0r + x as f64 * c1;

        // z = c (as in the Python version)
        let mut zr = cr;
        let mut zi = ci;
        let mut inside = true;

        // 7 blocks × 7 inner iterations, check |z| after each block
        for _outer in 0..7 {
            for _inner in 0..7 {
                // z = z*z + c
                let zr2 = zr * zr - zi * zi + cr;
                let zi2 = 2.0 * zr * zi + ci;
                zr = zr2;
                zi = zi2;
            }
            if zr * zr + zi * zi >= 4.0 {
                inside = false;
                break;
            }
        }

        if inside {
            byte |= 0x80u8 >> bit;
        }
    }
    byte
}

fn main() -> io::Result<()> {
    // Read image size (n x n)
    let n: usize = env::args()
        .nth(1)
        .and_then(|s| s.parse().ok())
        .unwrap_or(200);

    let row_bytes = (n + 7) / 8;
    let mut out = io::BufWriter::new(io::stdout());

    // PBM (P4) header
    writeln!(out, "P4")?;
    writeln!(out, "{} {}", n, n)?;

    // Compute rows in parallel, then write sequentially to keep output order
    let rows: Vec<Vec<u8>> = (0..n)
        .into_par_iter()
        .map(|y| {
            let mut row = vec![0u8; row_bytes];
            for xb in 0..row_bytes {
                row[xb] = pixel_byte(y, xb, n);
            }
            // Mask trailing bits in the last byte so that only the leftmost (n % 8) bits are kept
            if (n & 7) != 0 {
                let mask: u8 = 0xFFu8 << (8 - (n as u8 & 7));
                let last = row_bytes - 1;
                row[last] &= mask;
            }
            row
        })
        .collect();

    for row in rows {
        out.write_all(&row)?;
    }
    out.flush()?;
    Ok(())
}

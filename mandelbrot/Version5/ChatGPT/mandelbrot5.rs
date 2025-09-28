use rayon::prelude::*;
use std::env;
use std::io::{self, Write};

#[inline]
fn mand_byte(y: usize, xb: usize, size: usize) -> u8 {
    // Compute one output byte = 8 pixels at (x = xb*8 .. xb*8+7) for row y
    // Mapping matches classic benchmark: real in [-1.5, 0.5), imag in [-1.0, 1.0)
    let two_over_size = 2.0f64 / size as f64;
    let ci = -1.0 + two_over_size * y as f64;

    let mut byte = 0u8;
    // bit 7 corresponds to x = xb*8 + 0 (leftmost of the 8-pixel group)
    for bit in 0..8 {
        let x = xb * 8 + bit;
        if x >= size {
            // Off-image bits in the last (partial) byte are left as 0
            continue;
        }
        let cr = -1.5 + two_over_size * x as f64;

        let mut zr = 0.0f64;
        let mut zi = 0.0f64;
        let mut inside = true;

        // 50 iterations, bail if |z|^2 >= 4
        for _ in 0..50 {
            let zr2 = zr * zr - zi * zi + cr;
            let zi2 = 2.0 * zr * zi + ci;
            zr = zr2;
            zi = zi2;
            if zr * zr + zi * zi >= 4.0 {
                inside = false;
                break;
            }
        }
        if inside {
            byte |= 1 << (7 - bit);
        }
    }
    byte
}

fn main() -> io::Result<()> {
    // Read size (image is size x size)
    let n: usize = env::args()
        .nth(1)
        .and_then(|s| s.parse().ok())
        .unwrap_or(200);

    let row_bytes = (n + 7) / 8;
    let mut out = io::BufWriter::new(io::stdout());

    // PBM (P4) header
    writeln!(out, "P4")?;
    writeln!(out, "{} {}", n, n)?;

    // Compute all rows in parallel, then write sequentially
    let rows: Vec<Vec<u8>> = (0..n)
        .into_par_iter()
        .map(|y| {
            let mut row = vec![0u8; row_bytes];
            for xb in 0..row_bytes {
                row[xb] = mand_byte(y, xb, n);
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

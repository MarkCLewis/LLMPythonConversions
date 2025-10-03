// The Computer Language Benchmarks Game
// https://salsa.debian.org/benchmarksgame-team/benchmarksgame/
//
// translated from Python to Rust by a large language model

use std::io::{stdout, Write, BufWriter};
use std::cmp::min;

const LINE_WIDTH: usize = 60;

const ALU: &str = "GGCCGGGCGCGGTGGCTCACGCCTGTAATCCCAGCACTTTGG\
                   GAGGCCGAGGCGGGCGGATCACCTGAGGTCAGGAGTTCGAGA\
                   CCAGCCTGGCCAACATGGTGAAACCCCGTCTCTACTAAAAAT\
                   ACAAAAATTAGCCGGGCGTGGTGGCGCGCGCCTGTAATCCCA\
                   GCTACTCGGGAGGCTGAGGCAGGAGAATCGCTTGAACCCGGG\
                   AGGCGGAGGTTGCAGTGAGCCGAGATCGCGCCACTGCACTCC\
                   AGCCTGGGCGACAGAGCGAGACTCCGTCTCAAAAA";

// IUB ambiguity codes and their probabilities
const IUB: &[(u8, f64)] = &[
    (b'a', 0.27), (b'c', 0.12), (b'g', 0.12), (b't', 0.27),
    (b'B', 0.02), (b'D', 0.02), (b'H', 0.02), (b'K', 0.02),
    (b'M', 0.02), (b'N', 0.02), (b'R', 0.02), (b'S', 0.02),
    (b'W', 0.02), (b'Y', 0.02), (b'V', 0.02),
];

// Homo sapiens DNA frequencies
const HOMOSAPIENS: &[(u8, f64)] = &[
    (b'a', 0.3029549426680),
    (b'c', 0.1979883004921),
    (b'g', 0.1975473066391),
    (b't', 0.3015094502008),
];

/// From a table of characters and their probabilities, generate cumulative probabilities.
/// This is used for fast random selection.
fn make_cumulative(table: &[(u8, f64)]) -> (Vec<f64>, Vec<u8>) {
    let mut probs = Vec::with_capacity(table.len());
    let mut chars = Vec::with_capacity(table.len());
    let mut p = 0.0;
    for &(char, prob) in table {
        p += prob;
        probs.push(p);
        chars.push(char);
    }
    // Ensure the last probability is exactly 1.0 to handle potential floating point inaccuracies
    if let Some(last_prob) = probs.last_mut() {
        *last_prob = 1.0;
    }
    (probs, chars)
}

/// Repeats the `src` sequence to a total length of `n`, printing it in lines of `LINE_WIDTH`.
fn repeat_fasta(src: &[u8], n: usize, writer: &mut impl Write) -> std::io::Result<()> {
    let mut output_bytes = Vec::with_capacity(n + n / LINE_WIDTH);
    output_bytes.extend(src.iter().cycle().take(n));
    
    for chunk in output_bytes.chunks(LINE_WIDTH) {
        writer.write_all(chunk)?;
        writer.write_all(b"\n")?;
    }
    Ok(())
}

// Constants for the Linear Congruential Generator (LCG)
const IM: f64 = 139968.0;
const IA: f64 = 3877.0;
const IC: f64 = 29573.0;
static mut SEED: f64 = 42.0;

/// Generates a random DNA sequence based on the provided probability table.
fn random_fasta(table: &[(u8, f64)], n: usize, writer: &mut impl Write) -> std::io::Result<()> {
    let (probs, chars) = make_cumulative(table);
    let mut line_buf = [0u8; LINE_WIDTH];
    
    let mut remaining = n;
    while remaining > 0 {
        let line_len = min(remaining, LINE_WIDTH);

        for i in 0..line_len {
            // This is unsafe because SEED is a mutable static, which is not thread-safe.
            // For this single-threaded benchmark, it's a direct translation of the Python logic.
            unsafe {
                SEED = (SEED * IA + IC) % IM;
                let r = SEED / IM;
                // Find the first cumulative probability that is >= our random number
                let index = probs.iter().position(|&p| p >= r).unwrap();
                line_buf[i] = chars[index];
            }
        }

        writer.write_all(&line_buf[..line_len])?;
        writer.write_all(b"\n")?;
        remaining -= line_len;
    }
    Ok(())
}

fn main() -> std::io::Result<()> {
    let n = std::env::args_os()
        .nth(1)
        .and_then(|s| s.into_string().ok())
        .and_then(|s| s.parse().ok())
        .unwrap_or(1000);

    let mut writer = BufWriter::new(stdout().lock());

    writer.write_all(b">ONE Homo sapiens alu\n")?;
    repeat_fasta(ALU.as_bytes(), n * 2, &mut writer)?;

    writer.write_all(b">TWO IUB ambiguity codes\n")?;
    random_fasta(&IUB, n * 3, &mut writer)?;

    writer.write_all(b">THREE Homo sapiens frequency\n")?;
    random_fasta(&HOMOSAPIENS, n * 5, &mut writer)?;
    
    writer.flush()?;
    Ok(())
}

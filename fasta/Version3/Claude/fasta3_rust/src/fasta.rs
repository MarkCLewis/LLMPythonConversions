// The Computer Language Benchmarks Game
// https://salsa.debian.org/benchmarksgame-team/benchmarksgame/
//
// Converted from Python to Rust by Claude

use std::env;
use std::io::{self, Write, BufWriter};

const ALU: &str = concat!(
    "GGCCGGGCGCGGTGGCTCACGCCTGTAATCCCAGCACTTTGG",
    "GAGGCCGAGGCGGGCGGATCACCTGAGGTCAGGAGTTCGAGA",
    "CCAGCCTGGCCAACATGGTGAAACCCCGTCTCTACTAAAAAT",
    "ACAAAAATTAGCCGGGCGTGGTGGCGCGCGCCTGTAATCCCA",
    "GCTACTCGGGAGGCTGAGGCAGGAGAATCGCTTGAACCCGGG",
    "AGGCGGAGGTTGCAGTGAGCCGAGATCGCGCCACTGCACTCC",
    "AGCCTGGGCGACAGAGCGAGACTCCGTCTCAAAAA"
);

const WIDTH: usize = 60;
const IM: f64 = 139968.0;

// Frequency tables
const IUB: [(&str, f64); 15] = [
    ("a", 0.27), ("c", 0.12), ("g", 0.12), ("t", 0.27),
    ("B", 0.02), ("D", 0.02), ("H", 0.02), ("K", 0.02),
    ("M", 0.02), ("N", 0.02), ("R", 0.02), ("S", 0.02),
    ("V", 0.02), ("W", 0.02), ("Y", 0.02)
];

const HOMOSAPIENS: [(&str, f64); 4] = [
    ("a", 0.3029549426680),
    ("c", 0.1979883004921),
    ("g", 0.1975473066391),
    ("t", 0.3015094502008)
];

// Convert frequency table to cumulative probabilities
fn make_cumulative(table: &[(&str, f64)]) -> (Vec<f64>, Vec<u8>) {
    let mut probs = Vec::with_capacity(table.len());
    let mut chars = Vec::with_capacity(table.len());
    let mut prob = 0.0;
    
    for &(ch, p) in table {
        prob += p;
        probs.push(prob);
        chars.push(ch.as_bytes()[0]);
    }
    
    (probs, chars)
}

// Binary search to find character based on probability
fn bisect(probs: &[f64], r: f64) -> usize {
    let mut low = 0;
    let mut high = probs.len() - 1;
    
    while low < high {
        let mid = (low + high) / 2;
        if r < probs[mid] {
            high = mid;
        } else {
            low = mid + 1;
        }
    }
    
    low
}

// Generate a repeated sequence
fn repeat_fasta<W: Write>(writer: &mut W, src: &str, n: usize) -> io::Result<()> {
    let src_bytes = src.as_bytes();
    let src_len = src.len();
    let mut line = Vec::with_capacity(WIDTH + 1);
    line.resize(WIDTH + 1, 0);
    line[WIDTH] = b'\n';
    
    let mut pos = 0;
    let mut remaining = n;
    
    while remaining > 0 {
        let line_len = remaining.min(WIDTH);
        
        // Fill the line buffer
        for i in 0..line_len {
            line[i] = src_bytes[(pos + i) % src_len];
        }
        
        // Write the line
        if line_len == WIDTH {
            writer.write_all(&line[..WIDTH + 1])?;
        } else {
            line[line_len] = b'\n';
            writer.write_all(&line[..line_len + 1])?;
        }
        
        pos = (pos + line_len) % src_len;
        remaining -= line_len;
    }
    
    Ok(())
}

// Generate a random sequence
fn random_fasta<W: Write>(
    writer: &mut W,
    probs: &[f64],
    chars: &[u8],
    n: usize,
    mut seed: f64
) -> io::Result<f64> {
    let mut line = Vec::with_capacity(WIDTH + 1);
    line.resize(WIDTH + 1, 0);
    line[WIDTH] = b'\n';
    
    let mut remaining = n;
    
    while remaining > 0 {
        let line_len = remaining.min(WIDTH);
        
        for i in 0..line_len {
            // Generate random number
            seed = (seed * 3877.0 + 29573.0) % IM;
            let r = seed / IM;
            
            // Find character based on probability
            let index = bisect(probs, r);
            line[i] = chars[index];
        }
        
        // Write the line
        if line_len == WIDTH {
            writer.write_all(&line[..WIDTH + 1])?;
        } else {
            line[line_len] = b'\n';
            writer.write_all(&line[..line_len + 1])?;
        }
        
        remaining -= line_len;
    }
    
    Ok(seed)
}

fn main() -> io::Result<()> {
    // Parse command-line argument
    let n = env::args()
        .nth(1)
        .unwrap_or_else(|| "1000".to_string())
        .parse::<usize>()
        .expect("Expected a number");
    
    // Use a buffered writer for better performance
    let stdout = io::stdout();
    let mut writer = BufWriter::with_capacity(64 * 1024, stdout.lock());
    
    // Create cumulative frequency tables
    let (iub_probs, iub_chars) = make_cumulative(&IUB);
    let (homosapiens_probs, homosapiens_chars) = make_cumulative(&HOMOSAPIENS);
    
    // Generate sequences
    writeln!(writer, ">ONE Homo sapiens alu")?;
    repeat_fasta(&mut writer, ALU, n * 2)?;
    
    writeln!(writer, ">TWO IUB ambiguity codes")?;
    let seed = random_fasta(&mut writer, &iub_probs, &iub_chars, n * 3, 42.0)?;
    
    writeln!(writer, ">THREE Homo sapiens frequency")?;
    random_fasta(&mut writer, &homosapiens_probs, &homosapiens_chars, n * 5, seed)?;
    
    Ok(())
}
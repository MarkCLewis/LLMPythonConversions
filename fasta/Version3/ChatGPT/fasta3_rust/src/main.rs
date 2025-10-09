// The Computer Language Benchmarks Game
// https://salsa.debian.org/benchmarksgame-team/benchmarksgame/
//
// Translated from Python fasta3.py by ChatGPT
//
// Run with: cargo run --release -- 25000000 > out.txt

use std::env;
use std::io::{self, Write};

const WIDTH: usize = 60;

const ALU: &str = concat!(
    "GGCCGGGCGCGGTGGCTCACGCCTGTAATCCCAGCACTTTGG",
    "GAGGCCGAGGCGGGCGGATCACCTGAGGTCAGGAGTTCGAGA",
    "CCAGCCTGGCCAACATGGTGAAACCCCGTCTCTACTAAAAAT",
    "ACAAAAATTAGCCGGGCGTGGTGGCGCGCGCCTGTAATCCCA",
    "GCTACTCGGGAGGCTGAGGCAGGAGAATCGCTTGAACCCGGG",
    "AGGCGGAGGTTGCAGTGAGCCGAGATCGCGCCACTGCACTCC",
    "AGCCTGGGCGACAGAGCGAGACTCCGTCTCAAAAA"
);

static IUB: [(&str, f64); 15] = [
    ("a", 0.27), ("c", 0.12), ("g", 0.12), ("t", 0.27),
    ("B", 0.02), ("D", 0.02), ("H", 0.02), ("K", 0.02),
    ("M", 0.02), ("N", 0.02), ("R", 0.02), ("S", 0.02),
    ("V", 0.02), ("W", 0.02), ("Y", 0.02),
];

static HOMO_SAPIENS: [(&str, f64); 4] = [
    ("a", 0.3029549426680),
    ("c", 0.1979883004921),
    ("g", 0.1975473066391),
    ("t", 0.3015094502008),
];

// Convert a probability table into cumulative distribution
fn make_cumulative(table: &[(&str, f64)]) -> (Vec<f64>, Vec<u8>) {
    let mut probs = Vec::with_capacity(table.len());
    let mut chars = Vec::with_capacity(table.len());
    let mut sum = 0.0;
    for (ch, p) in table {
        sum += p;
        probs.push(sum);
        chars.push(ch.as_bytes()[0]);
    }
    (probs, chars)
}

// Linear congruential generator
fn gen_random(seed: &mut f64) -> f64 {
    *seed = (*seed * 3877.0 + 29573.0) % 139968.0;
    *seed / 139968.0
}

fn repeat_fasta(src: &str, n: usize, out: &mut impl Write) -> io::Result<()> {
    let bytes = src.as_bytes();
    let mut pos = 0;
    let mut remaining = n;
    while remaining > 0 {
        let line_len = remaining.min(WIDTH);
        for i in 0..line_len {
            out.write_all(&[bytes[pos]])?;
            pos += 1;
            if pos >= bytes.len() {
                pos = 0;
            }
        }
        out.write_all(b"\n")?;
        remaining -= line_len;
    }
    Ok(())
}

fn random_fasta(
    table: &[(&str, f64)],
    n: usize,
    seed: &mut f64,
    out: &mut impl Write,
) -> io::Result<()> {
    let (probs, chars) = make_cumulative(table);

    let mut remaining = n;
    let mut line = [0u8; WIDTH];

    while remaining > 0 {
        let line_len = remaining.min(WIDTH);
        for i in 0..line_len {
            let r = gen_random(seed);
            let j = probs.iter().position(|&p| r < p).unwrap_or(probs.len() - 1);
            line[i] = chars[j];
        }
        out.write_all(&line[..line_len])?;
        out.write_all(b"\n")?;
        remaining -= line_len;
    }
    Ok(())
}

fn main() -> io::Result<()> {
    let args: Vec<String> = env::args().collect();
    if args.len() < 2 {
        eprintln!("Usage: {} <n>", args[0]);
        return Ok(());
    }
    let n: usize = args[1].parse().unwrap();
    let mut seed = 42.0;
    let stdout = io::stdout();
    let mut out = stdout.lock();

    writeln!(out, ">ONE Homo sapiens alu")?;
    repeat_fasta(ALU, n * 2, &mut out)?;

    writeln!(out, ">TWO IUB ambiguity codes")?;
    random_fasta(&IUB, n * 3, &mut seed, &mut out)?;

    writeln!(out, ">THREE Homo sapiens frequency")?;
    random_fasta(&HOMO_SAPIENS, n * 5, &mut seed, &mut out)?;

    Ok(())
}

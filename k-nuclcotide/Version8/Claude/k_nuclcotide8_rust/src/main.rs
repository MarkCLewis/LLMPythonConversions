// The Computer Language Benchmarks Game
// https://salsa.debian.org/benchmarksgame-team/benchmarksgame/
//
// Converted from Python to Rust by Claude

use std::collections::HashMap;
use std::io::{self, BufRead, BufReader};
use std::cmp::Reverse;

// Read sequence from stdin
fn read_sequence() -> String {
    let stdin = io::stdin();
    let mut reader = BufReader::new(stdin.lock());
    let mut line = String::new();
    let mut found_header = false;

    // Find the ">THREE" header
    while reader.read_line(&mut line).unwrap() > 0 {
        if line.starts_with(">THREE") {
            found_header = true;
            line.clear();
            break;
        }
        line.clear();
    }

    // Read sequence lines until next header or EOF
    let mut sequence_lines = Vec::new();
    if found_header {
        while reader.read_line(&mut line).unwrap() > 0 {
            if line.starts_with('>') {
                break;
            }
            sequence_lines.push(line[..line.len() - 1].to_string()); // Remove newline
            line.clear();
        }
    }

    // Join lines and convert to uppercase
    sequence_lines.join("").to_uppercase()
}

// Count k-mers of specified length
fn base_counts(bases: usize, sequence: &str) -> HashMap<String, usize> {
    let mut counts = HashMap::new();
    let size = sequence.len() + 1 - bases;

    for i in 0..size {
        let nucleo = &sequence[i..i + bases];
        *counts.entry(nucleo.to_string()).or_insert(0) += 1;
    }

    counts
}

// Calculate and sort frequencies of k-mers
fn sorted_freq(bases: usize, sequence: &str) -> Vec<(String, f64)> {
    let counts = base_counts(bases, sequence);
    let size = sequence.len() + 1 - bases;

    let mut result: Vec<_> = counts.into_iter().collect();
    result.sort_by_key(|(_, count)| Reverse(*count));

    result
        .into_iter()
        .map(|(key, count)| (key, 100.0 * count as f64 / size as f64))
        .collect()
}

// Get count of a specific k-mer
fn specific_count(code: &str, sequence: &str) -> usize {
    let counts = base_counts(code.len(), sequence);
    counts.get(code).copied().unwrap_or(0)
}

fn main() {
    // Read sequence from stdin
    let sequence = read_sequence();

    // Calculate and print frequencies for 1-mers and 2-mers
    for bases in 1..=2 {
        let freq_results = sorted_freq(bases, &sequence);
        for (key, freq) in &freq_results {
            println!("{} {:.3}", key, freq);
        }
        println!();
    }

    // Calculate and print counts for specific sequences
    let specific_codes = [
        "GGT", "GGTA", "GGTATT", 
        "GGTATTTTAATT", "GGTATTTTAATTTATAGT"
    ];

    for code in &specific_codes {
        let count = specific_count(code, &sequence);
        println!("{}\t{}", count, code);
    }
}
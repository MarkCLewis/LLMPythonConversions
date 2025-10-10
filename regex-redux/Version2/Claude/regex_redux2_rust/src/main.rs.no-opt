// The Computer Language Benchmarks Game
// https://salsa.debian.org/benchmarksgame-team/benchmarksgame/
//
// Converted from Python to Rust by Claude

use pcre2::bytes::Regex;
use rayon::prelude::*;
use std::io::{self, Read};
use std::sync::Arc;

// Function to perform regex substitution
fn replace(pattern: &str, replacement: &str, src: &[u8]) -> Vec<u8> {
    let regex = Regex::new(pattern).expect("Invalid regex pattern");
    regex.replace_all(src, replacement.as_bytes()).into_owned()
}

// Function to count pattern matches
fn count_matches(pattern: &str, src: &[u8]) -> usize {
    let regex = Regex::new(pattern).expect("Invalid regex pattern");
    regex.find_iter(src).count()
}

fn main() -> io::Result<()> {
    // Read the entire input
    let mut input = Vec::new();
    io::stdin().read_to_end(&mut input)?;
    let input_length = input.len();
    
    // Remove sequence descriptions and newlines
    let sequences = replace(r">.*\n|\n", "", &input);
    let sequences_length = sequences.len();
    
    // Free the input to save memory
    drop(input);
    
    // Create shared reference to sequences
    let sequences_arc = Arc::new(sequences);
    
    // Define patterns to count
    let count_patterns = vec![
        "agggtaaa|tttaccct",
        "[cgt]gggtaaa|tttaccc[acg]",
        "a[act]ggtaaa|tttacc[agt]t",
        "ag[act]gtaaa|tttac[agt]ct",
        "agg[act]taaa|ttta[agt]cct",
        "aggg[acg]aaa|ttt[cgt]ccct",
        "agggt[cgt]aa|tt[acg]accct",
        "agggta[cgt]a|t[acg]taccct",
        "agggtaa[cgt]|[acg]ttaccct"
    ];
    
    // Count matches in parallel
    let counts: Vec<(String, usize)> = count_patterns.par_iter()
        .map(|pattern| {
            let count = count_matches(pattern, &sequences_arc);
            (pattern.to_string(), count)
        })
        .collect();
    
    // Define substitution patterns
    let replace_patterns = [
        ("tHa[Nt]", "<4>"),
        ("aND|caN|Ha[DS]|WaS", "<3>"),
        ("a[NSt]|BY", "<2>"),
        ("<[^>]*>", "|"),
        ("\\|[^|][^|]*\\|", "-")
    ];
    
    // Perform replacements sequentially
    let mut result = Arc::try_unwrap(sequences_arc).expect("Arc still has multiple owners");
    
    for (pattern, replacement) in &replace_patterns {
        result = replace(pattern, replacement, &result);
    }
    
    // Print the match counts
    for (pattern, count) in counts {
        println!("{} {}", pattern, count);
    }
    
    // Print the lengths
    println!();
    println!("{}", input_length);
    println!("{}", sequences_length);
    println!("{}", result.len());
    
    Ok(())
}
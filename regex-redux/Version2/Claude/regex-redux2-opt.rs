// The Computer Language Benchmarks Game
// https://salsa.debian.org/benchmarksgame-team/benchmarksgame/
//
// Converted from Python to Rust with optimizations by Claude

use pcre2::bytes::{Regex, RegexBuilder};
use rayon::prelude::*;
use std::io::{self, Read, BufReader};
use std::time::Instant;
use std::sync::Arc;

// Function to perform regex substitution with pre-compiled regex
fn replace(regex: &Regex, replacement: &[u8], src: &[u8]) -> Vec<u8> {
    regex.replace_all(src, replacement).into_owned()
}

// Function to count pattern matches with pre-compiled regex
fn count_matches(regex: &Regex, src: &[u8]) -> usize {
    regex.find_iter(src).count()
}

// Pre-compile regex pattern with JIT optimization
fn compile_regex(pattern: &str) -> Regex {
    RegexBuilder::new()
        .jit(true)
        .compile(pattern)
        .unwrap_or_else(|e| panic!("Invalid regex pattern '{}': {}", pattern, e))
}

fn main() -> io::Result<()> {
    // Read the entire input efficiently
    let stdin = io::stdin();
    let mut reader = BufReader::with_capacity(1024 * 1024, stdin.lock());
    let mut input = Vec::with_capacity(50 * 1024 * 1024); // Pre-allocate 50MB (adjust as needed)
    reader.read_to_end(&mut input)?;
    let input_length = input.len();
    
    // Pre-compile the header/newline removal regex
    let header_newline_regex = compile_regex(r">.*\n|\n");
    
    // Remove sequence descriptions and newlines
    let sequences = replace(&header_newline_regex, &[], &input);
    let sequences_length = sequences.len();
    
    // Free the input to save memory
    drop(input);
    
    // Define and pre-compile patterns to count
    let count_patterns = [
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
    
    // Pre-compile all counting regexes
    let compiled_count_regexes: Vec<(String, Regex)> = count_patterns
        .iter()
        .map(|pattern| {
            (pattern.to_string(), compile_regex(pattern))
        })
        .collect();
    
    // Create shared reference to sequences
    let sequences_arc = Arc::new(sequences);
    
    // Count matches in parallel with pre-compiled regexes
    let counts: Vec<(String, usize)> = compiled_count_regexes.par_iter()
        .map(|(pattern, regex)| {
            let count = count_matches(regex, &sequences_arc);
            (pattern.clone(), count)
        })
        .collect();
    
    // Define and pre-compile substitution patterns
    let replace_patterns = [
        ("tHa[Nt]", "<4>"),
        ("aND|caN|Ha[DS]|WaS", "<3>"),
        ("a[NSt]|BY", "<2>"),
        ("<[^>]*>", "|"),
        ("\\|[^|][^|]*\\|", "-")
    ];
    
    let compiled_replace_regexes: Vec<(Regex, Vec<u8>)> = replace_patterns
        .iter()
        .map(|(pattern, replacement)| {
            (compile_regex(pattern), replacement.as_bytes().to_vec())
        })
        .collect();
    
    // Perform replacements sequentially with pre-compiled regexes
    let mut result = Arc::try_unwrap(sequences_arc).expect("Arc still has multiple owners");
    
    for (regex, replacement) in &compiled_replace_regexes {
        result = replace(regex, replacement, &result);
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
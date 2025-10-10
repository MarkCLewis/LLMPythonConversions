// The Computer Language Benchmarks Game
// https://salsa.debian.org/benchmarksgame-team/benchmarksgame/
//
// Converted from Python to Rust with optimizations by Claude

use regex::Regex;
use std::collections::HashMap;
use std::io::{self, Read, BufReader};
use std::sync::Arc;
use rayon::prelude::*;
use std::time::Instant;
use regex::bytes::Regex as BytesRegex;

// Use pre-compiled regex for better performance
fn count_matches(re: &Regex, sequence: &str) -> usize {
    re.find_iter(sequence).count()
}

// Use bytes regex for faster header removal
fn remove_headers_newlines(input: &[u8]) -> Vec<u8> {
    let re = BytesRegex::new(r">.*\n|\n").expect("Invalid regex pattern");
    re.replace_all(input, &b""[..]).to_vec()
}

fn main() -> io::Result<()> {
    // Read the entire input efficiently using BufReader
    let stdin = io::stdin();
    let mut reader = BufReader::new(stdin.lock());
    let mut seq_bytes = Vec::new();
    reader.read_to_end(&mut seq_bytes)?;
    let ilen = seq_bytes.len();
    
    // Clean sequence (remove headers and newlines)
    let clean_seq_bytes = remove_headers_newlines(&seq_bytes);
    let clen = clean_seq_bytes.len();
    
    // Convert to string once (avoid multiple conversions)
    let seq_str = String::from_utf8(clean_seq_bytes)
        .expect("Input was not valid UTF-8");
    
    // Free the original bytes to save memory
    drop(seq_bytes);
    
    // Define patterns and pre-compile regexes
    let variants = vec![
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
    
    // Precompile all regexes
    let compiled_regexes: Vec<(String, Regex)> = variants.into_iter()
        .map(|pattern| {
            let regex = Regex::new(pattern).expect("Invalid regex pattern");
            (pattern.to_string(), regex)
        })
        .collect();
    
    // Share the sequence across threads
    let seq_arc = Arc::new(seq_str);
    
    // Count matches in parallel with pre-compiled regexes
    let counts: Vec<(String, usize)> = compiled_regexes.par_iter()
        .map(|(pattern, regex)| {
            let pattern_str = pattern.clone();
            let count = count_matches(regex, &seq_arc);
            (pattern_str, count)
        })
        .collect();
    
    // Print pattern counts
    for (pattern, count) in &counts {
        println!("{} {}", pattern, count);
    }
    
    // Define substitution patterns and precompile them
    let mut seq_final = Arc::try_unwrap(seq_arc).expect("Arc still has multiple owners");
    let subst: Vec<(Regex, &str)> = vec![
        (Regex::new(r"tHa[Nt]").unwrap(), "<4>"),
        (Regex::new(r"aND|caN|Ha[DS]|WaS").unwrap(), "<3>"),
        (Regex::new(r"a[NSt]|BY").unwrap(), "<2>"),
        (Regex::new(r"<[^>]*>").unwrap(), "|"),
        (Regex::new(r"\|[^|][^|]*\|").unwrap(), "-")
    ];
    
    // Apply substitutions
    for (pattern, replacement) in subst {
        // Use in-place replacement where possible
        seq_final = pattern.replace_all(&seq_final, replacement).to_string();
    }
    
    // Print results
    println!();
    println!("{}", ilen);
    println!("{}", clen);
    println!("{}", seq_final.len());
    
    Ok(())
}
// The Computer Language Benchmarks Game
// https://salsa.debian.org/benchmarksgame-team/benchmarksgame/
//
// Converted from Python to Rust with optimizations by Claude

use std::io::{self, BufRead, BufReader};
use std::cmp::Reverse;
use rayon::prelude::*;
use rustc_hash::FxHashMap;
use std::sync::{Arc, Mutex};

// Function to encode a k-mer as an integer (supports up to 32 nucleotides)
#[inline]
fn encode_kmer(kmer: &[u8], k: usize) -> u64 {
    let mut code = 0u64;
    for i in 0..k {
        let nucleotide = match kmer[i] {
            b'A' => 0,
            b'C' => 1,
            b'G' => 2,
            b'T' => 3,
            _ => 0, // Default case for invalid input
        };
        code = (code << 2) | nucleotide;
    }
    code
}

// Function to decode an integer back to a k-mer
fn decode_kmer(code: u64, k: usize) -> String {
    let mut result = Vec::with_capacity(k);
    let nucleotides = [b'A', b'C', b'G', b'T'];
    
    let mut temp_code = code;
    for _ in 0..k {
        let nucleotide = nucleotides[(temp_code & 0x3) as usize];
        result.push(nucleotide);
        temp_code >>= 2;
    }
    
    // Reverse the result as we decoded from least significant bit
    result.reverse();
    
    // Convert bytes to string
    String::from_utf8(result).unwrap()
}

// Read sequence from stdin with optimized buffering
fn read_sequence() -> Vec<u8> {
    let stdin = io::stdin();
    let mut reader = BufReader::with_capacity(1024 * 1024, stdin.lock());
    let mut line = String::with_capacity(1024);
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
    
    // Preallocate a large buffer for the sequence
    let mut sequence = Vec::with_capacity(10_000_000);
    
    // Read sequence lines until next header or EOF
    if found_header {
        while reader.read_line(&mut line).unwrap() > 0 {
            if line.starts_with('>') {
                break;
            }
            
            // Add only valid nucleotide characters, convert to uppercase
            for &byte in line.trim_end().as_bytes() {
                match byte {
                    b'A' | b'C' | b'G' | b'T' => sequence.push(byte),
                    b'a' | b'c' | b'g' | b't' => sequence.push(byte - 32), // Convert to uppercase
                    _ => {} // Skip other characters
                }
            }
            
            line.clear();
        }
    }
    
    sequence
}

// Count k-mers using parallel processing
fn count_kmers_parallel(sequence: &[u8], k: usize) -> FxHashMap<u64, usize> {
    // For small sequences or large k values, use sequential counting
    if sequence.len() < 1_000_000 || k > 10 {
        return count_kmers_sequential(sequence, k);
    }
    
    // Determine chunk size for parallel processing
    let num_threads = rayon::current_num_threads();
    let chunk_size = sequence.len() / num_threads;
    
    // Shared result map with mutex
    let result_map = Arc::new(Mutex::new(FxHashMap::default()));
    
    // Process chunks in parallel
    sequence.par_chunks(chunk_size)
        .enumerate()
        .for_each(|(i, chunk)| {
            let mut local_map = FxHashMap::default();
            
            // Determine effective end to handle chunks that cross boundaries
            let effective_end = if i == num_threads - 1 {
                chunk.len()
            } else {
                // Ensure we don't process incomplete k-mers at chunk boundaries
                chunk.len().saturating_sub(k - 1)
            };
            
            // Process each k-mer in the chunk
            for j in 0..effective_end.saturating_sub(k - 1) {
                let kmer_bytes = &chunk[j..j + k];
                let kmer_code = encode_kmer(kmer_bytes, k);
                *local_map.entry(kmer_code).or_insert(0) += 1;
            }
            
            // Handle k-mers that cross chunk boundaries
            if i < num_threads - 1 {
                for j in chunk.len().saturating_sub(k - 1)..chunk.len() {
                    if j + k <= sequence.len() {
                        // Create a temporary buffer for the cross-boundary k-mer
                        let mut kmer_bytes = Vec::with_capacity(k);
                        kmer_bytes.extend_from_slice(&chunk[j..]);
                        let remaining = k - (chunk.len() - j);
                        let next_chunk_start = (i + 1) * chunk_size;
                        if next_chunk_start + remaining <= sequence.len() {
                            kmer_bytes.extend_from_slice(&sequence[next_chunk_start..next_chunk_start + remaining]);
                            let kmer_code = encode_kmer(&kmer_bytes, k);
                            *local_map.entry(kmer_code).or_insert(0) += 1;
                        }
                    }
                }
            }
            
            // Merge local results into shared map
            let mut result = result_map.lock().unwrap();
            for (kmer, count) in local_map {
                *result.entry(kmer).or_insert(0) += count;
            }
        });
    
    // Convert Arc<Mutex<HashMap>> back to HashMap
    Arc::try_unwrap(result_map).unwrap().into_inner().unwrap()
}

// Sequential version for small sequences or large k values
fn count_kmers_sequential(sequence: &[u8], k: usize) -> FxHashMap<u64, usize> {
    let mut counts = FxHashMap::default();
    let size = sequence.len() + 1 - k;
    
    for i in 0..size {
        let kmer = encode_kmer(&sequence[i..i + k], k);
        *counts.entry(kmer).or_insert(0) += 1;
    }
    
    counts
}

// Calculate and sort frequencies of k-mers
fn sorted_freq(bases: usize, sequence: &[u8]) -> Vec<(String, f64)> {
    let counts = count_kmers_parallel(sequence, bases);
    let size = sequence.len() + 1 - bases;
    
    // Convert to vec for sorting
    let mut result: Vec<_> = counts.into_iter()
        .map(|(kmer_code, count)| (decode_kmer(kmer_code, bases), count))
        .collect();
    
    // Sort by count (descending)
    result.sort_by_key(|(_, count)| Reverse(*count));
    
    // Calculate frequencies
    result.into_iter()
        .map(|(key, count)| (key, 100.0 * count as f64 / size as f64))
        .collect()
}

// Get count of a specific k-mer
fn specific_count(code_str: &str, sequence: &[u8]) -> usize {
    let k = code_str.len();
    let code_bytes = code_str.as_bytes();
    
    if sequence.len() < k {
        return 0;
    }
    
    let code = encode_kmer(code_bytes, k);
    let counts = count_kmers_parallel(sequence, k);
    
    counts.get(&code).copied().unwrap_or(0)
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
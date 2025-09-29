// The Computer Language Benchmarks Game
// https://salsa.debian.org/benchmarksgame-team/benchmarksgame/
//
// Converted from Python to Rust with optimizations by Claude

use std::collections::HashMap;
use std::io::{self, BufRead};
use std::sync::{Arc, Mutex};
use rayon::prelude::*;
use rustc_hash::FxHashMap;
use bytecount;

// Efficient nucleotide encoding
const fn nucleotide_to_u8(c: u8) -> u8 {
    match c {
        b'A' | b'a' => 0,
        b'C' | b'c' => 1,
        b'G' | b'g' => 2,
        b'T' | b't' => 3,
        _ => 0, // Default for invalid input
    }
}

// K-mer encoding to a 64-bit integer (for k <= 32)
fn encode_kmer(sequence: &[u8], start: usize, k: usize) -> u64 {
    let mut key = 0u64;
    for i in 0..k {
        key = (key << 2) | (nucleotide_to_u8(sequence[start + i]) as u64);
    }
    key
}

// Decode a 64-bit integer back to a nucleotide sequence
fn decode_kmer(key: u64, k: usize) -> Vec<u8> {
    let mut result = vec![0; k];
    let nucleotides = [b'A', b'C', b'G', b'T'];
    
    let mut temp_key = key;
    for i in (0..k).rev() {
        result[i] = nucleotides[(temp_key & 0x3) as usize];
        temp_key >>= 2;
    }
    
    result
}

// An optimized hash map for k-mers using integer keys
type KmerIntMap = FxHashMap<u64, usize>;

// Function to read the input sequence from stdin
fn read_sequence() -> Vec<u8> {
    let stdin = io::stdin();
    let mut reader = stdin.lock();
    let mut sequence = Vec::with_capacity(10_000_000); // Pre-allocate memory
    let mut line = String::new();
    let mut found_header = false;

    // Read until we find the header
    while reader.read_line(&mut line).unwrap() > 0 {
        if line.starts_with(">THREE") {
            found_header = true;
            line.clear();
            break;
        }
        line.clear();
    }

    // Read the sequence data
    if found_header {
        while reader.read_line(&mut line).unwrap() > 0 {
            if line.starts_with('>') {
                break;
            }
            
            // Add only valid nucleotide characters (upper case)
            for &b in line.trim_end().as_bytes() {
                match b {
                    b'A' | b'C' | b'G' | b'T' => sequence.push(b),
                    b'a' | b'c' | b'g' | b't' => sequence.push(b - 32), // Convert to uppercase
                    _ => {} // Skip non-nucleotide characters
                }
            }
            
            line.clear();
        }
    }

    sequence
}

// Parallel k-mer counting using rayon with optimized encoding
fn count_kmers(sequence: &[u8], k: usize) -> KmerIntMap {
    // Use a single thread for small inputs
    if sequence.len() < 1_000_000 || k > 6 {
        return count_kmers_sequential(sequence, k);
    }
    
    // Determine optimal chunk size
    let num_cpus = rayon::current_num_threads();
    let chunk_size = (sequence.len() / num_cpus).max(k * 100);
    
    // Create chunks with overlap to handle k-mers that cross chunk boundaries
    let chunks: Vec<_> = (0..num_cpus)
        .map(|i| {
            let start = if i == 0 { 0 } else { i * chunk_size - (k - 1) };
            let end = if i == num_cpus - 1 { 
                sequence.len() 
            } else { 
                (i + 1) * chunk_size 
            };
            (start, end)
        })
        .collect();
    
    // Process chunks in parallel
    let results: Vec<KmerIntMap> = chunks.par_iter()
        .map(|&(start, end)| {
            let mut local_map = FxHashMap::default();
            
            // Skip overlapping k-mers from previous chunk
            let effective_start = if start == 0 { 0 } else { k - 1 };
            
            for i in effective_start..end.min(sequence.len() - k + 1) {
                let kmer_key = encode_kmer(sequence, start + i, k);
                *local_map.entry(kmer_key).or_insert(0) += 1;
            }
            
            local_map
        })
        .collect();
    
    // Merge results
    let mut final_map = FxHashMap::default();
    for map in results {
        for (kmer, count) in map {
            *final_map.entry(kmer).or_insert(0) += count;
        }
    }
    
    final_map
}

// Sequential k-mer counting for small sequences or large k
fn count_kmers_sequential(sequence: &[u8], k: usize) -> KmerIntMap {
    let mut map = FxHashMap::default();
    
    for i in 0..sequence.len() - k + 1 {
        let kmer_key = encode_kmer(sequence, i, k);
        *map.entry(kmer_key).or_insert(0) += 1;
    }
    
    map
}

// Format and print k-mer frequencies
fn print_frequencies(map: &KmerIntMap, sequence_len: usize, k: usize, sort: bool) {
    let total_kmers = sequence_len - k + 1;
    
    // Convert map to a vector for sorting
    let mut kmer_counts: Vec<_> = map.iter().collect();
    
    if sort {
        // Sort by count (descending) then by k-mer (ascending)
        kmer_counts.sort_by(|a, b| {
            b.1.cmp(a.1).then_with(|| a.0.cmp(b.0))
        });
    }
    
    // Print frequencies
    for (&kmer_key, &count) in &kmer_counts {
        let kmer_bytes = decode_kmer(kmer_key, k);
        let kmer_str = String::from_utf8_lossy(&kmer_bytes);
        let percentage = 100.0 * count as f64 / total_kmers as f64;
        println!("{} {:.3}", kmer_str, percentage);
    }
    println!();
}

// Print counts for specific k-mers
fn print_kmer_count(sequence: &[u8], kmer_str: &str) {
    let k = kmer_str.len();
    let kmer_map = count_kmers(sequence, k);
    
    // Encode the target k-mer
    let kmer_bytes = kmer_str.as_bytes();
    let mut kmer_key = 0u64;
    for i in 0..k {
        kmer_key = (kmer_key << 2) | (nucleotide_to_u8(kmer_bytes[i]) as u64);
    }
    
    let count = kmer_map.get(&kmer_key).unwrap_or(&0);
    println!("{}\t{}", count, kmer_str);
}

// Count occurrences of mono-nucleotides efficiently
fn count_mono_nucleotides(sequence: &[u8]) -> [usize; 4] {
    let mut counts = [0; 4];
    
    // Count each nucleotide
    counts[0] = bytecount::count(sequence, b'A');
    counts[1] = bytecount::count(sequence, b'C');
    counts[2] = bytecount::count(sequence, b'G');
    counts[3] = bytecount::count(sequence, b'T');
    
    counts
}

// Print mono-nucleotide frequencies
fn print_mono_frequencies(sequence: &[u8]) {
    let counts = count_mono_nucleotides(sequence);
    let total = sequence.len();
    
    let nucleotides = [b'A', b'C', b'G', b'T'];
    
    // Sort by count (descending)
    let mut indices = [0, 1, 2, 3];
    indices.sort_by(|&a, &b| counts[b].cmp(&counts[a]));
    
    for &i in &indices {
        let percentage = 100.0 * counts[i] as f64 / total as f64;
        println!("{} {:.3}", nucleotides[i] as char, percentage);
    }
    println!();
}

fn main() {
    // Read the DNA sequence
    let sequence = read_sequence();
    
    // Process mono-nucleotides (1-mers)
    print_mono_frequencies(&sequence);
    
    // Process di-nucleotides (2-mers)
    let di_map = count_kmers(&sequence, 2);
    print_frequencies(&di_map, sequence.len(), 2, true);
    
    // Process specific k-mers
    let specific_kmers = [
        "GGT", "GGTA", "GGTATT", "GGTATTTTAATT", "GGTATTTTAATTTATAGT"
    ];
    
    // Count and print each specific k-mer
    for &kmer in &specific_kmers {
        print_kmer_count(&sequence, kmer);
    }
}
// The Computer Language Benchmarks Game
// https://salsa.debian.org/benchmarksgame-team/benchmarksgame/
//
// Converted from Python to Rust by Claude

use std::collections::HashMap;
use std::io::{self, BufRead, Read};
use std::sync::{Arc, Mutex};
use std::thread;
use rayon::prelude::*;

type KmerMap = HashMap<Vec<u8>, usize>;

// Function to read the input sequence from stdin
fn read_sequence() -> Vec<u8> {
    let stdin = io::stdin();
    let mut reader = stdin.lock();
    let mut sequence = Vec::new();
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
            
            // Add only valid nucleotide characters
            for &b in line.trim_end().as_bytes() {
                match b {
                    b'A' | b'C' | b'G' | b'T' | b'a' | b'c' | b'g' | b't' => {
                        sequence.push(b.to_ascii_uppercase());
                    },
                    _ => {} // Skip non-nucleotide characters
                }
            }
            
            line.clear();
        }
    }

    sequence
}

// Count k-mer frequencies in a given sequence range
fn count_kmers(
    sequence: &[u8],
    start: usize,
    end: usize,
    k: usize,
    result_map: Arc<Mutex<KmerMap>>
) {
    let mut local_map = HashMap::new();
    
    // Process k-mers in the assigned range
    for i in start..end.min(sequence.len() - k + 1) {
        let kmer = &sequence[i..i+k];
        *local_map.entry(kmer.to_vec()).or_insert(0) += 1;
    }
    
    // Merge local results into the shared map
    let mut result = result_map.lock().unwrap();
    for (kmer, count) in local_map {
        *result.entry(kmer).or_insert(0) += count;
    }
}

// Parallel k-mer counting using multiple threads
fn parallel_count_kmers(sequence: &[u8], k: usize) -> KmerMap {
    let num_cpus = num_cpus::get();
    let chunk_size = (sequence.len() - k + 1) / num_cpus;
    let result_map = Arc::new(Mutex::new(HashMap::new()));
    
    let mut handles = Vec::new();
    
    // Create and start worker threads
    for i in 0..num_cpus {
        let start = i * chunk_size;
        let end = if i == num_cpus - 1 {
            sequence.len()
        } else {
            (i + 1) * chunk_size
        };
        
        let sequence = sequence.to_vec();
        let result_map = Arc::clone(&result_map);
        
        let handle = thread::spawn(move || {
            count_kmers(&sequence, start, end, k, result_map);
        });
        
        handles.push(handle);
    }
    
    // Wait for all threads to complete
    for handle in handles {
        handle.join().unwrap();
    }
    
    Arc::try_unwrap(result_map).unwrap().into_inner().unwrap()
}

// Alternative implementation using Rayon for parallel processing
fn rayon_count_kmers(sequence: &[u8], k: usize) -> KmerMap {
    let num_cpus = num_cpus::get();
    let chunk_size = (sequence.len() - k + 1) / num_cpus;
    
    // Create chunks for parallel processing
    let chunks: Vec<_> = (0..num_cpus)
        .map(|i| {
            let start = i * chunk_size;
            let end = if i == num_cpus - 1 {
                sequence.len()
            } else {
                (i + 1) * chunk_size
            };
            (start, end)
        })
        .collect();
    
    // Process chunks in parallel and collect results
    let results: Vec<KmerMap> = chunks.par_iter()
        .map(|&(start, end)| {
            let mut local_map = HashMap::new();
            for i in start..end.min(sequence.len() - k + 1) {
                let kmer = &sequence[i..i+k];
                *local_map.entry(kmer.to_vec()).or_insert(0) += 1;
            }
            local_map
        })
        .collect();
    
    // Merge results from all chunks
    let mut final_map = HashMap::new();
    for map in results {
        for (kmer, count) in map {
            *final_map.entry(kmer).or_insert(0) += count;
        }
    }
    
    final_map
}

// Format and print k-mer frequencies
fn print_frequencies(map: &KmerMap, sequence_len: usize, k: usize, sort: bool) {
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
    for (kmer, &count) in &kmer_counts {
        let percentage = 100.0 * count as f64 / total_kmers as f64;
        
        // Convert kmer to string for display
        let kmer_str = String::from_utf8_lossy(kmer);
        println!("{} {:.3}", kmer_str, percentage);
    }
    println!();
}

// Print counts for specific k-mers
fn print_kmer_counts(map: &KmerMap, kmers: &[&str]) {
    for &kmer_str in kmers {
        let kmer = kmer_str.as_bytes().to_vec();
        let count = map.get(&kmer).unwrap_or(&0);
        println!("{}\t{}", count, kmer_str);
    }
}

fn main() {
    // Read the DNA sequence
    let sequence = read_sequence();
    
    // Process mono-nucleotides (1-mers)
    let mono_map = rayon_count_kmers(&sequence, 1);
    print_frequencies(&mono_map, sequence.len(), 1, true);
    
    // Process di-nucleotides (2-mers)
    let di_map = rayon_count_kmers(&sequence, 2);
    print_frequencies(&di_map, sequence.len(), 2, true);
    
    // Process specific k-mers
    let specific_kmers = [
        "GGT", "GGTA", "GGTATT", "GGTATTTTAATT", "GGTATTTTAATTTATAGT"
    ];
    
    // Count each k-mer length separately for better performance
    for &kmer in &specific_kmers {
        let k = kmer.len();
        let kmer_map = rayon_count_kmers(&sequence, k);
        let count = kmer_map.get(&kmer.as_bytes().to_vec()).unwrap_or(&0);
        println!("{}\t{}", count, kmer);
    }
}
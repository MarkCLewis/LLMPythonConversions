use rayon::prelude::*;
use std::collections::HashMap;
use std::io::{self, BufRead};

/// Reads the input stream, finds the ">THREE" sequence,
/// and returns it as a single byte vector in uppercase.
fn read_sequence() -> Vec<u8> {
    let stdin = io::stdin();
    let mut handle = stdin.lock();

    // Find the start of the sequence
    let mut line_buf = String::new();
    loop {
        handle.read_line(&mut line_buf).unwrap();
        if line_buf.starts_with(">THREE") {
            break;
        }
        line_buf.clear();
    }

    // Read the sequence lines until the next header
    let mut sequence_bytes = Vec::with_capacity(130_000_000); // Pre-allocate for performance
    line_buf.clear();
    while handle.read_line(&mut line_buf).unwrap() > 0 {
        if line_buf.starts_with('>') {
            break;
        }
        // Append bytes, excluding the newline character at the end
        sequence_bytes.extend_from_slice(&line_buf.as_bytes()[..line_buf.len() - 1]);
        line_buf.clear();
    }

    // Convert the entire sequence to uppercase in parallel
    sequence_bytes.par_iter_mut().for_each(|byte| {
        byte.make_ascii_uppercase();
    });

    sequence_bytes
}

/// Calculates the frequency of all k-mers (substrings of length `k`) in a sequence.
/// This version is parallelized with Rayon for maximum speed.
fn calculate_frequencies<'a>(seq: &'a [u8], k: usize) -> HashMap<&'a [u8], u32> {
    if seq.len() < k {
        return HashMap::new();
    }
    // `par_windows` creates a parallel iterator over all slices of length `k`.
    // The `fold` and `reduce` pattern is a classic map-reduce operation to
    // build the final HashMap in parallel.
    seq.par_windows(k)
        .fold(HashMap::new, |mut counts, kmer| {
            *counts.entry(kmer).or_insert(0) += 1;
            counts
        })
        .reduce(HashMap::new, |mut map1, map2| {
            for (key, count) in map2 {
                *map1.entry(key).or_insert(0) += count;
            }
            map1
        })
}

/// Prints the frequencies of k-mers of a given size, sorted by count.
fn print_sorted_frequencies(seq: &[u8], k: usize) {
    let counts = calculate_frequencies(seq, k);
    let mut sorted_counts: Vec<_> = counts.into_iter().collect();
    // Sort by count (descending), then by k-mer (alphabetically) for a stable sort
    sorted_counts.sort_unstable_by(|a, b| b.1.cmp(&a.1).then(a.0.cmp(b.0)));

    let total = (seq.len() - k + 1) as f64;
    if total > 0.0 {
        for (kmer_bytes, count) in sorted_counts {
            // Safely convert byte slice to string for printing
            let kmer_str = std::str::from_utf8(kmer_bytes).unwrap();
            println!("{} {:.3}", kmer_str, (count as f64) * 100.0 / total);
        }
    }
    println!();
}

/// Counts occurrences of a single, specific DNA code.
fn count_specific_code(seq: &[u8], code: &str) -> (String, usize) {
    let code_bytes = code.as_bytes();
    let count = seq.windows(code_bytes.len()).filter(|&kmer| kmer == code_bytes).count();
    (code.to_string(), count)
}

fn main() {
    let seq = read_sequence();

    // --- Frequency Reports for 1-mers and 2-mers ---
    // These are run sequentially to maintain the correct output order.
    print_sorted_frequencies(&seq, 1);
    print_sorted_frequencies(&seq, 2);

    // --- Specific Code Counts ---
    let codes_to_check = [
        "GGT",
        "GGTA",
        "GGTATT",
        "GGTATTTTAATT",
        "GGTATTTTAATTTATAGT",
    ];

    // Run the counting for all specific codes in parallel.
    let counts: Vec<_> = codes_to_check
        .par_iter()
        .map(|&code| count_specific_code(&seq, code))
        .collect();

    // Print the results in the required order.
    for (code, count) in counts {
        println!("{}\t{}", count, code);
    }
}

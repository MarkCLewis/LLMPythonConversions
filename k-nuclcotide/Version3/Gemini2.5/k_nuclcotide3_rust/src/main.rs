use rayon::prelude::*;
use std::collections::HashMap;
use std::io::{self, Read};

/// Takes a nucleotide sequence string (e.g., "GGTAT") and packs it into a u64.
/// The encoding matches the Python script: G=0, T=1, C=2, A=3.
fn pack_dna(s: &str) -> (usize, u64) {
    let mut bits = 0u64;
    let table = {
        let mut t = [0u8; 256];
        t[b'G' as usize] = 0; t[b'g' as usize] = 0;
        t[b'T' as usize] = 1; t[b't' as usize] = 1;
        t[b'C' as usize] = 2; t[b'c' as usize] = 2;
        t[b'A' as usize] = 3; t[b'a' as usize] = 3;
        t
    };
    for &byte in s.as_bytes() {
        bits = (bits << 2) | (table[byte as usize] as u64);
    }
    (s.len(), bits)
}

/// Uses rayon to count the frequency of all k-mers of a given size `k` in parallel.
/// It iterates over all `k`-sized windows of the DNA sequence and produces a HashMap of counts.
fn count_kmer_freqs(dna: &[u8], k: usize) -> HashMap<u64, usize> {
    if dna.len() < k {
        return HashMap::new();
    }
    dna.par_windows(k)
        .fold(HashMap::new, |mut freqs, window| {
            let mut bits = 0u64;
            for &nucleotide in window {
                bits = (bits << 2) | (nucleotide as u64);
            }
            *freqs.entry(bits).or_insert(0) += 1;
            freqs
        })
        // CORRECTED LINE: .reduce() only takes one argument, the combining function.
        .reduce(|| HashMap::new(), |mut map1, map2| {
            for (key, count) in map2 {
                *map1.entry(key).or_insert(0) += count;
            }
            map1
        })
}

/// Uses rayon to count the occurrences of one specific k-mer in parallel.
fn count_one_kmer(dna: &[u8], packed_kmer: u64, k: usize) -> usize {
    if dna.len() < k {
        return 0;
    }
    dna.par_windows(k)
        .filter(|&window| {
            let mut bits = 0u64;
            for &nucleotide in window {
                bits = (bits << 2) | (nucleotide as u64);
            }
            bits == packed_kmer
        })
        .count()
}

fn main() {
    // --- 1. Read and preprocess sequence from stdin ---
    let mut buffer = Vec::new();
    io::stdin().lock().read_to_end(&mut buffer).unwrap();
    
    let sequence = {
        let header = b">THREE";
        let mut start_pos = 0;
        if let Some(pos) = buffer.windows(header.len()).position(|w| w == header) {
            start_pos = pos;
        }

        while start_pos < buffer.len() && buffer[start_pos] != b'\n' {
            start_pos += 1;
        }
        start_pos += 1;

        let table = {
            let mut t = [0u8; 256];
            t[b'G' as usize] = 0; t[b'g' as usize] = 0;
            t[b'T' as usize] = 1; t[b't' as usize] = 1;
            t[b'C' as usize] = 2; t[b'c' as usize] = 2;
            t[b'A' as usize] = 3; t[b'a' as usize] = 3;
            t
        };
        
        buffer[start_pos..]
            .iter()
            .filter_map(|&b| match b {
                b'G' | b'g' | b'T' | b't' | b'C' | b'c' | b'A' | b'a' => Some(table[b as usize]),
                _ => None,
            })
            .collect::<Vec<u8>>()
    };

    let k_nucleotides = ["GGT", "GGTA", "GGTATT", "GGTATTTTAATT", "GGTATTTTAATTTATAGT"];
    let packed_k_nucleotides: Vec<(usize, u64)> = k_nucleotides.iter().map(|s| pack_dna(s)).collect();

    // --- 2. Perform all counting tasks in parallel ---
    let ((freqs1, freqs2), specific_freqs) = rayon::join(
        || rayon::join(
            || count_kmer_freqs(&sequence, 1),
            || count_kmer_freqs(&sequence, 2)
        ),
        || {
            packed_k_nucleotides
                .par_iter()
                .map(|&(k, packed)| count_one_kmer(&sequence, packed, k))
                .collect::<Vec<_>>()
        },
    );

    // --- 3. Display results in the required format ---
    let total_1mers = (sequence.len() - 1 + 1) as f64;
    let mut freqs1_vec: Vec<(String, f64)> = freqs1
        .into_iter()
        .map(|(bits, count)| {
            let nuc = match bits { 0 => "G", 1 => "T", 2 => "C", 3 => "A", _ => "" };
            (nuc.to_string(), count as f64 * 100.0 / total_1mers)
        })
        .collect();
    freqs1_vec.sort_unstable_by(|a, b| b.1.partial_cmp(&a.1).unwrap().then_with(|| a.0.cmp(&b.0)));
    for (nuc, freq) in freqs1_vec {
        println!("{} {:.3}", nuc, freq);
    }
    println!();

    let total_2mers = (sequence.len() - 2 + 1) as f64;
    let mut freqs2_vec: Vec<(String, f64)> = freqs2
        .into_iter()
        .map(|(bits, count)| {
            let n1 = (bits >> 2) & 0x3;
            let n2 = bits & 0x3;
            let map_fn = |b| match b { 0 => 'G', 1 => 'T', 2 => 'C', 3 => 'A', _ => ' ' };
            let s = format!("{}{}", map_fn(n1), map_fn(n2));
            (s, count as f64 * 100.0 / total_2mers)
        })
        .collect();
    freqs2_vec.sort_unstable_by(|a, b| b.1.partial_cmp(&a.1).unwrap().then_with(|| a.0.cmp(&b.0)));
    for (nuc, freq) in freqs2_vec {
        println!("{} {:.3}", nuc, freq);
    }
    println!();

    for i in 0..k_nucleotides.len() {
        println!("{}\t{}", specific_freqs[i], k_nucleotides[i]);
    }
}

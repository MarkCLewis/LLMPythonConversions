use std::cmp::Ordering;
use std::io::{self, Read};

#[inline]
fn base_to_bits(c: u8) -> Option<u8> {
    // Map A,C,G,T (uppercase) -> 3,2,0,1 respectively
    match c {
        b'G' => Some(0),
        b'T' => Some(1),
        b'C' => Some(2),
        b'A' => Some(3),
        _ => None,
    }
}

fn read_three_sequence() -> Vec<u8> {
    // Read entire stdin
    let mut input = Vec::new();
    io::stdin().read_to_end(&mut input).unwrap();

    // Find header line starting with ">THREE"
    let n = input.len();
    let mut i = 0usize;
    let mut found = false;
    while i < n {
        if input[i] == b'>' {
            let mut j = i + 1;
            while j < n && input[j] != b'\n' && input[j] != b'\r' {
                j += 1;
            }
            if j > i + 1 && j - (i + 1) >= 5 && &input[i + 1..i + 6] == b"THREE" {
                found = true;
                i = j + 1;
                break;
            }
            i = j + 1;
        } else {
            while i < n && input[i] != b'\n' && input[i] != b'\r' {
                i += 1;
            }
            i += 1;
        }
    }
    if !found {
        return Vec::new();
    }

    // Collect sequence lines until next '>' or EOF; uppercase and map to 0..3
    let mut seq = Vec::with_capacity(1 << 20);
    while i < n && input[i] != b'>' {
        let mut c = input[i];
        if (b'a'..=b'z').contains(&c) {
            c = c - 32; // to ASCII uppercase
        }
        if let Some(b) = base_to_bits(c) {
            seq.push(b);
        }
        i += 1;
    }
    seq
}

#[inline]
fn code_of(pat: &str) -> u64 {
    let mut x = 0u64;
    for mut c in pat.bytes() {
        if (b'a'..=b'z').contains(&c) {
            c -= 32;
        }
        let b = base_to_bits(c).unwrap();
        x = (x << 2) | (b as u64);
    }
    x
}

#[inline]
fn count_k_all(seq: &[u8], k: usize) -> Vec<u64> {
    let m = 1usize << (2 * k);
    let mut table = vec![0u64; m];
    if seq.len() < k {
        return table;
    }
    let mask: u64 = if k == 32 { !0 } else { (1u64 << (2 * k)) - 1 };
    let mut code: u64 = 0;
    for i in 0..k - 1 {
        code = (code << 2) | seq[i] as u64;
    }
    for &b in &seq[k - 1..] {
        code = ((code << 2) | b as u64) & mask;
        table[code as usize] += 1;
    }
    table
}

#[inline]
fn count_k_specific(seq: &[u8], k: usize, target_code: u64) -> u64 {
    if seq.len() < k {
        return 0;
    }
    let mask: u64 = if k == 32 { !0 } else { (1u64 << (2 * k)) - 1 };
    let mut code: u64 = 0;
    for i in 0..k - 1 {
        code = (code << 2) | seq[i] as u64;
    }
    let mut cnt = 0u64;
    for &b in &seq[k - 1..] {
        code = ((code << 2) | b as u64) & mask;
        if code == target_code {
            cnt += 1;
        }
    }
    cnt
}

#[derive(Clone)]
struct Item {
    key: &'static str,
    count: u64,
}

fn main() {
    let seq = read_three_sequence();
    let n = seq.len();

    // Keys to report (mirrors the Python)
    // 1-mers and 2-mers are reported as frequency tables;
    // longer patterns are counted exactly.
    let mono_keys: [&str; 4] = ["A", "C", "G", "T"];
    let di_keys: [&str; 16] = [
        "AA", "AC", "AG", "AT", "CA", "CC", "CG", "CT",
        "GA", "GC", "GG", "GT", "TA", "TC", "TG", "TT",
    ];
    let queries: [&str; 5] = [
        "GGT",
        "GGTA",
        "GGTATT",
        "GGTATTTTAATT",
        "GGTATTTTAATTTATAGT",
    ];

    // 1-mers
    let mut c1 = [0u64; 4];
    for &b in &seq {
        c1[b as usize] += 1;
    }
    let mut mono_items: Vec<Item> = mono_keys
        .iter()
        .map(|&k| Item {
            key: k,
            count: c1[code_of(k) as usize],
        })
        .collect();

    // 2-mers
    let c2 = count_k_all(&seq, 2);
    let mut di_items: Vec<Item> = di_keys
        .iter()
        .map(|&k| Item {
            key: k,
            count: c2[code_of(k) as usize],
        })
        .collect();

    let by_freq_then_lex = |a: &Item, b: &Item| match b.count.cmp(&a.count) {
        Ordering::Equal => a.key.cmp(b.key),
        other => other,
    };
    mono_items.sort_by(by_freq_then_lex);
    di_items.sort_by(by_freq_then_lex);

    // Print 1-mer percentages
    let denom1 = if n > 0 { n as f64 } else { 1.0 };
    for it in &mono_items {
        let pct = (it.count as f64) * 100.0 / denom1;
        println!("{} {:.3}", it.key, pct);
    }
    println!();

    // Print 2-mer percentages
    let denom2 = if n >= 2 { (n - 1) as f64 } else { 1.0 };
    for it in &di_items {
        let pct = (it.count as f64) * 100.0 / denom2;
        println!("{} {:.3}", it.key, pct);
    }
    println!();

    // Print counts for specific queries
    for &pat in &queries {
        let k = pat.len();
        let cnt = count_k_specific(&seq, k, code_of(pat));
        println!("{}\t{}", cnt, pat);
    }
}

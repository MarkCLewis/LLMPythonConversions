// The Computer Language Benchmarks Game
// https://salsa.debian.org/benchmarksgame-team/benchmarksgame/
//
// Converted from Python to Rust with optimizations by Claude

use std::env;
use std::io::{self, Write, BufWriter};
use std::sync::{Arc, Mutex};
use std::thread;
use std::collections::VecDeque;
use std::sync::mpsc::{self, Sender, Receiver};

const ALU: &str = concat!(
    "GGCCGGGCGCGGTGGCTCACGCCTGTAATCCCAGCACTTTGG",
    "GAGGCCGAGGCGGGCGGATCACCTGAGGTCAGGAGTTCGAGA",
    "CCAGCCTGGCCAACATGGTGAAACCCCGTCTCTACTAAAAAT",
    "ACAAAAATTAGCCGGGCGTGGTGGCGCGCGCCTGTAATCCCA",
    "GCTACTCGGGAGGCTGAGGCAGGAGAATCGCTTGAACCCGGG",
    "AGGCGGAGGTTGCAGTGAGCCGAGATCGCGCCACTGCACTCC",
    "AGCCTGGGCGACAGAGCGAGACTCCGTCTCAAAAA"
);

const WIDTH: usize = 60;
const IM: f64 = 139968.0;
const BUFFER_SIZE: usize = 64 * 1024; // 64KB buffer

// Frequency tables
const IUB: [(&str, f64); 15] = [
    ("a", 0.27), ("c", 0.12), ("g", 0.12), ("t", 0.27),
    ("B", 0.02), ("D", 0.02), ("H", 0.02), ("K", 0.02),
    ("M", 0.02), ("N", 0.02), ("R", 0.02), ("S", 0.02),
    ("V", 0.02), ("W", 0.02), ("Y", 0.02)
];

const HOMOSAPIENS: [(&str, f64); 4] = [
    ("a", 0.3029549426680),
    ("c", 0.1979883004921),
    ("g", 0.1975473066391),
    ("t", 0.3015094502008)
];

// Message type for the writer thread
enum WriterMsg {
    Data(Vec<u8>),
    Done,
}

// Writer thread that handles all output
fn writer_thread(rx: Receiver<WriterMsg>) -> io::Result<()> {
    let stdout = io::stdout();
    let mut writer = BufWriter::with_capacity(BUFFER_SIZE, stdout.lock());
    
    while let Ok(msg) = rx.recv() {
        match msg {
            WriterMsg::Data(data) => {
                writer.write_all(&data)?;
            },
            WriterMsg::Done => {
                break;
            }
        }
    }
    
    writer.flush()?;
    Ok(())
}

// Convert frequency table to cumulative probabilities
fn make_cumulative(table: &[(&str, f64)]) -> (Vec<f64>, Vec<u8>) {
    let mut probs = Vec::with_capacity(table.len());
    let mut chars = Vec::with_capacity(table.len());
    let mut prob = 0.0;
    
    for &(ch, p) in table {
        prob += p;
        probs.push(prob);
        chars.push(ch.as_bytes()[0]);
    }
    
    (probs, chars)
}

// Binary search to find character based on probability - optimized version
#[inline]
fn bisect(probs: &[f64], r: f64) -> usize {
    let mut low = 0;
    let mut high = probs.len() - 1;
    
    while low < high {
        let mid = (low + high) / 2;
        if r < probs[mid] {
            high = mid;
        } else {
            low = mid + 1;
        }
    }
    
    low
}

// Generate a repeated sequence
fn repeat_fasta(tx: &Sender<WriterMsg>, src: &str, n: usize) -> io::Result<()> {
    let src_bytes = src.as_bytes();
    let src_len = src.len();
    
    // Create an expanded version of the source to reduce modulo operations
    let expanded_src = {
        let mut expanded = Vec::with_capacity(src_len * 2);
        expanded.extend_from_slice(src_bytes);
        expanded.extend_from_slice(src_bytes);
        expanded
    };
    
    let mut pos = 0;
    let mut remaining = n;
    
    // Use a buffer to reduce the number of channel sends
    let mut buffer = Vec::with_capacity(BUFFER_SIZE);
    let mut line = vec![0; WIDTH + 1];
    line[WIDTH] = b'\n';
    
    while remaining > 0 {
        let line_len = remaining.min(WIDTH);
        
        // Copy from the expanded source to avoid modulo operations
        if pos + line_len <= expanded_src.len() {
            line[..line_len].copy_from_slice(&expanded_src[pos..pos + line_len]);
        } else {
            // Fall back to modulo for the rare case where we need to wrap
            for i in 0..line_len {
                line[i] = src_bytes[(pos + i) % src_len];
            }
        }
        
        // Add to buffer
        if line_len == WIDTH {
            buffer.extend_from_slice(&line[..WIDTH + 1]);
        } else {
            line[line_len] = b'\n';
            buffer.extend_from_slice(&line[..line_len + 1]);
        }
        
        // Send buffer when it's full enough
        if buffer.len() >= BUFFER_SIZE - (WIDTH + 1) {
            tx.send(WriterMsg::Data(buffer)).unwrap();
            buffer = Vec::with_capacity(BUFFER_SIZE);
        }
        
        pos = (pos + line_len) % src_len;
        remaining -= line_len;
    }
    
    // Send any remaining data
    if !buffer.is_empty() {
        tx.send(WriterMsg::Data(buffer)).unwrap();
    }
    
    Ok(())
}

// Generate a random sequence
fn random_fasta(
    tx: &Sender<WriterMsg>,
    probs: &[f64],
    chars: &[u8],
    n: usize,
    mut seed: f64
) -> io::Result<f64> {
    let mut line = vec![0; WIDTH + 1];
    line[WIDTH] = b'\n';
    
    let mut buffer = Vec::with_capacity(BUFFER_SIZE);
    let mut remaining = n;
    
    // Pre-generate a table of characters for common probability ranges
    // This is a simple form of "fast path" optimization
    let char_table = if chars.len() <= 4 {  // Only for small tables like homosapiens
        let mut table = vec![0u8; 1024];
        for i in 0..1024 {
            let r = i as f64 / 1024.0;
            let index = bisect(probs, r);
            table[i] = chars[index];
        }
        Some(table)
    } else {
        None
    };
    
    while remaining > 0 {
        let line_len = remaining.min(WIDTH);
        
        for i in 0..line_len {
            // Generate random number
            seed = (seed * 3877.0 + 29573.0) % IM;
            let r = seed / IM;
            
            // Find character based on probability
            line[i] = if let Some(table) = &char_table {
                // Fast path for common cases
                let table_index = (r * 1024.0) as usize;
                if table_index < 1024 {
                    table[table_index]
                } else {
                    chars[bisect(probs, r)]
                }
            } else {
                chars[bisect(probs, r)]
            };
        }
        
        // Add to buffer
        if line_len == WIDTH {
            buffer.extend_from_slice(&line[..WIDTH + 1]);
        } else {
            line[line_len] = b'\n';
            buffer.extend_from_slice(&line[..line_len + 1]);
        }
        
        // Send buffer when it's full enough
        if buffer.len() >= BUFFER_SIZE - (WIDTH + 1) {
            tx.send(WriterMsg::Data(buffer)).unwrap();
            buffer = Vec::with_capacity(BUFFER_SIZE);
        }
        
        remaining -= line_len;
    }
    
    // Send any remaining data
    if !buffer.is_empty() {
        tx.send(WriterMsg::Data(buffer)).unwrap();
    }
    
    Ok(seed)
}

fn main() -> io::Result<()> {
    // Parse command-line argument
    let n = env::args()
        .nth(1)
        .unwrap_or_else(|| "1000".to_string())
        .parse::<usize>()
        .expect("Expected a number");
    
    // Create channel for writer thread
    let (tx, rx) = mpsc::channel();
    
    // Start writer thread
    let writer_handle = thread::spawn(move || {
        writer_thread(rx)
    });
    
    // Create cumulative frequency tables
    let (iub_probs, iub_chars) = make_cumulative(&IUB);
    let (homosapiens_probs, homosapiens_chars) = make_cumulative(&HOMOSAPIENS);
    
    // Generate sequences
    let header1 = b">ONE Homo sapiens alu\n";
    tx.send(WriterMsg::Data(header1.to_vec())).unwrap();
    repeat_fasta(&tx, ALU, n * 2)?;
    
    let header2 = b">TWO IUB ambiguity codes\n";
    tx.send(WriterMsg::Data(header2.to_vec())).unwrap();
    let seed = random_fasta(&tx, &iub_probs, &iub_chars, n * 3, 42.0)?;
    
    let header3 = b">THREE Homo sapiens frequency\n";
    tx.send(WriterMsg::Data(header3.to_vec())).unwrap();
    random_fasta(&tx, &homosapiens_probs, &homosapiens_chars, n * 5, seed)?;
    
    // Signal that we're done
    tx.send(WriterMsg::Done).unwrap();
    
    // Wait for writer thread to finish
    writer_handle.join().unwrap()?;
    
    Ok(())
}
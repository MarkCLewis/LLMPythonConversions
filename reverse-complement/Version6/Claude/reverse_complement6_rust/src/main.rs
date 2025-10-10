// The Computer Language Benchmarks Game
// https://salsa.debian.org/benchmarksgame-team/benchmarksgame/
//
// Converted from Python to Rust by Claude

use std::io::{self, Read, Write, BufReader, BufWriter};
use std::sync::{Arc, Mutex, Condvar};
use std::thread;
use std::sync::mpsc;
use std::cmp::min;

const LINE_LENGTH: usize = 60;

// Structure to hold a sequence with its header
#[derive(Clone)]
struct Sequence {
    header: Vec<u8>,
    data: Vec<u8>,
}

// Create the translation table for DNA complement
fn create_translation_table() -> [u8; 256] {
    let mut table = [0; 256];
    
    // Initialize with identity mapping
    for i in 0..256 {
        table[i] = i as u8;
    }
    
    // Define the complement pairs
    let from = b"ABCDGHKMNRSTUVWYabcdghkmnrstuvwy";
    let to =   b"TVGHCDMKNYSAABWRTVGHCDMKNYSAABWR";
    
    for (i, c) in from.iter().enumerate() {
        table[*c as usize] = to[i];
    }
    
    table
}

// Process a sequence: reverse complement and format
fn reverse_complement(sequence: &Sequence, translation_table: &[u8; 256]) -> Vec<u8> {
    // Translate and filter out newlines and spaces
    let mut translated = Vec::with_capacity(sequence.data.len());
    
    for &byte in &sequence.data {
        match byte {
            b'\n' | b'\r' | b' ' => continue,
            _ => translated.push(translation_table[byte as usize]),
        }
    }
    
    // Create the result with line breaks
    let mut result = Vec::with_capacity(translated.len() + translated.len() / LINE_LENGTH + 2);
    
    // Add initial newline
    result.push(b'\n');
    
    // Add trailing part (if any)
    let trailing_length = translated.len() % LINE_LENGTH;
    if trailing_length > 0 {
        let start = translated.len() - trailing_length;
        result.extend_from_slice(&translated[start..translated.len()]);
        result.push(b'\n');
    }
    
    // Add full lines in reverse order
    let mut pos = translated.len() - trailing_length;
    while pos >= LINE_LENGTH {
        pos -= LINE_LENGTH;
        result.extend_from_slice(&translated[pos..pos + LINE_LENGTH]);
        result.push(b'\n');
    }
    
    result
}

// Read sequences from input stream
fn read_sequences(input: impl Read) -> Vec<Sequence> {
    let mut sequences = Vec::new();
    let mut reader = BufReader::new(input);
    let mut buffer = Vec::new();
    
    // Read entire input
    reader.read_to_end(&mut buffer).expect("Failed to read input");
    
    if buffer.is_empty() {
        return sequences;
    }
    
    let mut start_pos = 0;
    let mut i = 0;
    
    while i < buffer.len() {
        // Find the next header
        if buffer[i] == b'>' {
            if i > start_pos {
                // We found a sequence, add it
                let header_end = buffer[start_pos..i]
                    .iter()
                    .position(|&b| b == b'\n')
                    .map(|pos| start_pos + pos + 1)
                    .unwrap_or(i);
                
                sequences.push(Sequence {
                    header: buffer[start_pos..header_end].to_vec(),
                    data: buffer[header_end..i].to_vec(),
                });
            }
            
            // Start a new sequence
            start_pos = i;
        }
        
        i += 1;
    }
    
    // Add the last sequence
    if start_pos < buffer.len() {
        let header_end = buffer[start_pos..]
            .iter()
            .position(|&b| b == b'\n')
            .map(|pos| start_pos + pos + 1)
            .unwrap_or(buffer.len());
        
        sequences.push(Sequence {
            header: buffer[start_pos..header_end].to_vec(),
            data: buffer[header_end..].to_vec(),
        });
    }
    
    sequences
}

// Worker thread function
fn worker_thread(
    rx: mpsc::Receiver<usize>,
    sequences: Arc<Vec<Sequence>>,
    translation_table: Arc<[u8; 256]>,
    next_index: Arc<Mutex<usize>>,
    cond: Arc<Condvar>,
    stdout: Arc<Mutex<BufWriter<io::Stdout>>>,
) {
    while let Ok(idx) = rx.recv() {
        // Process the sequence
        let sequence = &sequences[idx];
        let result = reverse_complement(sequence, &translation_table);
        
        // Wait for our turn to write
        let mut next = next_index.lock().unwrap();
        while *next != idx {
            next = cond.wait(next).unwrap();
        }
        
        // Write the result
        let mut writer = stdout.lock().unwrap();
        writer.write_all(&sequence.header).unwrap();
        writer.write_all(&result).unwrap();
        writer.flush().unwrap();
        
        // Signal the next thread
        *next += 1;
        cond.notify_all();
    }
}

fn main() {
    // Create translation table
    let translation_table = Arc::new(create_translation_table());
    
    // Read all sequences
    let sequences = Arc::new(read_sequences(io::stdin()));
    
    // Determine whether to use parallel processing
    let num_cores = num_cpus::get();
    let use_parallel = num_cores > 1 && 
                        !sequences.is_empty() && 
                        sequences[0].data.len() >= 1_000_000;
    
    // Create buffered stdout
    let stdout = Arc::new(Mutex::new(BufWriter::new(io::stdout())));
    
    if use_parallel {
        // Set up thread synchronization
        let next_index = Arc::new(Mutex::new(0));
        let cond = Arc::new(Condvar::new());
        
        // Create channel for work distribution
        let (tx, rx) = mpsc::channel();
        
        // Create worker threads
        let num_threads = min(sequences.len(), num_cores);
        let mut handles = Vec::with_capacity(num_threads);
        
        for _ in 0..num_threads {
            let thread_rx = rx.clone();
            let thread_sequences = Arc::clone(&sequences);
            let thread_translation_table = Arc::clone(&translation_table);
            let thread_next_index = Arc::clone(&next_index);
            let thread_cond = Arc::clone(&cond);
            let thread_stdout = Arc::clone(&stdout);
            
            handles.push(thread::spawn(move || {
                worker_thread(
                    thread_rx, 
                    thread_sequences, 
                    thread_translation_table,
                    thread_next_index,
                    thread_cond,
                    thread_stdout,
                );
            }));
        }
        
        // Distribute work
        for i in 0..sequences.len() {
            tx.send(i).unwrap();
        }
        
        // Close the channel
        drop(tx);
        
        // Wait for all threads to finish
        for handle in handles {
            handle.join().unwrap();
        }
    } else {
        // Process sequences sequentially
        let mut writer = BufWriter::new(io::stdout());
        
        for sequence in sequences.iter() {
            let result = reverse_complement(sequence, &translation_table);
            writer.write_all(&sequence.header).unwrap();
            writer.write_all(&result).unwrap();
        }
        
        writer.flush().unwrap();
    }
}
// The Computer Language Benchmarks Game
// https://salsa.debian.org/benchmarksgame-team/benchmarksgame/
//
// Converted from Python to Rust by Claude

use std::env;
use std::io::{self, Read, Write};
use std::sync::{Arc, Condvar, Mutex, mpsc};
use std::thread;

const READ_SIZE: usize = 65536;
const LINE_LENGTH: usize = 60;

// Function to create the complement lookup table
fn create_complement_lookup() -> [u8; 256] {
    let mut lookup = [0; 256];
    
    // Initialize with identity (each byte maps to itself)
    for i in 0..256 {
        lookup[i] = i as u8;
    }
    
    // Set up DNA complementary bases
    lookup[b'A' as usize] = b'T';
    lookup[b'a' as usize] = b't';
    lookup[b'C' as usize] = b'G';
    lookup[b'c' as usize] = b'g';
    lookup[b'G' as usize] = b'C';
    lookup[b'g' as usize] = b'c';
    lookup[b'T' as usize] = b'A';
    lookup[b't' as usize] = b'a';
    lookup[b'U' as usize] = b'A';
    lookup[b'u' as usize] = b'a';
    lookup[b'M' as usize] = b'K';
    lookup[b'm' as usize] = b'k';
    lookup[b'R' as usize] = b'Y';
    lookup[b'r' as usize] = b'y';
    lookup[b'W' as usize] = b'W';
    lookup[b'w' as usize] = b'w';
    lookup[b'S' as usize] = b'S';
    lookup[b's' as usize] = b's';
    lookup[b'Y' as usize] = b'R';
    lookup[b'y' as usize] = b'r';
    lookup[b'K' as usize] = b'M';
    lookup[b'k' as usize] = b'm';
    lookup[b'V' as usize] = b'B';
    lookup[b'v' as usize] = b'b';
    lookup[b'H' as usize] = b'D';
    lookup[b'h' as usize] = b'd';
    lookup[b'D' as usize] = b'H';
    lookup[b'd' as usize] = b'h';
    lookup[b'B' as usize] = b'V';
    lookup[b'b' as usize] = b'v';
    lookup[b'N' as usize] = b'N';
    lookup[b'n' as usize] = b'n';
    
    // Ensure newline remains unchanged
    lookup[b'\n' as usize] = b'\n';
    
    lookup
}

// Structure to represent a DNA sequence with its header and data
struct Sequence {
    data: Vec<u8>,
    sequence_number: usize,
}

// Process a single sequence: reverse complement and output
fn process_sequence(
    sequence: Sequence,
    complement_lookup: &[u8; 256],
    next_sequence_number: &Mutex<usize>,
    sequence_written_cond: &Condvar,
    stdout_lock: &Mutex<io::StdoutLock>,
) -> io::Result<()> {
    let data = &sequence.data;
    let sequence_number = sequence.sequence_number;
    
    // Skip sequence 0 (content before first header)
    if sequence_number == 0 {
        return Ok(());
    }
    
    // Find the header and sequence data
    if let Some(header_end_pos) = data.iter().position(|&b| b == b'\n') {
        let header = &data[0..=header_end_pos];
        let seq_data = &data[header_end_pos + 1..];
        
        // Check if all lines have optimal length
        let mut optimal_length = true;
        let mut i = 0;
        while i < seq_data.len() {
            if let Some(pos) = seq_data[i..].iter().position(|&b| b == b'\n') {
                if pos != LINE_LENGTH && i + pos < seq_data.len() - 1 {
                    optimal_length = false;
                    break;
                }
                i += pos + 1;
            } else {
                optimal_length = false;
                break;
            }
        }
        
        // Complement and reverse the sequence
        let mut result = Vec::with_capacity(seq_data.len());
        
        if optimal_length {
            // Process the data with optimal line lengths
            let mut reversed = Vec::with_capacity(seq_data.len());
            
            // First pass: complement and reverse, removing newlines
            for &byte in seq_data.iter().rev() {
                if byte != b'\n' {
                    reversed.push(complement_lookup[byte as usize]);
                }
            }
            
            // Second pass: add newlines every LINE_LENGTH characters
            for (i, &byte) in reversed.iter().enumerate() {
                result.push(byte);
                if (i + 1) % LINE_LENGTH == 0 || i == reversed.len() - 1 {
                    result.push(b'\n');
                }
            }
        } else {
            // Process the data with non-optimal line lengths
            let mut stripped = Vec::with_capacity(seq_data.len());
            
            // First pass: remove newlines and complement
            for &byte in seq_data.iter() {
                if byte != b'\n' {
                    stripped.push(complement_lookup[byte as usize]);
                }
            }
            
            // Second pass: reverse
            stripped.reverse();
            
            // Third pass: add newlines every LINE_LENGTH characters
            for (i, &byte) in stripped.iter().enumerate() {
                result.push(byte);
                if (i + 1) % LINE_LENGTH == 0 || i == stripped.len() - 1 {
                    result.push(b'\n');
                }
            }
        }
        
        // Wait for our turn to output
        let mut next_sequence = next_sequence_number.lock().unwrap();
        while *next_sequence < sequence_number {
            next_sequence = sequence_written_cond.wait(next_sequence).unwrap();
        }
        
        // Output the sequence
        let mut stdout = stdout_lock.lock().unwrap();
        stdout.write_all(header)?;
        stdout.write_all(&result)?;
        
        // Update next sequence number and notify waiting threads
        *next_sequence += 1;
        sequence_written_cond.notify_all();
    }
    
    Ok(())
}

// Thread worker function
fn worker_thread(
    receiver: mpsc::Receiver<Sequence>,
    complement_lookup: Arc<[u8; 256]>,
    next_sequence_number: Arc<Mutex<usize>>,
    sequence_written_cond: Arc<Condvar>,
    stdout_lock: Arc<Mutex<io::StdoutLock>>,
) -> io::Result<()> {
    while let Ok(sequence) = receiver.recv() {
        process_sequence(
            sequence,
            &complement_lookup,
            &next_sequence_number,
            &sequence_written_cond,
            &stdout_lock,
        )?;
    }
    Ok(())
}

fn main() -> io::Result<()> {
    // Create complement lookup table
    let complement_lookup = Arc::new(create_complement_lookup());
    
    // Determine number of worker threads (number of CPU cores - 1)
    let num_threads = num_cpus::get().max(1) - 1;
    
    // Set up synchronization primitives
    let next_sequence_number = Arc::new(Mutex::new(1));
    let sequence_written_cond = Arc::new(Condvar::new());
    let stdout_lock = Arc::new(Mutex::new(io::stdout().lock()));
    
    // Create channel for passing sequences to worker threads
    let (sender, receiver) = mpsc::channel();
    
    // Spawn worker threads
    let mut handles = Vec::with_capacity(num_threads);
    for _ in 0..num_threads {
        let thread_receiver = receiver.clone();
        let thread_complement_lookup = complement_lookup.clone();
        let thread_next_sequence_number = next_sequence_number.clone();
        let thread_sequence_written_cond = sequence_written_cond.clone();
        let thread_stdout_lock = stdout_lock.clone();
        
        let handle = thread::spawn(move || {
            worker_thread(
                thread_receiver,
                thread_complement_lookup,
                thread_next_sequence_number,
                thread_sequence_written_cond,
                thread_stdout_lock,
            )
        });
        handles.push(handle);
    }
    
    // Read and parse sequences from stdin
    let mut stdin = io::stdin();
    let mut buffer = [0; READ_SIZE];
    let mut current_sequence = Vec::new();
    let mut sequence_number = 0;
    
    loop {
        let bytes_read = stdin.read(&mut buffer)?;
        if bytes_read == 0 {
            break;
        }
        
        let mut i = 0;
        while i < bytes_read {
            // Check for sequence header
            if buffer[i] == b'>' {
                // If we have a current sequence, send it to a worker
                if !current_sequence.is_empty() {
                    sender.send(Sequence {
                        data: current_sequence,
                        sequence_number,
                    }).unwrap();
                    
                    current_sequence = Vec::new();
                    sequence_number += 1;
                }
            }
            
            // Add current byte to the sequence
            current_sequence.push(buffer[i]);
            i += 1;
        }
    }
    
    // Send the final sequence if there is one
    if !current_sequence.is_empty() {
        sender.send(Sequence {
            data: current_sequence,
            sequence_number,
        }).unwrap();
    }
    
    // Close the sender to signal no more sequences
    drop(sender);
    
    // Wait for all worker threads to finish
    for handle in handles {
        if let Err(e) = handle.join() {
            eprintln!("Error joining thread: {:?}", e);
        }
    }
    
    Ok(())
}

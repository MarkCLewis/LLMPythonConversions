use crossbeam_channel::{bounded, Receiver, Sender};
use memchr::memchr;
use std::collections::BTreeMap;
use std::io::{self, BufReader, BufWriter, Read, Write};
use std::thread;

// Defines the size of the buffer for reading from stdin.
const READ_SIZE: usize = 65536;
// Defines the number of characters per line in the output.
const LINE_LENGTH: usize = 60;

// The lookup table for complementing DNA bases.
// This is now built with a `const fn` using `while` loops to be compatible
// with stable Rust's compile-time execution capabilities.
const COMPLEMENT_TABLE: &[u8; 256] = &build_complement_table();

const fn build_complement_table() -> [u8; 256] {
    let mut table = [0; 256];
    let pairs = [
        (b'A', b'T'), (b'C', b'G'), (b'G', b'C'), (b'T', b'A'),
        (b'U', b'A'), (b'M', b'K'), (b'R', b'Y'), (b'W', b'W'),
        (b'S', b'S'), (b'Y', b'R'), (b'K', b'M'), (b'V', b'B'),
        (b'H', b'D'), (b'D', b'H'), (b'B', b'V'), (b'N', b'N'),
        (b'a', b't'), (b'c', b'g'), (b'g', b'c'), (b't', b'a'),
        (b'u', b'a'), (b'm', b'k'), (b'r', b'y'), (b'w', b'w'),
        (b's', b's'), (b'y', b'r'), (b'k', b'm'), (b'v', b'b'),
        (b'h', b'd'), (b'd', b'h'), (b'b', b'v'), (b'n', b'n'),
    ];

    // Initialize all values to themselves
    let mut i = 0;
    while i < 256 {
        table[i] = i as u8;
        i += 1;
    }

    // Overwrite the complement pairs
    let mut i = 0;
    while i < pairs.len() {
        let (key, val) = pairs[i];
        table[key as usize] = val;
        i += 1;
    }
    table
}


// Represents a chunk of a FASTA sequence to be processed.
struct WorkItem {
    id: usize,
    data: Vec<u8>,
}

// Represents a processed sequence ready for output.
struct ResultItem {
    id: usize,
    header: Vec<u8>,
    body: Vec<u8>,
}

fn main() {
    let num_threads = num_cpus::get();
    
    // Create channels for sending work to workers and results to the writer.
    let (work_sender, work_receiver) = bounded::<WorkItem>(num_threads * 2);
    let (result_sender, result_receiver) = bounded::<ResultItem>(num_threads * 2);

    // --- Spawn Worker Threads ---
    let mut worker_handles = Vec::with_capacity(num_threads);
    for _ in 0..num_threads {
        let receiver = work_receiver.clone();
        let sender = result_sender.clone();
        worker_handles.push(thread::spawn(move || worker_thread(receiver, sender)));
    }
    // Drop the main thread's sender clone to ensure the channel closes later.
    drop(result_sender);


    // --- Spawn Writer Thread ---
    // This thread receives processed results and prints them in order.
    let writer_handle = thread::spawn(move || writer_thread(result_receiver));


    // --- Producer (Main Thread) ---
    // Reads from stdin, finds sequences, and sends them to workers.
    let stdin = io::stdin();
    let mut reader = BufReader::new(stdin.lock());
    let mut buffer = vec![0; READ_SIZE];
    let mut sequence_id = 0;
    let mut leftover = Vec::new();

    loop {
        let bytes_read = reader.read(&mut buffer).expect("Failed to read from stdin");
        if bytes_read == 0 {
            if !leftover.is_empty() {
                // Process the final sequence fragment.
                sequence_id += 1;
                work_sender.send(WorkItem { id: sequence_id, data: leftover }).unwrap();
            }
            break;
        }

        let mut current_chunk = &buffer[..bytes_read];
        
        loop {
            // Find the start of the next sequence ('>').
            if let Some(pos) = memchr(b'>', current_chunk) {
                let (sequence_data, next_chunk) = current_chunk.split_at(pos);
                
                if sequence_id > 0 {
                    // We found a complete sequence.
                    let mut final_data = leftover;
                    final_data.extend_from_slice(sequence_data);
                    work_sender.send(WorkItem { id: sequence_id, data: final_data }).unwrap();
                }

                sequence_id += 1;
                leftover = Vec::new(); // Reset leftover for the new sequence.
                current_chunk = &next_chunk[1..]; // Move past the '>'.
            } else {
                // No '>' in this chunk, so append it to leftover and read more.
                leftover.extend_from_slice(current_chunk);
                break;
            }
        }
    }

    // Signal to workers that no more work is coming.
    drop(work_sender);

    // Wait for all threads to complete.
    for handle in worker_handles {
        handle.join().unwrap();
    }
    writer_handle.join().unwrap();
}

/// Worker thread function that receives, processes, and sends sequences.
fn worker_thread(receiver: Receiver<WorkItem>, sender: Sender<ResultItem>) {
    while let Ok(work) = receiver.recv() {
        let (header, sequence) = parse_sequence(work.data);
        
        // --- Reverse Complement Logic ---
        let complement_lookup = |base: u8| COMPLEMENT_TABLE[base as usize];
        
        // Remove newlines and complement the sequence data in one pass, reversing order.
        let mut clean_complemented = Vec::with_capacity(sequence.len());
        for &base in sequence.iter().rev() {
            if base != b'\n' && base != b'\r' {
                clean_complemented.push(complement_lookup(base));
            }
        }
        
        // Re-insert newlines at the correct positions.
        let mut body = Vec::with_capacity(clean_complemented.len() * (LINE_LENGTH + 1) / LINE_LENGTH);
        for chunk in clean_complemented.chunks(LINE_LENGTH) {
            body.extend_from_slice(chunk);
            body.push(b'\n');
        }

        sender.send(ResultItem { id: work.id, header, body }).unwrap();
    }
}

/// Writer thread function that collects results and writes them to stdout in order.
fn writer_thread(receiver: Receiver<ResultItem>) {
    let stdout = io::stdout();
    let mut writer = BufWriter::new(stdout.lock());
    let mut results_buffer: BTreeMap<usize, ResultItem> = BTreeMap::new();
    let mut next_id_to_print = 1;

    while let Ok(result) = receiver.recv() {
        if result.id == next_id_to_print {
            // This is the next sequence we were waiting for.
            writer.write_all(&result.header).unwrap();
            writer.write_all(&result.body).unwrap();
            next_id_to_print += 1;

            // Check if subsequent sequences are already in the buffer.
            while let Some(next_result) = results_buffer.remove(&next_id_to_print) {
                writer.write_all(&next_result.header).unwrap();
                writer.write_all(&next_result.body).unwrap();
                next_id_to_print += 1;
            }
        } else {
            // Result arrived out of order, store it.
            results_buffer.insert(result.id, result);
        }
    }
    writer.flush().unwrap();
}

/// Parses the raw sequence data into a header and the main body.
fn parse_sequence(data: Vec<u8>) -> (Vec<u8>, Vec<u8>) {
    if let Some(pos) = memchr(b'\n', &data) {
        let (header_line, body) = data.split_at(pos);
        // Include the '>' and the newline in the header.
        (format!(">{}\n", std::str::from_utf8(header_line).unwrap()).into_bytes(), body[1..].to_vec())
    } else {
        // Handle sequences with no newline after the header.
        (format!(">{}\n", std::str::from_utf8(&data).unwrap()).into_bytes(), Vec::new())
    }
}

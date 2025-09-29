// The Computer Language Benchmarks Game
// https://salsa.debian.org/benchmarksgame-team/benchmarksgame/
//
// Converted from Python to Rust by Claude
// Based on the Python version submitted by Joerg Baumann

use std::env;
use std::io::{self, BufWriter, Write};
use std::sync::{Arc, Barrier, Mutex};
use std::thread;

const LINE_LENGTH: usize = 60;
const IM: f32 = 139968.0;
const IA: f32 = 3877.0;
const IC: f32 = 29573.0;

struct Nucleotide {
    letter: u8,
    prob: f32,
}

struct Alphabet {
    cumulative_probs: Vec<f32>,
    letters: Vec<u8>,
}

impl Alphabet {
    fn new(nucleotides: &[Nucleotide]) -> Self {
        let mut cumulative_probs = Vec::with_capacity(nucleotides.len());
        let mut letters = Vec::with_capacity(nucleotides.len());
        let mut acc = 0.0;
        
        for nuc in nucleotides {
            letters.push(nuc.letter);
            acc += nuc.prob * IM;
            cumulative_probs.push(acc);
        }
        
        Alphabet { cumulative_probs, letters }
    }
    
    fn select_letter(&self, random_value: f32) -> u8 {
        // Binary search to find the appropriate letter based on random value
        let idx = self.cumulative_probs.binary_search_by(|&p| {
            if p < random_value { std::cmp::Ordering::Less } else { std::cmp::Ordering::Greater }
        }).unwrap_or_else(|i| i);
        
        self.letters[idx]
    }
}

struct RandomGenerator {
    seed: f32,
}

impl RandomGenerator {
    fn new(seed: f32) -> Self {
        RandomGenerator { seed }
    }
    
    fn next_random(&mut self) -> f32 {
        self.seed = (self.seed * IA + IC) % IM;
        self.seed
    }
}

// Write sequence lines with fixed width
fn write_lines<W: Write>(writer: &mut W, sequence: &[u8], width: usize) -> io::Result<()> {
    let mut start = 0;
    let n = sequence.len();
    
    while start + width < n {
        writer.write_all(&sequence[start..start + width])?;
        writer.write_all(b"\n")?;
        start += width;
    }
    
    // Write remaining characters
    if start < n {
        writer.write_all(&sequence[start..n])?;
        writer.write_all(b"\n")?;
    }
    
    writer.flush()?;
    Ok(())
}

// Repeat sequence pattern to fill buffer
fn repeat_sequence(pattern: &[u8], n: usize) -> Vec<u8> {
    let mut sequence = Vec::with_capacity(n);
    let pattern_len = pattern.len();
    
    while sequence.len() < n {
        let remaining = n - sequence.len();
        let to_copy = std::cmp::min(remaining, pattern_len);
        sequence.extend_from_slice(&pattern[..to_copy]);
    }
    
    sequence
}

// Generate random sequence based on alphabet probabilities
fn generate_random_sequence(alphabet: &Alphabet, n: usize, seed: &mut f32) -> Vec<u8> {
    let mut rng = RandomGenerator::new(*seed);
    let mut sequence = Vec::with_capacity(n);
    
    for _ in 0..n {
        let r = rng.next_random();
        sequence.push(alphabet.select_letter(r));
    }
    
    *seed = rng.seed;
    sequence
}

fn main() -> io::Result<()> {
    // Parse command line argument
    let n = env::args().nth(1)
        .and_then(|s| s.parse::<usize>().ok())
        .unwrap_or(1000);
    
    // Standard output with buffer
    let stdout = io::stdout();
    let stdout_handle = stdout.lock();
    let mut writer = BufWriter::with_capacity(65536, stdout_handle);
    
    // Define sequences
    let alu = b"GGCCGGGCGCGGTGGCTCACGCCTGTAATCCCAGCACTTTGGGAGGCCGAGGCGGGCGGA\
        TCACCTGAGGTCAGGAGTTCGAGACCAGCCTGGCCAACATGGTGAAACCCCGTCTCTACT\
        AAAAATACAAAAATTAGCCGGGCGTGGTGGCGCGCGCCTGTAATCCCAGCTACTCGGGAG\
        GCTGAGGCAGGAGAATCGCTTGAACCCGGGAGGCGGAGGTTGCAGTGAGCCGAGATCGCG\
        CCACTGCACTCCAGCCTGGGCGACAGAGCGAGACTCCGTCTCAAAAA";
    
    let iub = vec![
        Nucleotide { letter: b'a', prob: 0.27 },
        Nucleotide { letter: b'c', prob: 0.12 },
        Nucleotide { letter: b'g', prob: 0.12 },
        Nucleotide { letter: b't', prob: 0.27 },
        Nucleotide { letter: b'B', prob: 0.02 },
        Nucleotide { letter: b'D', prob: 0.02 },
        Nucleotide { letter: b'H', prob: 0.02 },
        Nucleotide { letter: b'K', prob: 0.02 },
        Nucleotide { letter: b'M', prob: 0.02 },
        Nucleotide { letter: b'N', prob: 0.02 },
        Nucleotide { letter: b'R', prob: 0.02 },
        Nucleotide { letter: b'S', prob: 0.02 },
        Nucleotide { letter: b'V', prob: 0.02 },
        Nucleotide { letter: b'W', prob: 0.02 },
        Nucleotide { letter: b'Y', prob: 0.02 },
    ];
    
    let homosapiens = vec![
        Nucleotide { letter: b'a', prob: 0.3029549426680 },
        Nucleotide { letter: b'c', prob: 0.1979883004921 },
        Nucleotide { letter: b'g', prob: 0.1975473066391 },
        Nucleotide { letter: b't', prob: 0.3015094502008 },
    ];
    
    // Create alphabets with cumulative probabilities
    let iub_alphabet = Alphabet::new(&iub);
    let homosapiens_alphabet = Alphabet::new(&homosapiens);
    
    // Determine if we should use multi-threading
    let num_cpus = num_cpus::get();
    
    if num_cpus < 2 {
        // Single-threaded execution
        // Task 1: Repeat ALU sequence
        writer.write_all(b">ONE Homo sapiens alu\n")?;
        let alu_sequence = repeat_sequence(alu, n * 2);
        write_lines(&mut writer, &alu_sequence, LINE_LENGTH)?;
        
        // Task 2: Generate IUB sequence
        let mut seed = 42.0;
        writer.write_all(b">TWO IUB ambiguity codes\n")?;
        let iub_sequence = generate_random_sequence(&iub_alphabet, n * 3, &mut seed);
        write_lines(&mut writer, &iub_sequence, LINE_LENGTH)?;
        
        // Task 3: Generate Homo sapiens sequence
        writer.write_all(b">THREE Homo sapiens frequency\n")?;
        let homo_sequence = generate_random_sequence(&homosapiens_alphabet, n * 5, &mut seed);
        write_lines(&mut writer, &homo_sequence, LINE_LENGTH)?;
    } else {
        // Multi-threaded execution with synchronization
        let seed = Arc::new(Mutex::new(42.0f32));
        let writer = Arc::new(Mutex::new(writer));
        
        // Use barriers to coordinate thread execution
        let after_alu = Arc::new(Barrier::new(2));  // Wait after ALU
        let after_iub = Arc::new(Barrier::new(2));  // Wait after IUB
        
        // Thread handles
        let mut handles = vec![];
        
        // Thread 1: ALU sequence
        {
            let writer_clone = Arc::clone(&writer);
            let after_alu_clone = Arc::clone(&after_alu);
            
            let handle = thread::spawn(move || {
                let alu_sequence = repeat_sequence(alu, n * 2);
                let mut w = writer_clone.lock().unwrap();
                w.write_all(b">ONE Homo sapiens alu\n").unwrap();
                write_lines(&mut *w, &alu_sequence, LINE_LENGTH).unwrap();
                drop(w);  // Release the lock
                
                after_alu_clone.wait();  // Signal completion
            });
            
            handles.push(handle);
        }
        
        // Thread 2: IUB sequence
        {
            let writer_clone = Arc::clone(&writer);
            let seed_clone = Arc::clone(&seed);
            let after_alu_clone = Arc::clone(&after_alu);
            let after_iub_clone = Arc::clone(&after_iub);
            let iub_alphabet = iub_alphabet;
            
            let handle = thread::spawn(move || {
                after_alu_clone.wait();  // Wait for ALU to complete
                
                let mut local_seed;
                {
                    let mut s = seed_clone.lock().unwrap();
                    local_seed = *s;
                }
                
                let iub_sequence = generate_random_sequence(&iub_alphabet, n * 3, &mut local_seed);
                
                {
                    let mut s = seed_clone.lock().unwrap();
                    *s = local_seed;
                }
                
                let mut w = writer_clone.lock().unwrap();
                w.write_all(b">TWO IUB ambiguity codes\n").unwrap();
                write_lines(&mut *w, &iub_sequence, LINE_LENGTH).unwrap();
                drop(w);  // Release the lock
                
                after_iub_clone.wait();  // Signal completion
            });
            
            handles.push(handle);
        }
        
        // Thread 3: Homo sapiens sequence
        {
            let writer_clone = Arc::clone(&writer);
            let seed_clone = Arc::clone(&seed);
            let after_iub_clone = Arc::clone(&after_iub);
            let homosapiens_alphabet = homosapiens_alphabet;
            
            let handle = thread::spawn(move || {
                after_iub_clone.wait();  // Wait for IUB to complete
                
                let mut local_seed;
                {
                    let mut s = seed_clone.lock().unwrap();
                    local_seed = *s;
                }
                
                let homo_sequence = generate_random_sequence(&homosapiens_alphabet, n * 5, &mut local_seed);
                
                let mut w = writer_clone.lock().unwrap();
                w.write_all(b">THREE Homo sapiens frequency\n").unwrap();
                write_lines(&mut *w, &homo_sequence, LINE_LENGTH).unwrap();
            });
            
            handles.push(handle);
        }
        
        // Wait for all threads to complete
        for handle in handles {
            handle.join().unwrap();
        }
    }
    
    Ok(())
}
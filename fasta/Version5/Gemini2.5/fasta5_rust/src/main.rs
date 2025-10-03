use std::io::{self, Write, BufWriter};
use std::sync::{Arc, Mutex, mpsc::channel}; // Import the 'channel' type
use std::thread;

const LINE_WIDTH: usize = 60;
const IM: f64 = 139968.0;
const IA: f64 = 3877.0;
const IC: f64 = 29573.0;
const SEED: f64 = 42.0;

/// A linear congruential generator for producing pseudo-random numbers.
struct Lcg {
    seed: f64,
}

impl Lcg {
    fn new(seed: f64) -> Self {
        Lcg { seed }
    }
}

impl Iterator for Lcg {
    type Item = f64;
    fn next(&mut self) -> Option<f64> {
        self.seed = (self.seed * IA + IC) % IM;
        Some(self.seed)
    }
}

/// Finds the index for a random value in a cumulative probability distribution.
/// This is equivalent to Python's `bisect`.
fn find_index(probabilities: &[f64], value: f64) -> u8 {
    probabilities.partition_point(|&p| p < value) as u8
}

/// Repeats a sequence to a specified length `n` and writes it to stdout.
fn make_repeat_fasta(header: &[u8], sequence: &str, n: usize) {
    let mut seq_bytes = Vec::with_capacity(n);
    let pattern = sequence.as_bytes();
    seq_bytes.extend(pattern.iter().cycle().take(n));

    let stdout = io::stdout();
    let mut handle = BufWriter::new(stdout.lock());
    handle.write_all(header).unwrap();
    for chunk in seq_bytes.chunks(LINE_WIDTH) {
        handle.write_all(chunk).unwrap();
        handle.write_all(b"\n").unwrap();
    }
}

/// Generates a random FASTA sequence based on a probability distribution.
fn make_random_fasta(header: &[u8], alphabet: &[(u8, f64)], n: usize, rng_stream: &mut dyn Iterator<Item = f64>) {
    let mut probabilities = Vec::with_capacity(alphabet.len());
    let mut acc = 0.0;
    for &(_, p) in alphabet {
        acc += p * IM;
        probabilities.push(acc);
    }

    let mut output_bytes = Vec::with_capacity(n);
    for rand_val in rng_stream.take(n) {
        let index = find_index(&probabilities, rand_val);
        output_bytes.push(alphabet[index as usize].0);
    }

    let stdout = io::stdout();
    let mut handle = BufWriter::new(stdout.lock());
    handle.write_all(header).unwrap();
    for chunk in output_bytes.chunks(LINE_WIDTH) {
        handle.write_all(chunk).unwrap();
        handle.write_all(b"\n").unwrap();
    }
}

fn fasta(n: usize) {
    let alu = "GGCCGGGCGCGGTGGCTCACGCCTGTAATCCCAGCACTTTGGGAGGCCGAGGCGGGCGGATCACCTGAGGTCAGGAGTTCGAGACCAGCCTGGCCAACATGGTGAAACCCCGTCTCTACTAAAAATACAAAAATTAGCCGGGCGTGGTGGCGCGCGCCTGTAATCCCAGCTACTCGGGAGGCTGAGGCAGGAGAATCGCTTGAACCCGGGAGGCGGAGGTTGCAGTGAGCCGAGATCGCGCCACTGCACTCCAGCCTGGGCGACAGAGCGAGACTCCGTCTCAAAAA";

    let iub: Vec<(u8, f64)> = vec![
        (b'a', 0.27), (b'c', 0.12), (b'g', 0.12), (b't', 0.27),
        (b'B', 0.02), (b'D', 0.02), (b'H', 0.02), (b'K', 0.02),
        (b'M', 0.02), (b'N', 0.02), (b'R', 0.02), (b'S', 0.02),
        (b'V', 0.02), (b'W', 0.02), (b'Y', 0.02),
    ];

    let homosapiens: Vec<(u8, f64)> = vec![
        (b'a', 0.3029549426680),
        (b'c', 0.1979883004921),
        (b'g', 0.1975473066391),
        (b't', 0.3015094502008),
    ];

    if num_cpus::get() < 2 {
        // Sequential execution for single-core systems
        make_repeat_fasta(b">ONE Homo sapiens alu\n", alu, n * 2);
        let mut lcg = Lcg::new(SEED);
        make_random_fasta(b">TWO IUB ambiguity codes\n", &iub, n * 3, &mut lcg);
        make_random_fasta(b">THREE Homo sapiens frequency\n", &homosapiens, n * 5, &mut lcg);
    } else {
        // Parallel execution using threads and channels to control execution order
        let seed = Arc::new(Mutex::new(Lcg::new(SEED)));

        // Create channels for signaling between threads
        let (tx_written_1, rx_written_1) = channel();
        let (tx_seeded_2, rx_seeded_2) = channel();
        let (tx_written_2, rx_written_2) = channel();
        
        let mut handles = Vec::new();

        // Task 1: ALU sequence
        handles.push(thread::spawn(move || {
            make_repeat_fasta(b">ONE Homo sapiens alu\n", alu, n * 2);
            // Send signal that writing is done. Ignore error if receiver has disconnected.
            let _ = tx_written_1.send(());
        }));
        
        // Task 2: IUB random sequence
        let seed_clone = Arc::clone(&seed);
        handles.push(thread::spawn(move || {
            let rng_values: Vec<f64> = seed_clone.lock().unwrap().by_ref().take(n * 3).collect();
            // Send signal that seeding is done.
            let _ = tx_seeded_2.send(());

            // Wait for signal from task 1 that its writing is complete.
            rx_written_1.recv().unwrap();
            make_random_fasta(b">TWO IUB ambiguity codes\n", &iub, n * 3, &mut rng_values.into_iter());
            
            // Send signal that this task's writing is done.
            let _ = tx_written_2.send(());
        }));

        // Task 3: Homo sapiens random sequence
        let seed_clone2 = Arc::clone(&seed);
        handles.push(thread::spawn(move || {
            // Wait for signal from task 2 that it has finished using the shared RNG.
            rx_seeded_2.recv().unwrap();
            let rng_values: Vec<f64> = seed_clone2.lock().unwrap().by_ref().take(n * 5).collect();
            
            // Wait for signal from task 2 that its writing is complete.
            rx_written_2.recv().unwrap();
            make_random_fasta(b">THREE Homo sapiens frequency\n", &homosapiens, n * 5, &mut rng_values.into_iter());
        }));

        for handle in handles {
            handle.join().unwrap();
        }
    }
}

fn main() {
    let n = std::env::args_os()
        .nth(1)
        .and_then(|s| s.into_string().ok())
        .and_then(|s| s.parse().ok())
        .unwrap_or(1000);
    fasta(n);
}

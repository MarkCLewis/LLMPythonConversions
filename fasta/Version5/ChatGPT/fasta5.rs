// fasta.rs
use rayon::prelude::*;
use std::io::{BufWriter, Write};
use std::sync::Mutex;

const IM: f64 = 139968.0;
const IA: f64 = 3877.0;
const IC: f64 = 29573.0;

const ALU: &str = concat!(
    "GGCCGGGCGCGGTGGCTCACGCCTGTAATCCCAGCACTTTGGGAGGCCGAGGCGGGCGGA",
    "TCACCTGAGGTCAGGAGTTCGAGACCAGCCTGGCCAACATGGTGAAACCCCGTCTCTACT",
    "AAAAATACAAAAATTAGCCGGGCGTGGTGGCGCGCGCCTGTAATCCCAGCTACTCGGGAG",
    "GCTGAGGCAGGAGAATCGCTTGAACCCGGGAGGCGGAGGTTGCAGTGAGCCGAGATCGCG",
    "CCACTGCACTCCAGCCTGGGCGACAGAGCGAGACTCCGTCTCAAAAA"
);

#[derive(Clone)]
struct Nucleotide {
    c: u8,
    p: f64,
}

fn cumulative_probs(seq: &[Nucleotide]) -> Vec<Nucleotide> {
    let mut acc = 0.0;
    seq.iter()
        .map(|n| {
            acc += n.p;
            Nucleotide { c: n.c, p: acc }
        })
        .collect()
}

fn select_random(probs: &[Nucleotide], seed: &mut f64) -> u8 {
    *seed = (*seed * IA + IC) % IM;
    let r = *seed / IM;
    for nuc in probs {
        if r < nuc.p {
            return nuc.c;
        }
    }
    probs.last().unwrap().c
}

fn repeat_fasta(header: &str, sequence: &str, n: usize, writer: &Mutex<BufWriter<Box<dyn Write + Send>>>) {
    let seq_bytes = sequence.as_bytes();
    let len = seq_bytes.len();
    let mut out = vec![0u8; 61]; // 60 + newline
    writeln!(writer.lock().unwrap(), "{}", header).unwrap();
    for i in 0..n {
        out[i % 60] = seq_bytes[i % len];
        if (i + 1) % 60 == 0 || i + 1 == n {
            let line_len = if i + 1 == n && n % 60 != 0 { n % 60 } else { 60 };
            writeln!(writer.lock().unwrap(), "{}", std::str::from_utf8(&out[..line_len]).unwrap()).unwrap();
        }
    }
}

fn random_fasta(
    header: &str,
    seq: &[Nucleotide],
    n: usize,
    seed_start: f64,
    writer: &Mutex<BufWriter<Box<dyn Write + Send>>>,
) {
    let probs = cumulative_probs(seq);
    writeln!(writer.lock().unwrap(), "{}", header).unwrap();

    let chunk_size = 60;
    let num_chunks = (n + chunk_size - 1) / chunk_size;

    let lines: Vec<String> = (0..num_chunks)
        .into_par_iter()
        .map(|i| {
            let mut seed = seed_start + i as f64 * 17.0; // different seed per thread
            let mut line = vec![0u8; chunk_size];
            for j in 0..chunk_size {
                let index = i * chunk_size + j;
                if index >= n {
                    break;
                }
                line[j] = select_random(&probs, &mut seed);
            }
            let line_len = if (i + 1) * chunk_size > n {
                n % chunk_size
            } else {
                chunk_size
            };
            format!("{}\n", std::str::from_utf8(&line[..line_len]).unwrap())
        })
        .collect();

    let mut locked = writer.lock().unwrap();
    for line in lines {
        locked.write_all(line.as_bytes()).unwrap();
    }
}

fn main() {
    let args: Vec<String> = std::env::args().collect();
    let n: usize = args.get(1).and_then(|s| s.parse().ok()).unwrap_or(1000);

    let writer: Mutex<BufWriter<Box<dyn Write + Send>>> =
        Mutex::new(BufWriter::new(Box::new(std::io::stdout())));

    let iub = vec![
        ('a', 0.27), ('c', 0.12), ('g', 0.12), ('t', 0.27),
        ('B', 0.02), ('D', 0.02), ('H', 0.02), ('K', 0.02),
        ('M', 0.02), ('N', 0.02), ('R', 0.02), ('S', 0.02),
        ('V', 0.02), ('W', 0.02), ('Y', 0.02),
    ].into_iter().map(|(c, p)| Nucleotide { c: c as u8, p }).collect::<Vec<_>>();

    let homosapiens = vec![
        ('a', 0.3029549426680),
        ('c', 0.1979883004921),
        ('g', 0.1975473066391),
        ('t', 0.3015094502008),
    ].into_iter().map(|(c, p)| Nucleotide { c: c as u8, p }).collect::<Vec<_>>();

    repeat_fasta(">ONE Homo sapiens alu", ALU, n * 2, &writer);
    random_fasta(">TWO IUB ambiguity codes", &iub, n * 3, 42.0, &writer);
    random_fasta(">THREE Homo sapiens frequency", &homosapiens, n * 5, 42.0, &writer);
}

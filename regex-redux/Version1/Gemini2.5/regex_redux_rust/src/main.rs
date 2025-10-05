use rayon::prelude::*;
use regex::Regex;
use std::io::{self, Read};

fn main() {
    // Read the entire standard input into a string.
    let mut seq = String::new();
    io::stdin().read_to_string(&mut seq).expect("Failed to read from stdin");
    let ilen = seq.len();

    // Remove FASTA sequence descriptions and newlines.
    let seq = Regex::new(">.*\n|\n").unwrap().replace_all(&seq, "").to_string();
    let clen = seq.len();

    // A list of regex patterns to count in parallel.
    let variants = &[
        "agggtaaa|tttaccct",
        "[cgt]gggtaaa|tttaccc[acg]",
        "a[act]ggtaaa|tttacc[agt]t",
        "ag[act]gtaaa|tttac[agt]ct",
        "agg[act]taaa|ttta[agt]cct",
        "aggg[acg]aaa|ttt[cgt]ccct",
        "agggt[cgt]aa|tt[acg]accct",
        "agggta[cgt]a|t[acg]taccct",
        "agggtaa[cgt]|[acg]ttaccct",
    ];

    // Use Rayon's `par_iter` to perform the regex searches on multiple cores.
    // The `map` operation runs in parallel for each variant.
    // `collect` gathers the results, preserving the original order.
    let counts: Vec<_> = variants
        .par_iter()
        .map(|&v| {
            let re = Regex::new(v).unwrap();
            let count = re.find_iter(&seq).count();
            (v, count)
        })
        .collect();

    // Print the count for each variant.
    for (variant, count) in counts {
        println!("{} {}", variant, count);
    }

    // A list of regex patterns and their replacements to be applied sequentially.
    let substs = &[
        (r"tHa[Nt]", "<4>"),
        (r"aND|caN|Ha[DS]|WaS", "<3>"),
        (r"a[NSt]|BY", "<2>"),
        (r"<[^>]*>", "|"),
        (r"\|[^|][^|]*\|", "-"),
    ];

    // Apply each substitution sequentially. The result of one `replace_all`
    // becomes the input for the next.
    let mut final_seq = seq;
    for (pattern, replacement) in substs {
        let re = Regex::new(pattern).unwrap();
        final_seq = re.replace_all(&final_seq, *replacement).to_string();
    }

    // Print the final length statistics.
    println!();
    println!("{}", ilen);
    println!("{}", clen);
    println!("{}", final_seq.len());
}

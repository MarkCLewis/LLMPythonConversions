use rayon::prelude::*;
use regex::Regex;
use std::env;
use std::io::{self, Read};

fn main() {
    // Read from stdin
    let mut input = String::new();
    io::stdin().read_to_string(&mut input).unwrap();

    let ilen = input.len();

    // Strip FASTA headers and newlines
    let re_cleanup = Regex::new(r">.*\n|\n").unwrap();
    let cleaned = re_cleanup.replace_all(&input, "");
    let clen = cleaned.len();
    let sequence = cleaned.to_string();

    // Regex variant patterns to count
    let variants = [
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

    // Parallel counting using rayon
    variants.par_iter().for_each(|&pat| {
        let re = Regex::new(pat).unwrap();
        let count = re.find_iter(&sequence).count();
        println!("{} {}", pat, count);
    });

    // Sequential substitutions
    let substitutions = [
        (r"tHa[Nt]", "<4>"),
        (r"aND|caN|Ha[DS]|WaS", "<3>"),
        (r"a[NSt]|BY", "<2>"),
        (r"<[^>]*>", "|"),
        (r"\|[^|][^|]*\|", "-"),
    ];

    let mut modified = sequence;
    for (pat, replacement) in substitutions.iter() {
        let re = Regex::new(pat).unwrap();
        modified = re.replace_all(&modified, *replacement).to_string();
    }

    println!();
    println!("{}", ilen);
    println!("{}", clen);
    println!("{}", modified.len());
}

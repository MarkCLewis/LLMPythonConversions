use pcre2::bytes::Regex;
use rayon::prelude::*;
use std::env;
use std::fs;
use std::io::{self, Read};

const VARIANTS: [&str; 9] = [
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

const SUBSTITUTIONS: [(&str, &str); 5] = [
    (r"tHa[Nt]", "<4>"),
    (r"aND|caN|Ha[DS]|WaS", "<3>"),
    (r"a[NSt]|BY", "<2>"),
    (r"<[^>]*>", "|"),
    (r"\|[^|][^|]*\|", "-"),
];

fn main() {
    // Read input from stdin
    let mut input = Vec::new();
    io::stdin().read_to_end(&mut input).unwrap();
    let ilen = input.len();

    // Remove headers and newlines
    let re_header = Regex::new(r">.*\n|\n").unwrap();
    let cleaned = re_header
        .replace_all(&input, &b""[..])
        .into_owned();
    let clen = cleaned.len();

    // Count matches in parallel
    let counts: Vec<(String, usize)> = VARIANTS
        .par_iter()
        .map(|&pattern| {
            let re = Regex::new(pattern).unwrap();
            let count = re.find_iter(&cleaned).count();
            (pattern.to_string(), count)
        })
        .collect();

    for (pattern, count) in &counts {
        println!("{} {}", pattern, count);
    }

    // Perform substitutions
    let mut result = String::from_utf8(cleaned).unwrap();
    for (pattern, replacement) in SUBSTITUTIONS.iter() {
        let re = regex::Regex::new(pattern).unwrap();
        result = re.replace_all(&result, *replacement).to_string();
    }

    println!();
    println!("{}", ilen);
    println!("{}", clen);
    println!("{}", result.len());
}

use pcre2::bytes::RegexBuilder;
use rayon::prelude::*;
use std::io::{self, Read};

fn main() {
    let mut input = Vec::new();
    io::stdin().lock().read_to_end(&mut input).unwrap();
    let initial_len = input.len();

    let cleaner_re = RegexBuilder::new().jit(true).build(">.*\\n|\\n").unwrap();
    
    // FIX: The arguments were swapped. The correct order is `replace_all(text, replacement)`.
    let sequences = cleaner_re.replace_all(&input, b"").into_owned();
    let cleaned_len = sequences.len();

    let final_len = {
        let replacements: &[(&[u8], &[u8])] = &[
            (b"tHa[Nt]", b"<4>"),
            (b"aND|caN|Ha[DS]|WaS", b"<3>"),
            (b"a[NSt]|BY", b"<2>"),
            (b"<[^>]*>", b"|"),
            (b"\\|[^|][^|]*\\|", b"-"),
        ];

        let mut buffer1 = sequences.clone();
        let mut buffer2 = Vec::with_capacity(buffer1.len());

        for (i, (pattern, replacement)) in replacements.iter().enumerate() {
            let re = RegexBuilder::new()
                .jit(true)
                .build(std::str::from_utf8(pattern).unwrap())
                .unwrap();

            if i % 2 == 0 {
                buffer2.clear();
                // FIX: The correct order is `replace_all_to(source, destination, replacement)`.
                re.replace_all_to(&buffer1, &mut buffer2, replacement);
            } else {
                buffer1.clear();
                // FIX: The correct order is `replace_all_to(source, destination, replacement)`.
                re.replace_all_to(&buffer2, &mut buffer1, replacement);
            }
        }

        if replacements.len() % 2 == 1 {
            buffer2.len()
        } else {
            buffer1.len()
        }
    };

    let count_patterns: &[&[u8]] = &[
        b"agggtaaa|tttaccct",
        b"[cgt]gggtaaa|tttaccc[acg]",
        b"a[act]ggtaaa|tttacc[agt]t",
        b"ag[act]gtaaa|tttac[agt]ct",
        b"agg[act]taaa|ttta[agt]cct",
        b"aggg[acg]aaa|ttt[cgt]ccct",
        b"agggt[cgt]aa|tt[acg]accct",
        b"agggta[cgt]a|t[acg]taccct",
        b"agggtaa[cgt]|[acg]ttaccct",
    ];

    let counts: Vec<usize> = count_patterns
        .par_iter()
        .map(|pattern| {
            let re = RegexBuilder::new()
                .jit(true)
                .build(std::str::from_utf8(pattern).unwrap())
                .unwrap();
            re.find_iter(&sequences).count()
        })
        .collect();

    for (pattern, count) in count_patterns.iter().zip(counts) {
        println!("{} {}", String::from_utf8_lossy(pattern), count);
    }
    println!();
    println!("{}", initial_len);
    println!("{}", cleaned_len);
    println!("{}", final_len);
}

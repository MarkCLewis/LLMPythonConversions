use std::env;
use std::io::{self, Write};

fn extract_digit(nth: u128, acc: u128, den: u128, num: u128) -> u128 {
    let tmp1 = num * nth;
    let tmp2 = tmp1 + acc;
    tmp2 / den
}

fn eliminate_digit(d: u128, acc: &mut u128, den: u128, num: &mut u128) {
    *acc = (*acc - den * d) * 10;
    *num *= 10;
}

fn next_term(k: u128, acc: &mut u128, den: &mut u128, num: &mut u128) {
    let k2 = k * 2 + 1;
    *acc = (*acc + *num * 2) * k2;
    *den *= k2;
    *num *= k;
}

fn main() {
    let args: Vec<String> = env::args().collect();
    let n: u128 = args.get(1).and_then(|s| s.parse().ok()).unwrap_or(1000);

    let (mut acc, mut den, mut num) = (0_u128, 1_u128, 1_u128);
    let mut k = 0;
    let mut i = 0;

    let stdout = io::stdout();
    let mut out = stdout.lock();

    while i < n {
        k += 1;
        next_term(k, &mut acc, &mut den, &mut num);

        if num > acc {
            continue;
        }

        let d = extract_digit(3, acc, den, num);
        if d != extract_digit(4, acc, den, num) {
            continue;
        }

        write!(out, "{}", d).unwrap();
        i += 1;
        if i % 10 == 0 {
            writeln!(out, "\t:{}", i).unwrap();
        }

        eliminate_digit(d, &mut acc, den, &mut num);
    }
}

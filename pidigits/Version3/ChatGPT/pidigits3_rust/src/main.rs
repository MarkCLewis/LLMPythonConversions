use rug::{Assign, Integer};
use std::env;

fn extract_digit(n: u32, acc: &Integer, den: &Integer, num: &Integer) -> u32 {
    let mut tmp1 = Integer::new();
    let mut tmp2 = Integer::new();
    tmp1.assign(num * n);
    tmp2.assign(&tmp1 + acc);
    let digit = &tmp2 / den;
    digit.to_u32_wrapping()
}

fn eliminate_digit(d: u32, acc: &mut Integer, den: &Integer, num: &mut Integer) {
    acc.sub_assign(den * d);
    acc.mul_assign(10);
    num.mul_assign(10);
}

fn next_term(k: u32, acc: &mut Integer, den: &mut Integer, num: &mut Integer) {
    let k2 = 2 * k + 1;
    acc.add_assign(num * 2);
    acc.mul_assign(k2);
    den.mul_assign(k2);
    num.mul_assign(k);
}

fn main() {
    let args: Vec<String> = env::args().collect();
    let n: u32 = args.get(1).and_then(|s| s.parse().ok()).unwrap_or(1000);

    let mut acc = Integer::from(0);
    let mut den = Integer::from(1);
    let mut num = Integer::from(1);

    let mut i = 0;
    let mut k = 0;

    while i < n {
        k += 1;
        next_term(k, &mut acc, &mut den, &mut num);

        if num > acc {
            continue;
        }

        let d = extract_digit(3, &acc, &den, &num);
        if d != extract_digit(4, &acc, &den, &num) {
            continue;
        }

        print!("{}", d);
        i += 1;

        if i % 10 == 0 {
            println!("\t:{}", i);
        }

        eliminate_digit(d, &mut acc, &den, &mut num);
    }
}

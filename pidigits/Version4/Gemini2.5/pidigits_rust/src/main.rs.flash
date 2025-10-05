use num_bigint::BigInt;
use num_traits::{One, Zero, ToPrimitive};
use std::env;

/// This struct holds the state of the Pi digit generation stream,
/// replacing the global variables used in the original Python script.
struct PiDigitStream {
    acc: BigInt,
    den: BigInt,
    num: BigInt,
}

impl PiDigitStream {
    /// Creates a new stream with the initial state values.
    fn new() -> Self {
        PiDigitStream {
            acc: BigInt::zero(), // Corresponds to acc = 0
            den: BigInt::one(),  // Corresponds to den = 1
            num: BigInt::one(),  // Corresponds to num = 1
        }
    }

    /// Calculates the next term in the series, updating the state.
    /// Corresponds to the `next_Term` function.
    fn next_term(&mut self, k: u32) {
        let k2 = (k as u64 * 2) + 1;
        
        self.acc += &self.num * 2u32;
        self.acc *= k2;
        self.den *= k2;
        self.num *= k;
    }
    
    /// Extracts a potential digit from the current state.
    /// Corresponds to the `extract_Digit` function.
    fn extract_digit(&self, nth: u32) -> BigInt {
        // The operations are combined for efficiency.
        let tmp = &self.num * nth + &self.acc;
        tmp / &self.den
    }

    /// Consumes a confirmed digit, adjusting the state for the next iteration.
    /// Corresponds to the `eliminate_Digit` function.
    fn eliminate_digit(&mut self, d: &BigInt) {
        self.acc -= &self.den * d;
        self.acc *= 10u32;
        self.num *= 10u32;
    }
}

fn main() {
    // 1. Get the number of digits `n` from the command-line arguments.
    //    Defaults to 27 if no argument is provided.
    let n = env::args_os()
        .nth(1)
        .and_then(|s| s.into_string().ok())
        .and_then(|s| s.parse().ok())
        .unwrap_or(27);

    // 2. Initialize the state and counters.
    let mut pi_stream = PiDigitStream::new();
    let mut i = 0;
    let mut k = 0u32;

    // 3. Main loop to generate and print digits.
    while i < n {
        k += 1;
        pi_stream.next_term(k);

        // A key step in the spigot algorithm: continue consuming terms until
        // the state is ready to produce a stable digit.
        if pi_stream.num > pi_stream.acc {
            continue;
        }

        // Check if the 3rd and 4th extracted digits are the same.
        // If they are, we can be confident the digit is correct.
        let d = pi_stream.extract_digit(3);
        if d != pi_stream.extract_digit(4) {
            continue;
        }

        // We have a confirmed digit, so print it.
        // `d` is a BigInt (e.g., 3), so we convert it to a u8, add the ASCII
        // offset for '0', and cast it to a character for printing.
        print!("{}", (d.to_u8().unwrap() + b'0') as char);
        i += 1;

        // Format the output with a tab and count every 10 digits.
        if i % 10 == 0 {
            println!("\t:{}", i);
        }

        // Consume the digit from the stream to prepare for the next one.
        pi_stream.eliminate_digit(&d);
    }
    
    // If n is not a multiple of 10, pad the final line with spaces
    // for clean alignment and print the final count.
    let remainder = n % 10;
    if remainder != 0 {
        for _ in 0..(10 - remainder) {
            print!(" ");
        }
        println!("\t:{}", n);
    }
}

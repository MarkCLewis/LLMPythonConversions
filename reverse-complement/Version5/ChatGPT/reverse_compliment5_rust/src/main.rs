use std::io::{self, Read, Write};

#[inline]
fn comp_byte(mut c: u8) -> u8 {
    // Uppercase ASCII
    if (b'a'..=b'z').contains(&c) { c -= 32; }
    match c {
        b'A' => b'T', b'C' => b'G', b'G' => b'C', b'T' => b'A',
        b'M' => b'K', b'R' => b'Y', b'Y' => b'R', b'K' => b'M',
        b'V' => b'B', b'B' => b'V', b'H' => b'D', b'D' => b'H',
        b'W' => b'W', b'S' => b'S', b'N' => b'N', b'U' => b'A',
        _ => c, // pass through any other byte unchanged
    }
}

fn main() -> io::Result<()> {
    // Read entire stdin
    let mut buf = Vec::new();
    io::stdin().read_to_end(&mut buf)?;

    let mut out = io::BufWriter::new(io::stdout());
    let n = buf.len();
    let mut i = 0usize;

    // Walk the FASTA stream
    while i < n {
        // Seek next header
        while i < n && buf[i] != b'>' { i += 1; }
        if i >= n { break; }

        // Emit header line (up to '\n')
        let hstart = i;
        while i < n && buf[i] != b'\n' { i += 1; }
        let hend = if i < n { i + 1 } else { i }; // include trailing '\n' if present
        out.write_all(&buf[hstart..hend])?;

        // Collect sequence bytes until next '>' or EOF (skip newlines)
        let mut seq = Vec::with_capacity(1 << 20);
        let mut p = hend;
        while p < n && buf[p] != b'>' {
            let b = buf[p];
            if b != b'\n' && b != b'\r' {
                seq.push(b);
            }
            p += 1;
        }

        // Reverse-complement and emit in 60-char lines
        let mut pos = seq.len();
        const L: usize = 60;
        let mut line = [0u8; L];

        while pos > 0 {
            let take = if pos >= L { L } else { pos };
            // Fill current line
            for k in 0..take {
                let b = seq[pos - 1 - k];
                line[k] = comp_byte(b);
            }
            out.write_all(&line[..take])?;
            out.write_all(b"\n")?;
            pos -= take;
        }

        i = p; // continue at next header (or EOF)
    }

    out.flush()
}

use std::io::{self, Read, Write};

#[inline]
fn build_comp_table() -> [u8; 256] {
    // Start as identity (we'll uppercase when needed)
    let mut comp = [0u8; 256];
    for i in 0..256 {
        comp[i] = i as u8;
    }

    // Same mapping as Python:
    // src = "ABCDGHKMNRSTUVWYabcdghkmnrstuvwy"
    // dst = "TVGHCDMKNYSAABWRTVGHCDMKNYSAABWR"
    let src = b"ABCDGHKMNRSTUVWYabcdghkmnrstuvwy";
    let dst = b"TVGHCDMKNYSAABWRTVGHCDMKNYSAABWR";
    for (&s, &d) in src.iter().zip(dst.iter()) {
        comp[s as usize] = d;
    }

    comp
}

#[inline]
fn comp_byte(c: u8, table: &[u8; 256]) -> u8 {
    let mut x = c;
    // map lowercase to uppercase for lookup if needed
    if (b'a'..=b'z').contains(&x) {
        x = x - 32;
    }
    let m = table[x as usize];
    if m != x {
        m
    } else {
        // Identity for anything unmapped; ensure uppercase for letters
        x
    }
}

fn main() -> io::Result<()> {
    // Read entire stdin
    let mut buf = Vec::new();
    io::stdin().read_to_end(&mut buf)?;

    let comp = build_comp_table();
    let mut out = io::BufWriter::new(io::stdout());

    let n = buf.len();
    let mut i = 0usize;

    while i < n {
        // Seek next header '>'
        while i < n && buf[i] != b'>' { i += 1; }
        if i >= n { break; }

        // Emit header line up to and including '\n' (if present)
        let hs = i;
        while i < n && buf[i] != b'\n' { i += 1; }
        let he = if i < n { i + 1 } else { i };
        out.write_all(&buf[hs..he])?;

        // Collect sequence until next '>' or EOF, ignoring '\n', '\r', ' '
        let mut seq = Vec::with_capacity(1 << 20);
        let mut p = he;
        while p < n && buf[p] != b'>' {
            let b = buf[p];
            if !(b == b'\n' || b == b'\r' || b == b' ') {
                seq.push(b);
            }
            p += 1;
        }

        // Reverse-complement in 60-char lines
        const L: usize = 60;
        let mut pos = seq.len();
        let mut line = [0u8; L];

        while pos > 0 {
            let take = if pos >= L { L } else { pos };
            for k in 0..take {
                let b = seq[pos - 1 - k];
                line[k] = comp_byte(b, &comp);
            }
            out.write_all(&line[..take])?;
            out.write_all(b"\n")?;
            pos -= take;
        }

        i = p;
    }

    out.flush()
}

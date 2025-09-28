// mandelbrot7.c — C port of mandelbrot7.py (Computer Language Benchmarks Game)
// Build (with OpenMP):  gcc -O3 -fopenmp -march=native -o mandelbrot7 mandelbrot7.c
// Run:                  ./mandelbrot7 6000 > out.pbm
// Output format: PBM (P4), 1 bit/pixel (0 = white escaped, 1 = black inside)

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#ifdef _OPENMP
#include <omp.h>
#endif

static inline unsigned char pixel_byte(int y, int xb, int n) {
    // Compute one output byte (8 pixels) at row y, byte-index xb.
    // Complex-plane mapping identical to Python:
    //   c1 = 2.0 / n
    //   c0 = (-1.5) + i*(y*c1 - 1.0)
    //   c  = (x*c1) + c0
    const double c1 = 2.0 / (double)n;
    const double ci = (double)y * c1 - 1.0;     // imag(c0)
    const double c0r = -1.5;
    const int x0 = xb * 8;

    unsigned char byte = 0;
    for (int bit = 0; bit < 8; ++bit) {
        int x = x0 + bit;
        if (x >= n) break; // off-image bits left as 0

        const double cr = c0r + (double)x * c1;
        double zr = cr, zi = ci; // z = c to start (matches Python's z=c)
        int inside = 1;

        // 7 blocks * 7 inner iterations = 49 steps; check escape after each block
        for (int outer = 0; outer < 7; ++outer) {
            for (int inner = 0; inner < 7; ++inner) {
                // z = z*z + c
                double zr2 = zr * zr - zi * zi + cr;
                double zi2 = 2.0 * zr * zi + ci;
                zr = zr2; zi = zi2;
            }
            if ((zr * zr + zi * zi) >= 4.0) { inside = 0; break; }
        }
        if (inside) byte |= (unsigned char)(0x80u >> bit);
    }
    return byte;
}

int main(int argc, char **argv) {
    if (argc < 2) {
        fprintf(stderr, "usage: %s <size>\n", argv[0]);
        return 1;
    }
    const int n = atoi(argv[1]);
    if (n <= 0) {
        fprintf(stderr, "size must be positive\n");
        return 1;
    }

    const int row_bytes = (n + 7) >> 3;

    // Emit PBM header
    printf("P4\n%d %d\n", n, n);

    // Allocate full image; fill rows (parallel), then write once to keep order
    unsigned char *img = (unsigned char*)malloc((size_t)row_bytes * (size_t)n);
    if (!img) { perror("malloc"); return 1; }

    #pragma omp parallel for schedule(static)
    for (int y = 0; y < n; ++y) {
        unsigned char *row = img + (size_t)y * row_bytes;
        for (int xb = 0; xb < row_bytes; ++xb) {
            row[xb] = pixel_byte(y, xb, n);
        }
        // Mask trailing bits in the last byte (same as Python's result[-1] &= ...)
        if ((n & 7) != 0) {
            unsigned char mask = (unsigned char)(0xFFu << (8 - (n & 7)));
            row[row_bytes - 1] &= mask;
        }
    }

    // Write image
    size_t total = (size_t)row_bytes * (size_t)n;
    if (fwrite(img, 1, total, stdout) != total) {
        perror("fwrite");
        free(img);
        return 1;
    }
    free(img);
    return 0;
}

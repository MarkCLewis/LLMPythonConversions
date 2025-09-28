// mandelbrot.c — C port of mandelbrot5.py (Computer Language Benchmarks Game)
// Build (with OpenMP):  gcc -O3 -fopenmp -march=native -o mandelbrot mandelbrot.c
// Run:                  ./mandelbrot 16000 > out.pbm
// Output format: PBM (P4), packed 1 bit per pixel, 0 = white (escaped), 1 = black (inside)

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#ifdef _OPENMP
#include <omp.h>
#endif

static inline int mand8(double ci, int x_start, int size) {
    // Builds one byte (8 pixels) starting at x index x_start (corresponds to Python's x=7,15,...)
    // Mapping identical to the Python:
    //   two_over_size = 2.0/size
    //   real c = -1.5 + two_over_size*(x - offset), offset=7..0
    //   imag c = -1.0 + 2.0*y/size  (ci is already that value)
    const double two_over_size = 2.0 / (double)size;
    int byte_acc = 0;

    for (int offset = 7; offset >= 0; --offset) {
        const double cr = -1.5 + two_over_size * (double)(x_start - offset);
        double zr = 0.0, zi = 0.0;
        int inside = 1;

        // 50 iterations, break on |z| >= 2
        for (int i = 0; i < 50; ++i) {
            // z = z*z + c  (with z = zr + i*zi, c = cr + i*ci)
            const double zr2 = zr*zr - zi*zi + cr;
            const double zi2 = 2.0*zr*zi + ci;
            zr = zr2;
            zi = zi2;
            if (zr*zr + zi*zi >= 4.0) { inside = 0; break; }
        }
        if (inside) byte_acc |= (1 << offset);
    }
    return byte_acc;
}

int main(int argc, char **argv) {
    if (argc < 2) { fprintf(stderr, "usage: %s <size>\n", argv[0]); return 1; }
    const int size = atoi(argv[1]);
    if (size <= 0) { fprintf(stderr, "size must be positive\n"); return 1; }

    // Write PBM header
    printf("P4\n%d %d\n", size, size);

    const double step_im = 2.0 / (double)size; // imag step per row
    // Each row has ceil(size/8.0) bytes
    const int row_bytes = (size + 7) >> 3;

    // Allocate whole image buffer for parallel fill, then write once
    unsigned char *image = (unsigned char*)malloc((size_t)row_bytes * (size_t)size);
    if (!image) { perror("malloc"); return 1; }

    // Parallel over rows
    #pragma omp parallel for schedule(static)
    for (int y = 0; y < size; ++y) {
        unsigned char *row = image + (size_t)y * row_bytes;
        const double ci = -1.0 + step_im * (double)y;   // imag part (fy in Python)
        int xb = 0;                                     // byte index in row

        // Pack pixels in groups of 8. In Python: for x in range(7, size, 8)
        for (int x = 7; x < size; x += 8) {
            const int byte_acc = mand8(ci, x, size);
            row[xb++] = (unsigned char)byte_acc;
        }

        // Python appends last byte again when width not aligned to 8
        // (because 'x != size - 1' after the loop). Reproduce that behavior:
        // After the loop, the last computed byte_acc corresponds to the last group,
        // and Python appends it if width%8 != 0.
        if (((size - 1) - 7) % 8 != 0) {
            // recompute the last byte (same x as previous iteration)
            int last_x = ((size - 1) / 8) * 8 - 1 + 8 - 1; // nearest of form 7,15,... <= size-1
            if (last_x < 7) last_x = 7;
            const int byte_acc = mand8(ci, last_x, size);
            row[xb++] = (unsigned char)byte_acc;
        }
        // If width is multiple of 8, loop filled exactly row_bytes bytes already.
        // Ensure row fully written (defensive): pad zeros if any gap (shouldn't happen).
        while (xb < row_bytes) row[xb++] = 0;
    }

    // Write all rows
    if (fwrite(image, 1, (size_t)row_bytes * (size_t)size, stdout) !=
        (size_t)row_bytes * (size_t)size) {
        perror("fwrite");
        free(image);
        return 1;
    }
    free(image);
    return 0;
}

/*
 * The Computer Language Benchmarks Game
 * https://salsa.debian.org/benchmarksgame-team/benchmarksgame/
 *
 * Contributed by Joerg Baumann (original Python)
 * Converted to C by Gemini
 */

#include <stdio.h>
#include <stdlib.h>
#include <omp.h>

int main(int argc, char **argv) {
    // Set image size from command-line argument, default to 1000
    const int n = (argc > 1) ? atoi(argv[1]) : 1000;
    const int width_bytes = (n + 7) / 8;

    // Print the PBM header
    printf("P4\n%d %d\n", n, n);

    // Allocate memory for the entire bitmap
    unsigned char *bitmap = malloc(n * width_bytes);
    if (!bitmap) {
        fprintf(stderr, "Error: Memory allocation failed.\n");
        return 1;
    }

    // Parallelize the calculation of rows using OpenMP
    // The 'guided' schedule is efficient for workloads where some iterations
    // (rows near the center of the set) take longer than others.
    #pragma omp parallel for schedule(guided)
    for (int y = 0; y < n; y++) {
        double ci = 2.0 * y / n - 1.0;
        unsigned char *row_ptr = bitmap + y * width_bytes;

        // Process one byte (8 pixels) at a time
        for (int i = 0; i < n; i += 8) {
            unsigned char byte_acc = 0;

            // Calculate the 8 pixels that make up the byte
            for (int j = 0; j < 8; j++) {
                int x = i + j;
                // Stop if we're past the right edge of the image
                if (x >= n) {
                    break;
                }

                double cr = 2.0 * x / n - 1.5;
                double zr = cr;
                double zi = ci;
                int k;

                // Mandelbrot escape-time algorithm for max 50 iterations
                for (k = 0; k < 50; k++) {
                    double tr = zr * zr - zi * zi + cr;
                    double ti = 2.0 * zr * zi + ci;
                    zr = tr;
                    zi = ti;
                    if (zr * zr + zi * zi >= 4.0) {
                        break;
                    }
                }

                // If the point did not escape, it's in the set.
                // Set the corresponding bit in the byte accumulator.
                if (k == 50) {
                    byte_acc |= (1 << (7 - j));
                }
            }
            // Store the completed byte in the row's data
            row_ptr[i / 8] = byte_acc;
        }
    }

    // Write the entire bitmap to standard output
    fwrite(bitmap, width_bytes, n, stdout);

    // Free the allocated memory
    free(bitmap);

    return 0;
}

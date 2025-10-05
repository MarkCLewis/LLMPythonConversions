#include <stdio.h>
#include <stdlib.h>
#include <omp.h>

// A simple error handling function
void die(const char *msg) {
    fputs(msg, stderr);
    fputc('\n', stderr);
    exit(1);
}

int main(int argc, char **argv) {
    if (argc < 2) {
        die("Usage: ./mandelbrot <size>");
    }
    const int size = atoi(argv[1]);
    if (size <= 0) {
        die("Size must be a positive integer.");
    }

    // The PBM image format (P4) requires a simple header
    printf("P4\n%d %d\n", size, size);

    // Calculate the number of bytes needed per row, rounding up
    const int row_bytes = (size + 7) / 8;
    
    // Allocate a memory buffer for the entire bitmap image
    unsigned char *bitmap = malloc(size * row_bytes);
    if (!bitmap) {
        die("Memory allocation failed.");
    }

    // Parallelize the main loop over the rows using OpenMP.
    // 'schedule(guided)' is efficient for workloads where some iterations
    // (rows closer to the Mandelbrot set) take longer than others.
    #pragma omp parallel for schedule(guided)
    for (int y = 0; y < size; y++) {
        // Imaginary component of the complex number `c`. Constant for the entire row.
        const double ci = 2.0 * y / size - 1.0;
        
        // Pointer to the start of the current row in the main bitmap buffer
        unsigned char *row_ptr = bitmap + y * row_bytes;

        // Loop through each pixel in the row
        for (int x = 0; x < size; x++) {
            // Real component of the complex number `c`.
            const double cr = 2.0 * x / size - 1.5;

            double zr = 0.0, zi = 0.0;
            int i = 0;
            
            // The core Mandelbrot iteration
            for (; i < 50; i++) {
                // z = z*z + c
                const double temp_zr = zr * zr - zi * zi + cr;
                zi = 2.0 * zr * zi + ci;
                zr = temp_zr;

                // Check the escape condition. Using zr*zr + zi*zi >= 4.0 is faster
                // than using sqrt() to check if abs(z) >= 2.0.
                if (zr * zr + zi * zi >= 4.0) {
                    break;
                }
            }
            
            // If the loop finished without escaping, the point is in the set.
            // We set the corresponding bit in the byte for this row.
            if (i == 50) {
                // The bit position is (7 - (x % 8)) to ensure the first pixel
                // corresponds to the most significant bit (MSB).
                row_ptr[x / 8] |= (1 << (7 - (x % 8)));
            }
        }
    }

    // Write the entire binary bitmap to standard output at once
    fwrite(bitmap, row_bytes, size, stdout);

    // Free the allocated memory
    free(bitmap);

    return 0;
}

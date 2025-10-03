#include <stdio.h>    // For printf
#include <stdlib.h>   // For atoi
#include <stdbool.h>  // For the bool type

/**
 * Calculates the Fannkuch Redux benchmark.
 * This function generates all n! permutations of {0, 1, ..., n-1},
 * and for each one, counts the number of "flips" required to bring
 * the first element to 0. It tracks the maximum flips found and a checksum.
 * @param n The size of the permutation.
 * @return The maximum number of flips found for any permutation.
 */
int fannkuch(int n) {
    int maxFlipsCount = 0;
    bool permSign = true;
    int checksum = 0;
    int nm = n - 1;

    // Allocate arrays on the stack using VLA (C99 feature)
    int perm1[n];
    int count[n];
    int perm[n];

    // Initialize perm1 and count arrays
    for (int i = 0; i < n; ++i) {
        perm1[i] = i;
        count[i] = i;
    }

    // Main loop to generate and process permutations
    while (true) {
        int k = perm1[0];
        if (k != 0) {
            // Make a copy of the current permutation to work with
            for (int i = 0; i < n; ++i) {
                perm[i] = perm1[i];
            }

            int flipsCount = 1;
            while (perm[k] != 0) {
                // Reverse the prefix of the array of length k+1
                for (int i = 0, j = k; i < j; ++i, --j) {
                    int temp = perm[i];
                    perm[i] = perm[j];
                    perm[j] = temp;
                }
                flipsCount++;
                k = perm[k];
            }

            if (maxFlipsCount < flipsCount) {
                maxFlipsCount = flipsCount;
            }
            checksum += permSign ? flipsCount : -flipsCount;
        }

        // --- Generate the next permutation incrementally ---
        if (permSign) {
            // Easy case: swap the first two elements
            int temp = perm1[0];
            perm1[0] = perm1[1];
            perm1[1] = temp;
            permSign = false;
        } else {
            // Harder case: swap and rotate
            int temp = perm1[1];
            perm1[1] = perm1[2];
            perm1[2] = temp;
            permSign = true;

            // This section simulates Python's 'for...else' construct
            // to find the next permutation by rotation.
            bool broke = false;
            int r;
            for (r = 2; r < n - 1; r++) {
                if (count[r] > 0) {
                    broke = true;
                    break;
                }
                count[r] = r;
                // Rotate the prefix of the array
                int perm0 = perm1[0];
                for (int i = 0; i <= r; ++i) {
                    perm1[i] = perm1[i + 1];
                }
                perm1[r + 1] = perm0;
            }

            if (!broke) {
                r = nm; // Corresponds to the 'else' block of the Python loop
                if (count[r] == 0) {
                    printf("%d\n", checksum);
                    return maxFlipsCount; // All permutations done
                }
            }
            count[r]--;
        }
    }
}

int main(int argc, char *argv[]) {
    if (argc < 2) {
        printf("Usage: %s <n>\n", argv[0]);
        return 1;
    }

    int n = atoi(argv[1]);
    if (n <= 0) {
        printf("Error: Please provide a positive integer for n.\n");
        return 1;
    }

    printf("Pfannkuchen(%d) = %d\n", n, fannkuch(n));

    return 0;
}

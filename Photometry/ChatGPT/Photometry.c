#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <fftw3.h>
#include <time.h>

// Occultation Lightcurve Simulation in C
// Compile: gcc quaoar.c -o quaoar -lfftw3 -lm

#define PI 3.141592653589793

// Transmission profile: simple flat example
double transmission(double r) {
    // Example: tau = 0.1 flat → T = exp(-tau)
    return exp(-0.1);
}

// Build aperture with vertical ring segment
void RingSeg_ap(double lam, double D, int npts, double wid, double *ap) {
    double lam_km = lam * 1e-9;  // microns → km
    double gridSz = sqrt(lam_km * D / npts);
    int npts2 = npts / 2;
    double wid2 = 0.5 * wid;

    for (int j = 0; j < npts; j++) {
        double x = (j - npts2) * gridSz; // km
        for (int i = 0; i < npts; i++) {
            double val = 1.0;
            if (fabs(x) <= wid2) {
                val = transmission(x);
            }
            ap[j*npts + i] = val;
        }
    }
}

// Occultation lightcurve calculation
void occ_lc(int npts, double *ap, double *out) {
    int N = npts;
    int N2 = N/2;
    fftw_complex *in, *outfft;
    fftw_plan p;

    in = (fftw_complex*) fftw_malloc(sizeof(fftw_complex) * N * N);
    outfft = (fftw_complex*) fftw_malloc(sizeof(fftw_complex) * N * N);

    // Apply modified aperture function
    for (int j = 0; j < N; j++) {
        for (int i = 0; i < N; i++) {
            double y = j - N2;
            double x = i - N2;
            double phase = PI * (x*x + y*y) / N;
            double val = ap[j*N + i];
            in[j*N + i][0] = val * cos(phase); // real
            in[j*N + i][1] = val * sin(phase); // imag
        }
    }

    // FFT
    p = fftw_plan_dft_2d(N, N, in, outfft, FFTW_FORWARD, FFTW_ESTIMATE);
    fftw_execute(p);

    // Compute intensity
    for (int j = 0; j < N; j++) {
        for (int i = 0; i < N; i++) {
            double re = outfft[j*N + i][0];
            double im = outfft[j*N + i][1];
            out[j*N + i] = re*re + im*im;
        }
    }

    fftw_destroy_plan(p);
    fftw_free(in);
    fftw_free(outfft);
}

int main() {
    int npts = 1024;          // Smaller than Python (4096) for testing
    double lam = 0.5;         // microns
    double D = 43 * 150e6;    // km
    double wid = 46;          // km

    double *ap = malloc(sizeof(double) * npts * npts);
    double *obs = malloc(sizeof(double) * npts * npts);

    clock_t start = clock();

    RingSeg_ap(lam, D, npts, wid, ap);
    occ_lc(npts, ap, obs);

    clock_t end = clock();
    printf("Computation finished in %.2f seconds\n",
           (double)(end-start)/CLOCKS_PER_SEC);

    // Save one slice of the lightcurve (mid-row)
    FILE *f = fopen("lightcurve.csv", "w");
    int mid = npts/2;
    for (int i = 0; i < npts; i++) {
        fprintf(f, "%d, %e\n", i, obs[mid*npts + i]);
    }
    fclose(f);

    free(ap);
    free(obs);
    return 0;
}

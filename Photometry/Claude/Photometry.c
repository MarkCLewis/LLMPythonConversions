#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <complex.h>
#include <string.h>
#include <time.h>
#include <fftw3.h>

// Function to calculate the E-field in the observers plane for an occultation
void occ_lc(fftw_complex *ap, fftw_complex *result, int npts) {
    int i, j;
    int n2 = npts / 2;
    double pi = 3.14159265358979323846;
    
    // Create plan for FFTW
    fftw_complex *M = fftw_malloc(sizeof(fftw_complex) * npts * npts);
    fftw_plan plan = fftw_plan_dft_2d(npts, npts, M, result, FFTW_FORWARD, FFTW_ESTIMATE);
    
    // Calculate modified aperture function: ap * exp((i*pi/npts) * (y^2 + x^2))
    for (i = 0; i < npts; i++) {
        for (j = 0; j < npts; j++) {
            int idx = i * npts + j;
            double y = (double)(i - n2);
            double x = (double)(j - n2);
            double complex eTerm = cexp(((pi * I) / npts) * (y * y + x * x));
            M[idx] = ap[idx] * eTerm;
        }
    }
    
    // Execute FFT
    fftw_execute(plan);
    
    // Calculate intensity: |E|^2
    for (i = 0; i < npts; i++) {
        for (j = 0; j < npts; j++) {
            int idx = i * npts + j;
            int roll_idx = ((i + n2) % npts) * npts + ((j + n2) % npts);
            double complex conj_val = conj(result[idx]);
            result[roll_idx] = creal(result[idx] * conj_val);
        }
    }
    
    fftw_destroy_plan(plan);
    fftw_free(M);
}

// Function to create an aperture with a vertical ring segment running through it
void RingSeg_ap(double lam, double D, int npts, double wid, 
                double *radial_pos, double *trans_values, int num_radial_points,
                fftw_complex *ap, double *FOV, double *gridSz) {
    int i, j;
    
    double lam_km = lam * 1e-9; // Convert microns to km
    *gridSz = sqrt(lam_km * D / npts);
    *FOV = sqrt(lam_km * D * npts);
    double FOV2 = 0.5 * (*FOV);
    int npts2 = npts / 2;
    double wid2 = 0.5 * wid; // Half the ring width
    
    // Initialize aperture with full transmission
    for (i = 0; i < npts; i++) {
        for (j = 0; j < npts; j++) {
            ap[i * npts + j] = 1.0;
        }
    }
    
    // Create grid of x values centered on zero
    double *xv = (double *)malloc(npts * sizeof(double));
    for (i = 0; i < npts; i++) {
        xv[i] = (i - npts2) * (*gridSz);
    }
    
    // Replace in-aperture columns with their transmission functions
    for (i = 0; i < npts; i++) {
        for (j = 0; j < npts; j++) {
            double x_val = xv[j];
            if (fabs(x_val) <= wid2) {
                // Interpolate to find transmission at this radial position
                // Linear interpolation of transmission values
                double trans = 1.0;
                if (x_val <= radial_pos[0]) {
                    trans = trans_values[0];
                } else if (x_val >= radial_pos[num_radial_points-1]) {
                    trans = trans_values[num_radial_points-1];
                } else {
                    for (int k = 0; k < num_radial_points-1; k++) {
                        if (x_val >= radial_pos[k] && x_val <= radial_pos[k+1]) {
                            double t = (x_val - radial_pos[k]) / (radial_pos[k+1] - radial_pos[k]);
                            trans = trans_values[k] * (1-t) + trans_values[k+1] * t;
                            break;
                        }
                    }
                }
                ap[i * npts + j] = trans;
            }
        }
    }
    
    free(xv);
}

// Utility function to create and fill transmission profile arrays
void create_transmission_profile(double wid, int nRad, double *wVal, double *tau, double *trans) {
    double wid2 = 0.5 * wid;
    
    // Fill radial positions array (wVal)
    for (int i = 0; i < nRad; i++) {
        wVal[i] = -wid2 + i * (2 * wid2 / (nRad - 1));
    }
}

// Function to generate flat optical depth profile
void generate_flat_profile(double tau_value, int nRad, double *tau, double *trans) {
    for (int i = 0; i < nRad; i++) {
        tau[i] = tau_value;
        trans[i] = exp(-tau[i]);
    }
}

// Function to generate centrally peaked optical depth profile
void generate_cp_profile(double *wVal, int nRad, double *tau, double *trans) {
    double pbMax = 0, pbMin = 0;
    double targMax = 0.0;
    double targMin = 0.1;
    double m, b;
    
    // Calculate parabola values
    double *pb = (double *)malloc(nRad * sizeof(double));
    for (int i = 0; i < nRad; i++) {
        pb[i] = wVal[i] * wVal[i];
        if (i == 0 || pb[i] > pbMax) pbMax = pb[i];
        if (i == 0 || pb[i] < pbMin) pbMin = pb[i];
    }
    
    // Scale the parabola
    m = (targMax - targMin) / (pbMax - pbMin);
    b = targMax - m * pbMax;
    
    // Calculate tau and transmission
    for (int i = 0; i < nRad; i++) {
        tau[i] = m * pb[i] + b;
        trans[i] = exp(-tau[i]);
    }
    
    free(pb);
}

// Function to generate sharp edge optical depth profile
void generate_se_profile(double *wVal, int nRad, double *tau, double *trans) {
    double pbMax = 0, pbMin = 0;
    double targMax = 0.1;
    double targMin = 0.0;
    double m, b;
    
    // Calculate parabola values
    double *pb = (double *)malloc(nRad * sizeof(double));
    for (int i = 0; i < nRad; i++) {
        pb[i] = wVal[i] * wVal[i];
        if (i == 0 || pb[i] > pbMax) pbMax = pb[i];
        if (i == 0 || pb[i] < pbMin) pbMin = pb[i];
    }
    
    // Scale the parabola
    m = (targMax - targMin) / (pbMax - pbMin);
    b = targMax - m * pbMax;
    
    // Calculate tau and transmission
    for (int i = 0; i < nRad; i++) {
        tau[i] = m * pb[i] + b;
        trans[i] = exp(-tau[i]);
    }
    
    free(pb);
}

void do_run(int npts) {
    clock_t start_time, end_time;
    double cpu_time_used;
    
    // Parameters
    double lam = 0.5;              // wavelength (microns)
    double D = 43 * 150e6;         // Quaoar at 43 AU (km)
    double wid = 46;               // Width of the ring (km)
    
    // For wider ring simulation
    double widW = 100;             // Width of the wider ring (km)
    
    // Create radial profile arrays
    int nRad = 100;
    double *wVal = (double *)malloc(nRad * sizeof(double));
    double *tau = (double *)malloc(nRad * sizeof(double));
    double *trans = (double *)malloc(nRad * sizeof(double));
    
    // Initialize arrays and wVal
    create_transmission_profile(wid, nRad, wVal, tau, trans);
    
    // Allocate memory for aperture and result
    fftw_complex *ap = fftw_malloc(sizeof(fftw_complex) * npts * npts);
    fftw_complex *result = fftw_malloc(sizeof(fftw_complex) * npts * npts);
    double FOV, gridSz;
    
    // Variables for output
    double *r_km = (double *)malloc(npts * sizeof(double));
    double *t_sec = (double *)malloc(npts * sizeof(double));
    double *lightcurve = (double *)malloc(npts * sizeof(double));
    
    // Initialize FFTW
    fftw_init_threads();
    fftw_plan_with_nthreads(4);  // Use 4 threads for parallel execution
    
    printf("Starting simulations...\n");
    
    // Run the first simulation: Flat profile with tau = 0.1
    printf("Running flat profile (tau = 0.1)...\n");
    start_time = clock();
    
    // Generate the profile
    generate_flat_profile(0.1, nRad, tau, trans);
    
    // Create aperture
    RingSeg_ap(lam, D, npts, wid, wVal, trans, nRad, ap, &FOV, &gridSz);
    
    // Calculate lightcurve
    occ_lc(ap, result, npts);
    
    // Normalize to baseline of 1
    double bg = 0;
    for (int i = 0; i < 100; i++) {
        bg += creal(result[2048 * npts + i]);
    }
    bg /= 100.0;
    
    // Create r_km and t_sec arrays
    for (int i = 0; i < npts; i++) {
        r_km[i] = gridSz * (i - npts/2);
        t_sec[i] = r_km[i] / 5.8;  // Event velocity in km/s
        
        // Extract and normalize the lightcurve
        lightcurve[i] = creal(result[2048 * npts + i]) / bg;
    }
    
    end_time = clock();
    cpu_time_used = ((double)(end_time - start_time)) / CLOCKS_PER_SEC;
    printf("Simulation completed in %f seconds\n", cpu_time_used);
    
    // Write results to file
    FILE *fp = fopen("lightcurve_flat_tau01.txt", "w");
    fprintf(fp, "# Time (s), Normalized Flux\n");
    for (int i = 0; i < npts; i++) {
        fprintf(fp, "%f %f\n", t_sec[i], lightcurve[i]);
    }
    fclose(fp);
    
    // Run other simulations (flat tau=1.0, centrally peaked, sharp edges)
    // Similar code structure for each simulation...
    
    // Clean up
    fftw_free(ap);
    fftw_free(result);
    free(wVal);
    free(tau);
    free(trans);
    free(r_km);
    free(t_sec);
    free(lightcurve);
    
    fftw_cleanup_threads();
    fftw_cleanup();
    
    printf("All simulations completed. Results written to files.\n");
}

int main() {
	do_run(4096*1);
	do_run(4096*2);
	do_run(4096*3);
}

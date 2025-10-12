use std::f64::consts::PI;
use std::time::Instant;
use std::fs::File;
use std::io::Write;
use rustfft::{FftPlanner, num_complex::{Complex, Complex64}};
use rayon::prelude::*;

/// Calculates the E-field in the observers plane for an occultation
/// by an object with a complex aperture
fn occ_lc(ap: &[Complex64], npts: usize) -> Vec<f64> {
    let n2 = npts / 2;
    
    // Create modified aperture function
    let mut m = vec![Complex64::new(0.0, 0.0); npts * npts];
    
    for i in 0..npts {
        for j in 0..npts {
            let y = (i as f64) - (n2 as f64);
            let x = (j as f64) - (n2 as f64);
            let e_term = Complex64::new(0.0, (PI / npts as f64) * (y * y + x * x)).exp();
            m[i * npts + j] = ap[i * npts + j] * e_term;
        }
    }
    
    // Perform 2D FFT
    let mut planner = FftPlanner::new();
    let fft = planner.plan_fft_forward(npts);
    
    // Transform each row
    let mut rows: Vec<Vec<Complex64>> = (0..npts)
        .map(|i| m[i*npts..(i+1)*npts].to_vec())
        .collect();
    
    rows.par_iter_mut().for_each(|row| {
        fft.process(row);
    });
    
    // Transpose
    let mut transposed = vec![Complex64::new(0.0, 0.0); npts * npts];
    for i in 0..npts {
        for j in 0..npts {
            transposed[j * npts + i] = rows[i][j];
        }
    }
    
    // Transform each column
    let mut cols: Vec<Vec<Complex64>> = (0..npts)
        .map(|i| transposed[i*npts..(i+1)*npts].to_vec())
        .collect();
    
    cols.par_iter_mut().for_each(|col| {
        fft.process(col);
    });
    
    // Transpose back
    let mut e_obs = vec![Complex64::new(0.0, 0.0); npts * npts];
    for i in 0..npts {
        for j in 0..npts {
            e_obs[i * npts + j] = cols[j][i];
        }
    }
    
    // Calculate intensity: |E|^2 and roll
    let mut p_obs = vec![0.0; npts * npts];
    for i in 0..npts {
        for j in 0..npts {
            let i_roll = (i + n2) % npts;
            let j_roll = (j + n2) % npts;
            let val = e_obs[i * npts + j];
            p_obs[i_roll * npts + j_roll] = (val * val.conj()).re;
        }
    }
    
    p_obs
}

/// Creates apertures with a vertical ring segment running through it
fn ring_seg_ap(
    lam: f64, 
    d: f64, 
    npts: usize, 
    wid: f64, 
    t_r: &dyn Fn(f64) -> f64
) -> (Vec<Complex64>, f64, f64) {
    let lam_km = lam * 1e-9; // convert microns to km
    
    let grid_sz = (lam_km * d / npts as f64).sqrt();
    let fov = (lam_km * d * npts as f64).sqrt();
    let npts2 = npts / 2;
    
    let wid2 = 0.5 * wid; // Half the ring width
    
    // Initialize aperture with full transmission
    let mut ap = vec![Complex64::new(1.0, 0.0); npts * npts];
    
    // For each column
    for j in 0..npts {
        let x = ((j as f64) - (npts2 as f64)) * grid_sz;
        
        // If within ring width
        if x.abs() <= wid2 {
            let transmission = t_r(x);
            
            // Apply transmission to entire column
            for i in 0..npts {
                ap[i * npts + j] = Complex64::new(transmission, 0.0);
            }
        }
    }
    
    (ap, fov, grid_sz)
}

/// Generates transmission profile based on optical depth
fn generate_transmission_profile(
    wid: f64, 
    n_rad: usize
) -> (Vec<f64>, Vec<f64>) {
    let wid2 = 0.5 * wid;
    
    // Generate radial positions
    let mut w_val = vec![0.0; n_rad];
    for i in 0..n_rad {
        w_val[i] = -wid2 + (i as f64) * (2.0 * wid2) / ((n_rad - 1) as f64);
    }
    
    (w_val.clone(), w_val.clone())
}

/// Linear interpolation function
fn interp1d(x_values: &[f64], y_values: &[f64], x: f64) -> f64 {
    // Handle edge cases
    if x <= x_values[0] {
        return y_values[0];
    }
    if x >= x_values[x_values.len() - 1] {
        return y_values[y_values.len() - 1];
    }
    
    // Find surrounding points
    for i in 0..(x_values.len() - 1) {
        if x_values[i] <= x && x <= x_values[i + 1] {
            let t = (x - x_values[i]) / (x_values[i + 1] - x_values[i]);
            return y_values[i] * (1.0 - t) + y_values[i + 1] * t;
        }
    }
    
    // Fallback
    y_values[0]
}

fn main() {
    println!("Quaoar Rings Occultation Simulator (Rust implementation)");
    
    // Parameters
    let lam = 0.5; // wavelength (microns)
    let d = 43.0 * 150e6; // Quaoar at 43 AU (km)
    let npts = 4096;
    let wid = 46.0; // Width of the ring (km)
    let wid2 = 0.5 * wid;
    
    let n_rad = 100; // Number of steps across the ring's radius
    
    // Generate radial profile points
    let (w_val, _) = generate_transmission_profile(wid, n_rad);
    
    // Generate different optical depth profiles
    
    // 1. Flat profile with tau = 0.1
    let mut tau_flat_01 = vec![0.0; n_rad];
    let mut tr_flat_01 = vec![0.0; n_rad];
    for i in 0..n_rad {
        tau_flat_01[i] = 0.1;
        tr_flat_01[i] = ((-tau_flat_01[i]) as f64).exp();
    }
    
    // 2. Flat profile with tau = 1.0
    let mut tau_flat_10 = vec![0.0; n_rad];
    let mut tr_flat_10 = vec![0.0; n_rad];
    for i in 0..n_rad {
        tau_flat_10[i] = 1.0;
        tr_flat_10[i] = ((-tau_flat_10[i]) as f64).exp();
    }
    
    // 3. Centrally peaked profile
    let mut tau_cp = vec![0.0; n_rad];
    let mut tr_cp = vec![0.0; n_rad];
    
    // Make a parabola
    let mut pb = vec![0.0; n_rad];
    for i in 0..n_rad {
        pb[i] = w_val[i] * w_val[i];
    }
    
    let pb_max = pb.iter().fold(f64::NEG_INFINITY, |a, &b| a.max(b));
    let pb_min = pb.iter().fold(f64::INFINITY, |a, &b| a.min(b));
    
    // Scale for centrally peaked profile
    let targ_max = 0.0;
    let targ_min = 0.1;
    let m = (targ_max - targ_min) / (pb_max - pb_min);
    let b = targ_max - m * pb_max;
    
    for i in 0..n_rad {
        tau_cp[i] = m * pb[i] + b;
        tr_cp[i] = (-tau_cp[i]).exp();
    }
    
    // 4. Sharp edge profile
    let mut tau_se = vec![0.0; n_rad];
    let mut tr_se = vec![0.0; n_rad];
    
    // Scale for sharp edge profile
    let targ_max = 0.1;
    let targ_min = 0.0;
    let m = (targ_max - targ_min) / (pb_max - pb_min);
    let b = targ_max - m * pb_max;
    
    for i in 0..n_rad {
        tau_se[i] = m * pb[i] + b;
        tr_se[i] = (-tau_se[i]).exp();
    }
    
    // Create transmission functions using closures
    let t_f1 = |x: f64| -> f64 { interp1d(&w_val, &tr_flat_01, x) };
    let t_f2 = |x: f64| -> f64 { interp1d(&w_val, &tr_flat_10, x) };
    let t_cp = |x: f64| -> f64 { interp1d(&w_val, &tr_cp, x) };
    let t_se = |x: f64| -> f64 { interp1d(&w_val, &tr_se, x) };
    
    println!("Running simulations for four ring profiles...");
    
    // Create apertures
    let start = Instant::now();
    let (ap_f1, fov, grid_sz) = ring_seg_ap(lam, d, npts, wid, &t_f1);
    let (ap_f2, _, _) = ring_seg_ap(lam, d, npts, wid, &t_f2);
    let (ap_f3, _, _) = ring_seg_ap(lam, d, npts, wid, &t_cp);
    let (ap_f4, _, _) = ring_seg_ap(lam, d, npts, wid, &t_se);
    
    // Calculate observer plane intensity fields
    let op_f1 = occ_lc(&ap_f1, npts);
    let op_f2 = occ_lc(&ap_f2, npts);
    let op_f3 = occ_lc(&ap_f3, npts);
    let op_f4 = occ_lc(&ap_f4, npts);
    
    // Normalize to an out-of-event baseline of 1
    let bg1 = op_f1[2048 * npts..(2048 * npts + 100)]
        .iter()
        .sum::<f64>() / 100.0;
    let bg2 = op_f2[2048 * npts..(2048 * npts + 100)]
        .iter()
        .sum::<f64>() / 100.0;
    let bg3 = op_f3[2048 * npts..(2048 * npts + 100)]
        .iter()
        .sum::<f64>() / 100.0;
    let bg4 = op_f4[2048 * npts..(2048 * npts + 100)]
        .iter()
        .sum::<f64>() / 100.0;
    
    // Calculate r_km and t_sec values
    let mut r_km = vec![0.0; npts];
    let mut t_sec = vec![0.0; npts];
    let v_event = 5.8; // Event velocity in km/s
    
    for i in 0..npts {
        r_km[i] = grid_sz * ((i as f64) - (npts as f64) / 2.0);
        t_sec[i] = r_km[i] / v_event;
    }
    
    // Write results to file
    let mut file = File::create("quaoar_lightcurves.csv").expect("Unable to create file");
    
    writeln!(file, "time_sec,flat_tau01,flat_tau10,central_peak,sharp_edge")
        .expect("Unable to write header");
    
    for i in 0..npts {
        writeln!(
            file,
            "{},{},{},{},{}",
            t_sec[i],
            op_f1[2048 * npts + i] / bg1,
            op_f2[2048 * npts + i] / bg2,
            op_f3[2048 * npts + i] / bg3,
            op_f4[2048 * npts + i] / bg4
        ).expect("Unable to write data");
    }
    
    let elapsed = start.elapsed();
    println!("Basic simulation completed in {:.2?}", elapsed);
    
    // Run wider ring simulation
    let wid_w = 100.0; // Width of wider ring (km)
    let n_rad = 200;
    
    let (w_val_w, _) = generate_transmission_profile(wid_w, n_rad);
    
    // Flat profile with tau = 0.1 for wider ring
    let mut tau_flat_01_w = vec![0.0; n_rad];
    let mut tr_flat_01_w = vec![0.0; n_rad];
    
    for i in 0..n_rad {
        tau_flat_01_w[i] = 0.1;
        tr_flat_01_w[i] = ((-tau_flat_01_w[i]) as f64).exp();
    }
    
    let t_f1_w = |x: f64| -> f64 { interp1d(&w_val_w, &tr_flat_01_w, x) };
    
    println!("Running wider ring simulation...");
    let start = Instant::now();
    
    let (ap_f1_w, _, _) = ring_seg_ap(lam, d, npts, wid_w, &t_f1_w);
    let op_f1_w = occ_lc(&ap_f1_w, npts);
    
    let bg1_w = op_f1_w[2048 * npts..(2048 * npts + 100)]
        .iter()
        .sum::<f64>() / 100.0;
    
    let mut file = File::create("quaoar_wider_ring.csv").expect("Unable to create file");
    writeln!(file, "time_sec,normalized_flux").expect("Unable to write header");
    
    for i in 0..npts {
        writeln!(
            file,
            "{},{}",
            t_sec[i],
            op_f1_w[2048 * npts + i] / bg1_w
        ).expect("Unable to write data");
    }
    
    let elapsed = start.elapsed();
    println!("Wider ring simulation completed in {:.2?}", elapsed);
    
    // Run higher resolution simulation
    let npts_high = 4096 * 2;
    println!("Running high resolution simulation with npts = {}...", npts_high);
    let start = Instant::now();
    
    let (ap_high, fov_high, grid_sz_high) = ring_seg_ap(lam, d, npts_high, wid_w, &t_f1_w);
    let op_high = occ_lc(&ap_high, npts_high);
    
    let bg_high = op_high[2048 * npts_high..(2048 * npts_high + 100)]
        .iter()
        .sum::<f64>() / 100.0;
    
    // Calculate r_km and t_sec values for high resolution
    let mut r_km_high = vec![0.0; npts_high];
    let mut t_sec_high = vec![0.0; npts_high];
    
    for i in 0..npts_high {
        r_km_high[i] = grid_sz_high * ((i as f64) - (npts_high as f64) / 2.0);
        t_sec_high[i] = r_km_high[i] / v_event;
    }
    
    let mut file = File::create("quaoar_high_res.csv").expect("Unable to create file");
    writeln!(file, "time_sec,normalized_flux").expect("Unable to write header");
    
    for i in 0..npts_high {
        writeln!(
            file,
            "{},{}",
            t_sec_high[i],
            op_high[2048 * npts_high + i] / bg_high
        ).expect("Unable to write data");
    }
    
    let elapsed = start.elapsed();
    println!("High resolution simulation completed in {:.2?}", elapsed);
    println!("FOV: {}, Grid Size: {}", fov, grid_sz);
    println!("All simulations completed successfully!");
}

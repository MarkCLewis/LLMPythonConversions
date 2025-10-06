use ndarray::{s, Array1, Array2, Axis};
use num_complex::Complex64 as C64;
use rayon::prelude::*;
use rustfft::{Fft, FftPlanner};
use std::f64::consts::PI;
use std::time::Instant;

/// ---------- Utilities ----------

#[inline]
fn linspace(start: f64, end: f64, n: usize) -> Array1<f64> {
    let step = if n > 1 { (end - start) / (n as f64 - 1.0) } else { 0.0 };
    Array1::from_iter((0..n).map(|i| start + step * i as f64))
}

/// Simple 1-D linear interpolation f(x) given (xs, ys), xs strictly increasing.
#[derive(Clone)]
struct LinInterp {
    xs: Vec<f64>,
    ys: Vec<f64>,
}
impl LinInterp {
    fn new(xs: &[f64], ys: &[f64]) -> Self {
        assert!(xs.len() == ys.len() && xs.len() >= 2);
        // ensure strictly increasing
        for i in 1..xs.len() {
            assert!(xs[i] > xs[i - 1], "xs must be strictly increasing");
        }
        Self { xs: xs.to_vec(), ys: ys.to_vec() }
    }
    #[inline]
    fn eval(&self, x: f64) -> f64 {
        let n = self.xs.len();
        if x <= self.xs[0] {
            return self.ys[0];
        }
        if x >= self.xs[n - 1] {
            return self.ys[n - 1];
        }
        // binary search for interval
        let mut lo = 0usize;
        let mut hi = n - 1;
        while hi - lo > 1 {
            let mid = (lo + hi) / 2;
            if self.xs[mid] <= x { lo = mid; } else { hi = mid; }
        }
        let x0 = self.xs[lo];
        let x1 = self.xs[hi];
        let y0 = self.ys[lo];
        let y1 = self.ys[hi];
        y0 + (y1 - y0) * (x - x0) / (x1 - x0)
    }
}

/// Circular shift (roll) of a 2-D array along both axes by k rows/cols.
fn roll2(a: &Array2<f64>, k: isize) -> Array2<f64> {
    let (ny, nx) = a.dim();
    let ky = ((k % ny as isize) + ny as isize) as usize % ny;
    let kx = ((k % nx as isize) + nx as isize) as usize % nx;

    // roll rows
    let mut tmp = Array2::<f64>::zeros((ny, nx));
    tmp
        .slice_mut(s![..ny - ky, ..])
        .assign(&a.slice(s![ky.., ..]));
    tmp
        .slice_mut(s![ny - ky.., ..])
        .assign(&a.slice(s![..ky, ..]));

    // roll cols
    let mut out = Array2::<f64>::zeros((ny, nx));
    out
        .slice_mut(s![.., ..nx - kx])
        .assign(&tmp.slice(s![.., kx..]));
    out
        .slice_mut(s![.., nx - kx..])
        .assign(&tmp.slice(s![.., ..kx]));
    out
}

/// In-place 2-D complex FFT: row FFTs then column FFTs (both forward).
fn fft2_inplace(a: &mut Array2<C64>) {
    let (ny, nx) = a.dim();
    let mut planner = FftPlanner::<f64>::new();
    let fft_x: std::sync::Arc<dyn Fft<f64>> = planner.plan_fft_forward(nx);
    let fft_y: std::sync::Arc<dyn Fft<f64>> = planner.plan_fft_forward(ny);

    // Row FFTs in parallel
    a.axis_iter_mut(Axis(0))
        .into_par_iter()
        .for_each(|mut row| {
            let slice = row.as_slice_mut().unwrap();
            fft_x.process(slice);
        });

    // Column FFTs in parallel: copy each column, transform, write back
    (0..nx).into_par_iter().for_each(|j| {
        // gather column j
        let mut col: Vec<C64> = (0..ny).map(|i| a[[i, j]]).collect();
        fft_y.process(&mut col);
        // scatter back
        for i in 0..ny {
            a[[i, j]] = col[i];
        }
    });
}

/// ---------- Physics/Optics pieces (ported) ----------

/// Occultation E-field power in observers' plane (Trester 1999 style).
/// Input: real aperture ap (transmission), square n×n.
/// Returns: rolled |FFT(ap * exp(i*pi/n * (x^2 + y^2)))|^2
fn occ_lc(ap: &Array2<f64>) -> Array2<f64> {
    let n = ap.dim().0;
    assert_eq!(ap.dim().0, ap.dim().1, "aperture must be square");
    let n2 = (n / 2) as isize;

    // Precompute separable phase multipliers
    let idx: Vec<isize> = (0..n as isize).map(|v| v - n as isize / 2).collect();
    let e_row: Vec<C64> = idx
        .iter()
        .map(|&y| {
            let phase = (PI / n as f64) * (y as f64).powi(2);
            C64::from_polar(1.0, phase)
        })
        .collect();
    let e_col: Vec<C64> = idx
        .iter()
        .map(|&x| {
            let phase = (PI / n as f64) * (x as f64).powi(2);
            C64::from_polar(1.0, phase)
        })
        .collect();

    // M = ap * eTerm (separable)
    let mut m = Array2::<C64>::zeros((n, n));
    m.indexed_iter_mut().for_each(|((y, x), v)| {
        *v = C64::from(ap[[y, x]]) * e_row[y] * e_col[x];
    });

    // E_obs = FFT2(M)
    fft2_inplace(&mut m);

    // power = |E|^2
    let mut p = Array2::<f64>::zeros((n, n));
    p.indexed_iter_mut().for_each(|((y, x), v)| {
        let z = m[[y, x]];
        *v = z.norm_sqr();
    });

    // roll by n/2 like numpy np.roll(..., n2, axis)
    roll2(&p, n2)
}

/// Elliptical solid-body aperture (obscuration). Not used in main, provided for parity.
#[allow(dead_code)]
fn solid_ap(fov_km: f64, npts: usize, a_km: f64, b_km: f64) -> Array2<f64> {
    let fov2 = 0.5 * fov_km;
    let mpts = linspace(-fov2, fov2, npts);
    let (ny, nx) = (npts, npts);
    let mut ap = Array2::<f64>::ones((ny, nx));
    for (iy, y) in mpts.iter().enumerate() {
        for (ix, x) in mpts.iter().enumerate() {
            let inside = (y / b_km).powi(2) + (x / a_km).powi(2) < 1.0;
            if inside {
                ap[[iy, ix]] = 0.0; // blocked
            }
        }
    }
    ap
}

/// Ring segment aperture (vertical segment).
/// lam_μm: microns; D_km: distance (km); npts: array size; wid_km: ring width (km)
/// t_r: transmission function vs radial distance from ring center (km)
fn ringseg_ap(
    lam_um: f64,
    d_km: f64,
    npts: usize,
    wid_km: f64,
    t_r: &LinInterp,
) -> (Array2<f64>, f64, f64) {
    let lam_km = lam_um * 1e-9; // microns → km
    let grid_sz = (lam_km * d_km / npts as f64).sqrt();
    let fov = (lam_km * d_km * npts as f64).sqrt();
    let n2 = (npts as isize) / 2;
    let wid2 = 0.5 * wid_km;

    // x grid is column coordinate (km) centered on zero; y is irrelevant except for tiling
    let xv: Vec<f64> = (0..npts)
        .map(|i| (i as isize - n2) as f64 * grid_sz)
        .collect();

    let mut ap = Array2::<f64>::ones((npts, npts));
    // For columns within ±wid2, apply the radial transmission t_r(|x|)
    for (ix, &xkm) in xv.iter().enumerate() {
        if xkm.abs() <= wid2 {
            // fill entire column with transmission at this radius;
            // matches Python's column-wise assignment
            let tr = t_r.eval(xkm);
            ap.slice_mut(s![.., ix]).fill(tr);
        }
    }
    (ap, fov, grid_sz)
}

/// Build the four radial profiles (tau → transmission) from the Python notebook.
fn build_transmission_profiles(wid_km: f64) -> (LinInterp, LinInterp, LinInterp, LinInterp) {
    let wid2 = 0.5 * wid_km;
    let n_rad = 100usize;
    let w = linspace(-wid2, wid2, n_rad);

    // Flat tau
    let tau_flat_01 = Array1::from_elem(n_rad, 0.1);
    let tau_flat_10 = Array1::from_elem(n_rad, 1.0);

    // Parabola across width
    let pb = &w * &w;
    let pb_max = pb.iter().cloned().fold(f64::NEG_INFINITY, f64::max);
    let pb_min = pb.iter().cloned().fold(f64::INFINITY, f64::min);

    // Centrally peaked: tau ranges 0.1 (edges) → 0.0 (center) in Python it was described,
    // but the code sets targMax=0.0, targMin=0.1 then scales m,b and does tauCP = m*pb + b.
    // Replicate exactly:
    let targ_max_cp = 0.0_f64;
    let targ_min_cp = 0.1_f64;
    let m_cp = (targ_max_cp - targ_min_cp) / (pb_max - pb_min);
    let b_cp = targ_max_cp - m_cp * pb_max;
    let tau_cp = pb.mapv(|v| m_cp * v + b_cp);

    // Peaked at edges: targMax=0.1, targMin=0.0
    let targ_max_se = 0.1_f64;
    let targ_min_se = 0.0_f64;
    let m_se = (targ_max_se - targ_min_se) / (pb_max - pb_min);
    let b_se = targ_max_se - m_se * pb_max;
    let tau_se = pb.mapv(|v| m_se * v + b_se);

    // Transmission = exp(-tau)
    let t1 = tau_flat_01.mapv(|t| (-t).exp());
    let t2 = tau_flat_10.mapv(|t| (-t).exp());
    let tcp = tau_cp.mapv(|t| (-t).exp());
    let tse = tau_se.mapv(|t| (-t).exp());

    (
        LinInterp::new(w.as_slice().unwrap(), t1.as_slice().unwrap()),
        LinInterp::new(w.as_slice().unwrap(), t2.as_slice().unwrap()),
        LinInterp::new(w.as_slice().unwrap(), tcp.as_slice().unwrap()),
        LinInterp::new(w.as_slice().unwrap(), tse.as_slice().unwrap()),
    )
}

/// Convenience: median of a slice
fn median(v: &[f64]) -> f64 {
    let mut w = v.to_vec();
    w.sort_by(|a, b| a.partial_cmp(b).unwrap());
    let n = w.len();
    if n % 2 == 1 { w[n / 2] } else { 0.5 * (w[n / 2 - 1] + w[n / 2]) }
}

fn main() -> Result<(), Box<dyn std::error::Error>> {
    // --- Parameters replicated from the notebook ---
    let lam_um = 0.5_f64;                 // wavelength (microns)
    let d_km = 43.0 * 150e6_f64;          // ~43 AU in km (as in notebook)
    let npts = 4096usize;                 // grid size
    let wid_km = 46.0_f64;                // ring width for first set
    let (tF1, tF2, tCP, tSE) = build_transmission_profiles(wid_km);

    // Build apertures
    let (ap1, fov, grid_sz) = ringseg_ap(lam_um, d_km, npts, wid_km, &tF1);
    let (ap2, _ , _      ) = ringseg_ap(lam_um, d_km, npts, wid_km, &tF2);
    let (ap3, _ , _      ) = ringseg_ap(lam_um, d_km, npts, wid_km, &tCP);
    let (ap4, _ , _      ) = ringseg_ap(lam_um, d_km, npts, wid_km, &tSE);

    // Observer-plane intensity
    let op1 = occ_lc(&ap1);
    let op2 = occ_lc(&ap2);
    let op3 = occ_lc(&ap3);
    let op4 = occ_lc(&ap4);

    // Normalize to out-of-event baseline (median of row 2048, first 100 cols)
    let row_idx = 2048usize.min(npts - 1);
    let bg1 = median(op1.slice(s![row_idx, 0..100]).as_slice().unwrap());
    let bg2 = median(op2.slice(s![row_idx, 0..100]).as_slice().unwrap());
    let bg3 = median(op3.slice(s![row_idx, 0..100]).as_slice().unwrap());
    let bg4 = median(op4.slice(s![row_idx, 0..100]).as_slice().unwrap());

    let op1n = &op1 / bg1;
    let op2n = &op2 / bg2;
    let op3n = &op3 / bg3;
    let op4n = &op4 / bg4;

    // Time axis along central row
    let r_km: Vec<f64> = (0..npts)
        .map(|i| grid_sz * (i as f64 - npts as f64 / 2.0))
        .collect();
    let v_event = 5.8_f64; // km/s
    let t_sec: Vec<f64> = r_km.iter().map(|r| r / v_event).collect();

    // Write lightcurves to CSV
    {
        let mut wtr = csv::Writer::from_path("lightcurves_46km.csv")?;
        wtr.write_record(&["t_sec", "flat_tau_0p1", "flat_tau_1p0", "central_peak", "edge_peak"])?;
        for i in 0..npts {
            wtr.write_record(&[
                format!("{}", t_sec[i]),
                format!("{}", op1n[[row_idx, i]]),
                format!("{}", op2n[[row_idx, i]]),
                format!("{}", op3n[[row_idx, i]]),
                format!("{}", op4n[[row_idx, i]]),
            ])?;
        }
        wtr.flush()?;
    }

    // Second scenario: wider, low-tau ring (matches notebook’s later block)
    let wid_w = 100.0_f64;
    let nrad = 200usize;
    let w = linspace(-0.5 * wid_w, 0.5 * wid_w, nrad);
    let tau_flat01 = Array1::from_elem(nrad, 0.1);
    let tF1W = LinInterp::new(w.as_slice().unwrap(),
                              tau_flat01.mapv(|t| (-t).exp()).as_slice().unwrap());

    for mul in [1usize, 2, 3] {
        let npts_w = npts * mul;
        let start = Instant::now();
        let (apw, fovw, gridw) = ringseg_ap(lam_um, d_km, npts_w, wid_w, &tF1W);
        let opw = occ_lc(&apw);
        let bg = median(opw.slice(s![npts_w/2, 0..100]).as_slice().unwrap());
        let opw = &opw / bg;
        let secs = start.elapsed().as_secs_f64();

        // save the central row
        let r_km_w: Vec<f64> = (0..npts_w)
            .map(|i| gridw * (i as f64 - npts_w as f64 / 2.0))
            .collect();
        let t_sec_w: Vec<f64> = r_km_w.iter().map(|r| r / v_event).collect();

        let fname = format!("lightcurve_wide_{}x.csv", mul);
        let mut wtr = csv::Writer::from_path(&fname)?;
        wtr.write_record(&["t_sec", "flux"])?;
        for i in 0..npts_w {
            wtr.write_record(&[
                format!("{}", t_sec_w[i]),
                format!("{}", opw[[npts_w/2, i]]),
            ])?;
        }
        wtr.flush()?;

        println!(
            "npts={} ⇒ FOV={:.6} km, grid={:.6} km, elapsed {:.3} s (wrote {})",
            npts_w, fovw, gridw, secs, fname
        );
    }

    println!("FOV={:.6} km, grid={:.6} km (first scenario)", fov, grid_sz);
    println!("Done. CSVs: lightcurves_46km.csv, lightcurve_wide_{1x,2x,3x}.csv");
    Ok(())
}

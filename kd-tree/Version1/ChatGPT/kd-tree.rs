use rand::prelude::*;
use rayon::prelude::*;
use std::cmp::Ordering;
use std::f64::consts::PI;

// ===== parameters (match the Python) =========================================
const MAX_PARTS: usize = 7;   // leaf threshold
const THETA: f64 = 0.3;       // Barnes–Hut opening angle
const SOFTENING2: f64 = 0.0;  // set >0 if you want Plummer softening

// ===== basic math =============================================================
#[derive(Clone, Copy, Debug, Default)]
struct Vec3 { x: f64, y: f64, z: f64 }
impl Vec3 {
    #[inline] fn new(x: f64, y: f64, z: f64) -> Self { Self { x, y, z } }
    #[inline] fn add(self, o: Self) -> Self { Self::new(self.x + o.x, self.y + o.y, self.z + o.z) }
    #[inline] fn sub(self, o: Self) -> Self { Self::new(self.x - o.x, self.y - o.y, self.z - o.z) }
    #[inline] fn scale(self, s: f64) -> Self { Self::new(self.x * s, self.y * s, self.z * s) }
    #[inline] fn dot(self, o: Self) -> f64 { self.x*o.x + self.y*o.y + self.z*o.z }
}

// ===== particle ===============================================================
#[derive(Clone, Debug)]
struct Particle {
    p: Vec3,  // position
    v: Vec3,  // velocity
    m: f64,   // mass
}

#[inline]
fn coord(p: &Particle, dim: usize) -> f64 {
    match dim {
        0 => p.p.x,
        1 => p.p.y,
        _ => p.p.z,
    }
}

// Pairwise accel on a due to b: a <- a - m_b*(r_vec)/|r|^3
#[inline]
fn accel_pair(a: &Particle, b: &Particle) -> Vec3 {
    let dp = a.p.sub(b.p);
    let r2 = dp.dot(dp) + SOFTENING2;
    if r2 == 0.0 { return Vec3::new(0.0, 0.0, 0.0); }
    let r = r2.sqrt();
    let mag = -b.m / (r2 * r);
    dp.scale(mag)
}

// ===== KD node ================================================================
#[derive(Clone, Debug)]
struct KDNode {
    // leaf: Some(particle_indices); internal: None
    leaf_particles: Option<Vec<usize>>,

    // internal data (valid when leaf_particles is None)
    split_dim: usize,
    split_val: f64,
    m: f64,
    cm: Vec3,
    size: f64,        // extent along split_dim
    left: usize,      // index of left child in nodes vec
    right: usize,     // index of right child
}
impl KDNode {
    fn new_leaf(idxs: &[usize]) -> Self {
        Self {
            leaf_particles: Some(idxs.to_vec()),
            split_dim: 0, split_val: 0.0, m: 0.0, cm: Vec3::default(),
            size: 0.0, left: 0, right: 0,
        }
    }
    fn new_internal(split_dim: usize, split_val: f64, m: f64, cm: Vec3, size: f64, left: usize, right: usize) -> Self {
        Self {
            leaf_particles: None,
            split_dim, split_val, m, cm, size, left, right,
        }
    }
    #[inline]
    fn is_leaf(&self) -> bool { self.leaf_particles.is_some() }
}

// ===== Quickselect nth_element over index slice by coordinate ================
fn nth_element_indices(
    idx: &mut [usize],
    parts: &[Particle],
    split_dim: usize,
    n_th: usize, // 0-based inside idx-slice
) {
    let mut rng = thread_rng();
    let mut left = 0usize;
    let mut right = idx.len();

    while left + 1 < right {
        let pivot_i = left + rng.gen_range(0..(right - left));
        idx.swap(left, pivot_i);

        // partition around pivot at left
        let pv = coord(&parts[idx[left]], split_dim);
        let mut lo = left + 1;
        let mut hi = right - 1;

        while lo <= hi {
            if coord(&parts[idx[lo]], split_dim) < pv {
                lo += 1;
            } else {
                idx.swap(lo, hi);
                if hi == 0 { break; }
                hi -= 1;
            }
        }
        idx.swap(left, hi);

        match hi.cmp(&(left + n_th)) {
            Ordering::Less => left = hi + 1,
            Ordering::Greater => right = hi,
            Ordering::Equal => return,
        }
    }
}

// ===== Tree building =========================================================
struct KDTree {
    nodes: Vec<KDNode>,
    indices: Vec<usize>, // permutation of 0..n
}

impl KDTree {
    fn build(parts: &[Particle]) -> Self {
        let n = parts.len();
        let mut tree = Self { nodes: Vec::with_capacity(2 * (n / (MAX_PARTS.saturating_sub(1).max(1))) + 16),
                              indices: (0..n).collect() };
        // reserve a root slot
        tree.rec_build(parts, 0, n, 0);
        tree
    }

    // returns last node index created (continuous layout)
    fn rec_build(&mut self, parts: &[Particle], start: usize, end: usize, cur_node: usize) -> usize {
        if self.nodes.len() <= cur_node {
            self.nodes.resize_with(cur_node + 1, || KDNode::new_leaf(&[]));
        }

        let count = end - start;
        if count <= MAX_PARTS {
            // leaf
            let leaf = KDNode::new_leaf(&self.indices[start..end]);
            self.nodes[cur_node] = leaf;
            return cur_node;
        }

        // bounding box + mass/CM
        let mut minv = Vec3::new( f64::INFINITY,  f64::INFINITY,  f64::INFINITY);
        let mut maxv = Vec3::new(-f64::INFINITY, -f64::INFINITY, -f64::INFINITY);
        let mut m = 0.0;
        let mut cm = Vec3::new(0.0, 0.0, 0.0);
        for &i in &self.indices[start..end] {
            let p = &parts[i];
            m += p.m;
            cm = cm.add(p.p.scale(p.m));
            if p.p.x < minv.x { minv.x = p.p.x; }
            if p.p.y < minv.y { minv.y = p.p.y; }
            if p.p.z < minv.z { minv.z = p.p.z; }
            if p.p.x > maxv.x { maxv.x = p.p.x; }
            if p.p.y > maxv.y { maxv.y = p.p.y; }
            if p.p.z > maxv.z { maxv.z = p.p.z; }
        }
        cm = cm.scale(1.0 / m);

        let span = [maxv.x - minv.x, maxv.y - minv.y, maxv.z - minv.z];
        let mut split_dim = 0usize;
        if span[1] > span[split_dim] { split_dim = 1; }
        if span[2] > span[split_dim] { split_dim = 2; }
        let size = span[split_dim];

        // partition indices around median on split_dim
        let mid = (start + end) / 2;
        nth_element_indices(&mut self.indices[start..end], parts, split_dim, mid - start);
        let split_val = coord(&parts[self.indices[mid]], split_dim);

        // recurse: left is next slot, right follows left subtree
        let left_root = cur_node + 1;
        let left_last = self.rec_build(parts, start, mid, left_root);
        let right_root = left_last + 1;
        let right_last = self.rec_build(parts, mid, end, right_root);

        self.nodes[cur_node] = KDNode::new_internal(split_dim, split_val, m, cm, size, left_root, right_root);
        right_last
    }

    // acceleration at particle i
    fn accel(&self, i: usize, parts: &[Particle]) -> Vec3 {
        self.accel_recur(0, i, parts)
    }

    fn accel_recur(&self, node_idx: usize, i: usize, parts: &[Particle]) -> Vec3 {
        let nd = &self.nodes[node_idx];
        if let Some(ref ids) = nd.leaf_particles {
            let mut a = Vec3::new(0.0, 0.0, 0.0);
            for &q in ids {
                if q != i {
                    a = a.add(accel_pair(&parts[i], &parts[q]));
                }
            }
            a
        } else {
            let dp = parts[i].p.sub(nd.cm);
            let r2 = dp.dot(dp);
            if nd.size * nd.size < (THETA * THETA) * r2 {
                // use COM approximation
                let r = r2.sqrt();
                if r == 0.0 { return Vec3::new(0.0, 0.0, 0.0); }
                let mag = -nd.m / (r2 * r);
                dp.scale(mag)
            } else {
                let a_l = self.accel_recur(nd.left,  i, parts);
                let a_r = self.accel_recur(nd.right, i, parts);
                a_l.add(a_r)
            }
        }
    }
}

// ===== Simple stepper (like your simple_sim) =================================
fn step_system(parts: &mut [Particle], dt: f64) {
    let tree = KDTree::build(parts);

    // Parallel per-body accelerations
    let accels: Vec<Vec3> = (0..parts.len())
        .into_par_iter()
        .map(|i| tree.accel(i, parts))
        .collect();

    // Kick-drift Euler (matches typical simple loop)
    for (p, a) in parts.iter_mut().zip(accels.into_iter()) {
        p.v = p.v.add(a.scale(dt));
        p.p = p.p.add(p.v.scale(dt));
    }
}

// ===== Demo main =============================================================
fn main() {
    // Small demo; adjust N/steps to taste
    let n = 20_000usize;
    let steps = 5usize;
    let dt = 1e-3_f64;

    let mut rng = StdRng::seed_from_u64(42);
    let mut bodies: Vec<Particle> = (0..n).map(|_| {
        // Put points in a noisy ring for something nontrivial
        let r = 50.0 + 10.0 * (rng.gen::<f64>() - 0.5);
        let ang = 2.0 * PI * rng.gen::<f64>();
        let z = 2.0 * (rng.gen::<f64>() - 0.5);
        Particle {
            p: Vec3::new(r * ang.cos(), r * ang.sin(), z),
            v: Vec3::new(0.0, 0.0, 0.0),
            m: 1.0 / n as f64,
        }
    }).collect();

    for s in 0..steps {
        println!("step {}", s);
        step_system(&mut bodies, dt);
    }

    // Print final COM as a tiny check
    let (mut cm, mut msum) = (Vec3::new(0.0,0.0,0.0), 0.0);
    for p in &bodies { cm = cm.add(p.p.scale(p.m)); msum += p.m; }
    cm = cm.scale(1.0 / msum);
    println!("final COM ≈ ({:.6}, {:.6}, {:.6})", cm.x, cm.y, cm.z);
}

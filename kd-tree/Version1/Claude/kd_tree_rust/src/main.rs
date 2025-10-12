use std::cmp::Ordering;
use std::fs::File;
use std::io::Write;
use std::time::Instant;
use rand::Rng;
use std::env;

// Constants
const MAX_PARTS: usize = 7;
const THETA: f64 = 0.3;

// 3D vector structure
#[derive(Clone, Copy, Debug)]
struct F64x3 {
    x: f64,
    y: f64,
    z: f64,
}

impl F64x3 {
    fn new(x: f64, y: f64, z: f64) -> Self {
        Self { x, y, z }
    }

    fn zeros() -> Self {
        Self::new(0.0, 0.0, 0.0)
    }

    fn ones(scalar: f64) -> Self {
        Self::new(scalar, scalar, scalar)
    }

    fn min(&self, other: &Self) -> Self {
        Self::new(
            self.x.min(other.x),
            self.y.min(other.y),
            self.z.min(other.z),
        )
    }

    fn max(&self, other: &Self) -> Self {
        Self::new(
            self.x.max(other.x),
            self.y.max(other.y),
            self.z.max(other.z),
        )
    }

    fn dot(&self, other: &Self) -> f64 {
        self.x * other.x + self.y * other.y + self.z * other.z
    }
}

impl std::ops::Add for F64x3 {
    type Output = Self;

    fn add(self, other: Self) -> Self {
        Self::new(self.x + other.x, self.y + other.y, self.z + other.z)
    }
}

impl std::ops::AddAssign for F64x3 {
    fn add_assign(&mut self, other: Self) {
        *self = Self::new(self.x + other.x, self.y + other.y, self.z + other.z);
    }
}

impl std::ops::Sub for F64x3 {
    type Output = Self;

    fn sub(self, other: Self) -> Self {
        Self::new(self.x - other.x, self.y - other.y, self.z - other.z)
    }
}

impl std::ops::Mul<f64> for F64x3 {
    type Output = Self;

    fn mul(self, scalar: f64) -> Self {
        Self::new(self.x * scalar, self.y * scalar, self.z * scalar)
    }
}

impl std::ops::Div<f64> for F64x3 {
    type Output = Self;

    fn div(self, scalar: f64) -> Self {
        Self::new(self.x / scalar, self.y / scalar, self.z / scalar)
    }
}

impl std::ops::Index<usize> for F64x3 {
    type Output = f64;

    fn index(&self, idx: usize) -> &Self::Output {
        match idx {
            0 => &self.x,
            1 => &self.y,
            2 => &self.z,
            _ => panic!("F64x3 index out of bounds"),
        }
    }
}

// Particle structure
#[derive(Clone, Debug)]
struct Particle {
    m: f64,    // mass
    p: F64x3,  // position
    v: F64x3,  // velocity
}

impl Particle {
    fn new(m: f64, p: F64x3, v: F64x3) -> Self {
        Self { m, p, v }
    }
}

// Calculate particle-particle acceleration
fn calc_pp_accel(a: &Particle, b: &Particle) -> F64x3 {
    let dp = b.p - a.p;
    let dist_sqr = dp.dot(&dp);
    let dist = dist_sqr.sqrt();
    let magi = -b.m / (dist_sqr * dist);
    dp * magi
}

// KD-tree node structure
#[derive(Clone, Debug)]
struct KDTree {
    // For leaves
    num_parts: usize,
    particles: Vec<usize>,

    // For internal nodes
    split_dim: usize,
    split_val: f64,
    m: f64,
    cm: F64x3,
    size: f64,
    left: usize,
    right: usize,
}

impl KDTree {
    fn new_leaf(num_parts: usize, particles: Vec<usize>) -> Self {
        Self {
            num_parts,
            particles,
            split_dim: 0,
            split_val: 0.0,
            m: 0.0,
            cm: F64x3::zeros(),
            size: 0.0,
            left: 0,
            right: 0,
        }
    }

    fn new_internal(
        split_dim: usize,
        split_val: f64,
        m: f64,
        cm: F64x3,
        size: f64,
        left: usize,
        right: usize,
    ) -> Self {
        Self {
            num_parts: 0,
            particles: Vec::new(),
            split_dim,
            split_val,
            m,
            cm,
            size,
            left,
            right,
        }
    }
}

// System structure
struct System {
    indices: Vec<usize>,
    nodes: Vec<KDTree>,
}

impl System {
    fn new(n: usize) -> Self {
        let indices = (0..n).collect();
        let num_nodes = 2 * (n / (MAX_PARTS - 1) + 1);
        let nodes = vec![KDTree::new_leaf(0, Vec::new()); num_nodes];

        Self { indices, nodes }
    }

    // Build KD-tree
    fn build_tree(
        &mut self,
        start: usize,
        end: usize,
        particles: &[Particle],
        cur_node: usize,
    ) -> usize {
        let np1 = end - start;

        // Ensure we have enough space in the nodes vector
        while cur_node >= self.nodes.len() {
            self.nodes.push(KDTree::new_leaf(0, Vec::new()));
        }

        if np1 <= MAX_PARTS {
            // Leaf node
            let mut leaf_particles = Vec::with_capacity(np1);
            for i in 0..np1 {
                leaf_particles.push(self.indices[start + i]);
            }

            self.nodes[cur_node] = KDTree::new_leaf(np1, leaf_particles);
            return cur_node;
        } else {
            // Internal node
            // Find bounds and center of mass
            let mut min_val = F64x3::ones(1e100);
            let mut max_val = F64x3::ones(-1e100);
            let mut m = 0.0;
            let mut cm = F64x3::zeros();

            for i in start..end {
                let p = &particles[self.indices[i]];
                m += p.m;
                cm += p.p * p.m;
                min_val = min_val.min(&p.p);
                max_val = max_val.max(&p.p);
            }

            cm = cm / m;

            // Find split dimension (dimension with largest spread)
            let mut split_dim = 0;
            for dim in 1..3 {
                if max_val[dim] - min_val[dim] > max_val[split_dim] - min_val[split_dim] {
                    split_dim = dim;
                }
            }

            let size = max_val[split_dim] - min_val[split_dim];

            // Partition particles on split_dim
            let mid = (start + end) / 2;
            let mut s = start;
            let mut e = end;

            while s + 1 < e {
                let pivot = rand::thread_rng().gen_range(s..e);
                self.indices.swap(s, pivot);

                let mut low = s + 1;
                let mut high = e - 1;
                let pivot_val = particles[self.indices[s]].p[split_dim];

                while low <= high {
                    if particles[self.indices[low]].p[split_dim] < pivot_val {
                        low += 1;
                    } else {
                        self.indices.swap(low, high);
                        high -= 1;
                    }
                }

                self.indices.swap(s, high);

                match high.cmp(&mid) {
                    Ordering::Less => s = high + 1,
                    Ordering::Greater => e = high,
                    Ordering::Equal => s = e,
                }
            }

            let split_val = particles[self.indices[mid]].p[split_dim];

            // Recurse on children
            let left = self.build_tree(start, mid, particles, cur_node + 1);
            let right = self.build_tree(mid, end, particles, left + 1);

            // Create internal node
            self.nodes[cur_node] = KDTree::new_internal(
                split_dim,
                split_val,
                m,
                cm,
                size,
                cur_node + 1,
                left + 1,
            );

            return right;
        }
    }
}

// Calculate acceleration recursively through tree
fn accel_recur(cur_node: usize, p: usize, particles: &[Particle], nodes: &[KDTree]) -> F64x3 {
    if nodes[cur_node].num_parts > 0 {
        // Leaf node - direct calculation
        let mut acc = F64x3::zeros();

        for &particle_idx in &nodes[cur_node].particles {
            if particle_idx != p {
                acc += calc_pp_accel(&particles[p], &particles[particle_idx]);
            }
        }

        acc
    } else {
        // Internal node
        let dp = particles[p].p - nodes[cur_node].cm;
        let dist_sqr = dp.dot(&dp);

        if nodes[cur_node].size * nodes[cur_node].size < THETA * THETA * dist_sqr {
            // Far enough to use approximation
            let dist = dist_sqr.sqrt();
            let magi = -nodes[cur_node].m / (dist_sqr * dist);
            dp * magi
        } else {
            // Too close, need to recurse
            let left_acc = accel_recur(nodes[cur_node].left, p, particles, nodes);
            let right_acc = accel_recur(nodes[cur_node].right, p, particles, nodes);
            left_acc + right_acc
        }
    }
}

// Calculate acceleration for a particle
fn calc_accel(p: usize, particles: &[Particle], nodes: &[KDTree]) -> F64x3 {
    accel_recur(0, p, particles, nodes)
}

// Run simulation for given number of steps
fn simple_sim(bodies: &mut [Particle], dt: f64, steps: usize, print_steps: bool) {
    let mut sys = System::new(bodies.len());

    for step in 0..steps {
        if print_steps {
            println!("Step {}", step);
        }

        // Reset indices
        for i in 0..bodies.len() {
            sys.indices[i] = i;
        }

        // Build tree
        sys.build_tree(0, bodies.len(), bodies, 0);

        // Calculate accelerations
        let mut acc = vec![F64x3::zeros(); bodies.len()];
        for i in 0..bodies.len() {
            acc[i] = calc_accel(i, bodies, &sys.nodes);
        }

        // Update positions and velocities
        for (i, body) in bodies.iter_mut().enumerate() {
            body.v += acc[i] * dt;
            body.p += body.v * dt;
        }
    }
}

// Print tree structure to file
fn print_tree(step: usize, tree: &[KDTree], particles: &[Particle]) {
    let filename = format!("tree{}.txt", step);
    let mut file = File::create(filename).expect("Failed to create file");

    writeln!(file, "{}", particles.len()).expect("Failed to write to file");

    for node in tree {
        if node.num_parts > 0 {
            // Leaf node
            writeln!(file, "L {}", node.num_parts).expect("Failed to write to file");

            for &p in &node.particles {
                writeln!(
                    file,
                    "{} {} {}",
                    particles[p].p.x, particles[p].p.y, particles[p].p.z
                )
                .expect("Failed to write to file");
            }
        } else if node.left > 0 && node.right > 0 {
            // Internal node
            writeln!(
                file,
                "I {} {} {} {}",
                node.split_dim, node.split_val, node.left, node.right
            )
            .expect("Failed to write to file");
        }
    }
}

// Test tree structure
fn recur_test_tree_struct(
    node: usize,
    nodes: &[KDTree],
    particles: &[Particle],
    mut min_bounds: F64x3,
    mut max_bounds: F64x3,
) {
    if nodes[node].num_parts > 0 {
        // Leaf node - check all particles
        for &i in &nodes[node].particles {
            for dim in 0..3 {
                let p_val = particles[i].p[dim];
                let min_val = match dim {
                    0 => min_bounds.x,
                    1 => min_bounds.y,
                    2 => min_bounds.z,
                    _ => unreachable!(),
                };
                let max_val = match dim {
                    0 => max_bounds.x,
                    1 => max_bounds.y,
                    2 => max_bounds.z,
                    _ => unreachable!(),
                };

                assert!(
                    p_val >= min_val,
                    "Particle dim {} is below min. i={} p={} min={}",
                    dim, i, p_val, min_val
                );
                assert!(
                    p_val < max_val,
                    "Particle dim {} is above max. i={} p={} max={}",
                    dim, i, p_val, max_val
                );
            }
        }
    } else {
        // Internal node - recurse on children
        let split_dim = nodes[node].split_dim;
        let tmin = match split_dim {
            0 => min_bounds.x,
            1 => min_bounds.y,
            2 => min_bounds.z,
            _ => unreachable!(),
        };
        let tmax = match split_dim {
            0 => max_bounds.x,
            1 => max_bounds.y,
            2 => max_bounds.z,
            _ => unreachable!(),
        };

        // Modify bounds for left child
        match split_dim {
            0 => max_bounds.x = nodes[node].split_val,
            1 => max_bounds.y = nodes[node].split_val,
            2 => max_bounds.z = nodes[node].split_val,
            _ => unreachable!(),
        }
        recur_test_tree_struct(nodes[node].left, nodes, particles, min_bounds, max_bounds);

        // Restore and modify bounds for right child
        match split_dim {
            0 => {
                max_bounds.x = tmax;
                min_bounds.x = nodes[node].split_val;
            }
            1 => {
                max_bounds.y = tmax;
                min_bounds.y = nodes[node].split_val;
            }
            2 => {
                max_bounds.z = tmax;
                min_bounds.z = nodes[node].split_val;
            }
            _ => unreachable!(),
        }
        recur_test_tree_struct(nodes[node].right, nodes, particles, min_bounds, max_bounds);

        // Restore min bounds
        match split_dim {
            0 => min_bounds.x = tmin,
            1 => min_bounds.y = tmin,
            2 => min_bounds.z = tmin,
            _ => unreachable!(),
        }
    }
}

fn main() {
    // Create some test particles
	let args: Vec<String> = env::args().collect();
    let num_particles: usize = args[1].parse().expect("Failed to parse string into a number!");
    let mut particles = Vec::with_capacity(num_particles);
    let mut rng = rand::thread_rng();

    // Initialize particles (random positions)
    for _ in 0..num_particles {
        particles.push(Particle::new(
            1.0,
            F64x3::new(
                rng.gen_range(-50.0..50.0),
                rng.gen_range(-50.0..50.0),
                rng.gen_range(-50.0..50.0),
            ),
            F64x3::new(
                rng.gen_range(-1.0..1.0),
                rng.gen_range(-1.0..1.0),
                rng.gen_range(-1.0..1.0),
            ),
        ));
    }

    // Run simulation
    let dt = 0.1;
    let steps: usize = args[2].parse().expect("Failed to parse string into a number!");
    let start = Instant::now();
    simple_sim(&mut particles, dt, steps, true);
    let duration = start.elapsed();

    println!("Simulation completed in {:?}", duration);
}

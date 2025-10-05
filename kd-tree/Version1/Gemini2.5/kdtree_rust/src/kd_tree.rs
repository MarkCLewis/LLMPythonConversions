use crate::particle::{calc_pp_accel, F64x3, Particle};
use rand::Rng;

const MAX_PARTS: usize = 7;
const THETA: f64 = 0.3;

// An enum is used to represent the two possible states of a node (Internal or Leaf).
// This is more type-safe than using `num_parts` as a flag.
enum KDTreeNode {
    Internal {
        split_dim: usize,
        split_val: f64,
        m: f64,
        cm: F64x3,
        size: f64,
        left: usize,
        right: usize,
    },
    Leaf {
        num_parts: usize,
        // Store indices into the main particle vector
        particles: [usize; MAX_PARTS],
    },
}

struct System {
    indices: Vec<usize>,
    nodes: Vec<KDTreeNode>,
    // Tracks the next available node index to avoid reallocations.
    next_node_idx: usize,
}

impl System {
    fn from_amount(n: usize) -> Self {
        let num_nodes = 2 * (n / (MAX_PARTS - 1) + 1);
        let nodes = Vec::with_capacity(num_nodes);
        System {
            indices: (0..n).collect(),
            nodes,
            next_node_idx: 0,
        }
    }

    fn new_node(&mut self, node: KDTreeNode) -> usize {
        let idx = self.next_node_idx;
        if idx >= self.nodes.len() {
            self.nodes.push(node);
        } else {
            self.nodes[idx] = node;
        }
        self.next_node_idx += 1;
        idx
    }

    fn build_tree(&mut self, start: usize, end: usize, particles: &[Particle]) -> usize {
        let np = end - start;
        if np <= MAX_PARTS {
            let mut part_indices = [0; MAX_PARTS];
            for i in 0..np {
                part_indices[i] = self.indices[start + i];
            }
            return self.new_node(KDTreeNode::Leaf {
                num_parts: np,
                particles: part_indices,
            });
        }

        let mut min_pos = F64x3::new(1e100, 1e100, 1e100);
        let mut max_pos = F64x3::new(-1e100, -1e100, -1e100);
        let mut m = 0.0;
        let mut cm = F64x3::default();

        for i in start..end {
            let p = &particles[self.indices[i]];
            m += p.m;
            cm += p.p * p.m;
            min_pos = min_pos.min(p.p);
            max_pos = max_pos.max(p.p);
        }
        cm /= m;

        let mut split_dim = 0;
        if max_pos.y - min_pos.y > max_pos.x - min_pos.x {
            split_dim = 1;
        }
        if max_pos.z - min_pos.z > max_pos.component(split_dim) - min_pos.component(split_dim) {
            split_dim = 2;
        }
        let size = max_pos.component(split_dim) - min_pos.component(split_dim);

        // Partition particles around the median
        let mid = start + (end - start) / 2;
        partition(&mut self.indices[start..end], particles, mid - start, split_dim);
        
        let split_val = particles[self.indices[mid]].p.component(split_dim);

        let left_idx = self.build_tree(start, mid, particles);
        let right_idx = self.build_tree(mid, end, particles);

        self.new_node(KDTreeNode::Internal {
            split_dim,
            split_val,
            m,
            cm,
            size,
            left: left_idx,
            right: right_idx,
        })
    }
}

// Partition a slice of indices using a pivot value
fn partition(indices: &mut [usize], particles: &[Particle], k: usize, dim: usize) {
    if indices.len() <= 1 {
        return;
    }

    let mut rng = rand::thread_rng();
    let mut low = 0;
    let mut high = indices.len() - 1;

    while low < high {
        let pivot_idx = rng.gen_range(low..=high);
        indices.swap(low, pivot_idx);
        let pivot_val = particles[indices[low]].p.component(dim);

        let mut i = low + 1;
        let mut j = high;

        loop {
            while i <= j && particles[indices[i]].p.component(dim) < pivot_val {
                i += 1;
            }
            while i <= j && particles[indices[j]].p.component(dim) > pivot_val {
                j -= 1;
            }
            if i > j {
                break;
            }
            indices.swap(i, j);
            i += 1;
            j -= 1;
        }
        indices.swap(low, j);

        if j == k {
            return;
        } else if j < k {
            low = j + 1;
        } else {
            high = j - 1;
        }
    }
}


fn accel_recur(
    cur_node_idx: usize,
    p_idx: usize,
    particles: &[Particle],
    nodes: &[KDTreeNode],
) -> F64x3 {
    match &nodes[cur_node_idx] {
        KDTreeNode::Leaf { num_parts, particles: node_particles } => {
            let mut acc = F64x3::default();
            for i in 0..*num_parts {
                let other_p_idx = node_particles[i];
                if p_idx != other_p_idx {
                    acc += calc_pp_accel(&particles[p_idx], &particles[other_p_idx]);
                }
            }
            acc
        }
        KDTreeNode::Internal { cm, m, size, left, right, .. } => {
            let dp = particles[p_idx].p - *cm;
            let dist_sqr = dp.dot(dp);
            if size * size < THETA * THETA * dist_sqr {
                let dist = dist_sqr.sqrt();
                let magi = -m / (dist_sqr * dist);
                dp * magi
            } else {
                accel_recur(*left, p_idx, particles, nodes) + accel_recur(*right, p_idx, particles, nodes)
            }
        }
    }
}

fn calc_accel(p_idx: usize, particles: &[Particle], nodes: &[KDTreeNode]) -> F64x3 {
    accel_recur(nodes.len() - 1, p_idx, particles, nodes)
}

pub fn simple_sim(bodies: &mut [Particle], dt: f64, steps: usize) {
    let n = bodies.len();
    if n == 0 {
        return;
    }
    
    let mut sys = System::from_amount(n);
    let mut acc = vec![F64x3::default(); n];

    for _step in 0..steps {
        sys.indices = (0..n).collect();
        sys.nodes.clear();
        sys.next_node_idx = 0;
        sys.build_tree(0, n, bodies);
        // The root node is the last one added.
        let root_idx = sys.nodes.len() - 1;

        for i in 0..n {
            acc[i] = accel_recur(root_idx, i, bodies, &sys.nodes);
        }

        for (i, body) in bodies.iter_mut().enumerate() {
            body.v += acc[i] * dt;
            body.p += body.v * dt;
        }
    }
}

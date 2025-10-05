use std::ops::{Add, AddAssign, Div, DivAssign, Mul, MulAssign, Sub};
use rand::Rng;

#[derive(Clone, Copy, Debug, Default)]
pub struct F64x3 {
    pub x: f64,
    pub y: f64,
    pub z: f64,
}

impl F64x3 {
    pub fn new(x: f64, y: f64, z: f64) -> Self {
        Self { x, y, z }
    }

    // Dot product
    pub fn dot(self, other: Self) -> f64 {
        self.x * other.x + self.y * other.y + self.z * other.z
    }

    pub fn min(self, other: Self) -> Self {
        Self {
            x: self.x.min(other.x),
            y: self.y.min(other.y),
            z: self.z.min(other.z),
        }
    }

    pub fn max(self, other: Self) -> Self {
        Self {
            x: self.x.max(other.x),
            y: self.y.max(other.y),
            z: self.z.max(other.z),
        }
    }

    pub fn component(self, i: usize) -> f64 {
        match i {
            0 => self.x,
            1 => self.y,
            2 => self.z,
            _ => panic!("Invalid index for F64x3"),
        }
    }

    pub fn set_component(&mut self, i: usize, val: f64) {
        match i {
            0 => self.x = val,
            1 => self.y = val,
            2 => self.z = val,
            _ => panic!("Invalid index for F64x3"),
        }
    }
}

// Operator Overloading for F64x3
impl Add for F64x3 {
    type Output = Self;
    fn add(self, rhs: Self) -> Self::Output {
        Self { x: self.x + rhs.x, y: self.y + rhs.y, z: self.z + rhs.z }
    }
}

impl AddAssign for F64x3 {
    fn add_assign(&mut self, rhs: Self) {
        self.x += rhs.x;
        self.y += rhs.y;
        self.z += rhs.z;
    }
}

impl Sub for F64x3 {
    type Output = Self;
    fn sub(self, rhs: Self) -> Self::Output {
        Self { x: self.x - rhs.x, y: self.y - rhs.y, z: self.z - rhs.z }
    }
}

impl Mul<f64> for F64x3 {
    type Output = Self;
    fn mul(self, rhs: f64) -> Self::Output {
        Self { x: self.x * rhs, y: self.y * rhs, z: self.z * rhs }
    }
}

impl MulAssign<f64> for F64x3 {
    fn mul_assign(&mut self, rhs: f64) {
        self.x *= rhs;
        self.y *= rhs;
        self.z *= rhs;
    }
}

impl Div<f64> for F64x3 {
    type Output = Self;
    fn div(self, rhs: f64) -> Self::Output {
        Self { x: self.x / rhs, y: self.y / rhs, z: self.z / rhs }
    }
}

impl DivAssign<f64> for F64x3 {
    fn div_assign(&mut self, rhs: f64) {
        self.x /= rhs;
        self.y /= rhs;
        self.z /= rhs;
    }
}

#[derive(Debug, Clone)]
pub struct Particle {
    pub p: F64x3, // Position
    pub v: F64x3, // Velocity
    pub r: f64,   // Radius
    pub m: f64,   // Mass
}

impl Particle {
    pub fn new(p: F64x3, v: F64x3, r: f64, m: f64) -> Self {
        Self { p, v, r, m }
    }
}

pub fn circular_orbits(n: usize) -> Vec<Particle> {
    let mut particles = vec![Particle::new(
        F64x3::new(0.0, 0.0, 0.0),
        F64x3::new(0.0, 0.0, 0.0),
        0.00465047,
        1.0,
    )];

    let mut rng = rand::thread_rng();

    for i in 0..n {
        let d = 0.1 + (i as f64 * 5.0 / n as f64);
        let v = (1.0 / d).sqrt();
        let theta = rng.r#gen::<f64>() * 6.28;

        let x = d * theta.cos();
        let y = d * theta.sin();
        let vx = -v * theta.sin();
        let vy = v * theta.cos();

        particles.push(Particle::new(
            F64x3::new(x, y, 0.0),
            F64x3::new(vx, vy, 0.0),
            1e-14,
            1e-7,
        ));
    }
    particles
}

pub fn calc_pp_accel(pi: &Particle, pj: &Particle) -> F64x3 {
    let dp = pi.p - pj.p;
    let dp2 = dp.dot(dp);
    let dist = dp2.sqrt();
    let magi = -pj.m / (dist * dist * dist);
    dp * magi
}

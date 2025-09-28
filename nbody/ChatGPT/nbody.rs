use std::env;
use std::f64::consts::PI;

const SOLAR_MASS: f64 = 4.0 * PI * PI;
const DAYS_PER_YEAR: f64 = 365.24;

#[derive(Clone)]
struct Body {
    position: [f64; 3],
    velocity: [f64; 3],
    mass: f64,
}

impl Body {
    fn offset_momentum(bodies: &mut [Body]) {
        let mut px = 0.0;
        let mut py = 0.0;
        let mut pz = 0.0;

        for body in bodies.iter() {
            px += body.velocity[0] * body.mass;
            py += body.velocity[1] * body.mass;
            pz += body.velocity[2] * body.mass;
        }

        bodies[0].velocity[0] = -px / SOLAR_MASS;
        bodies[0].velocity[1] = -py / SOLAR_MASS;
        bodies[0].velocity[2] = -pz / SOLAR_MASS;
    }

    fn advance(bodies: &mut [Body], dt: f64, steps: usize) {
        for _ in 0..steps {
            for i in 0..bodies.len() {
                for j in (i + 1)..bodies.len() {
                    let dx = bodies[i].position[0] - bodies[j].position[0];
                    let dy = bodies[i].position[1] - bodies[j].position[1];
                    let dz = bodies[i].position[2] - bodies[j].position[2];

                    let dist_sq = dx * dx + dy * dy + dz * dz;
                    let dist = dist_sq.sqrt();
                    let mag = dt / (dist_sq * dist);

                    let m1 = bodies[i].mass * mag;
                    let m2 = bodies[j].mass * mag;

                    bodies[i].velocity[0] -= dx * m2;
                    bodies[i].velocity[1] -= dy * m2;
                    bodies[i].velocity[2] -= dz * m2;

                    bodies[j].velocity[0] += dx * m1;
                    bodies[j].velocity[1] += dy * m1;
                    bodies[j].velocity[2] += dz * m1;
                }
            }

            for body in bodies.iter_mut() {
                body.position[0] += dt * body.velocity[0];
                body.position[1] += dt * body.velocity[1];
                body.position[2] += dt * body.velocity[2];
            }
        }
    }

    fn energy(bodies: &[Body]) -> f64 {
        let mut e = 0.0;
        for i in 0..bodies.len() {
            let v = &bodies[i].velocity;
            e += 0.5 * bodies[i].mass * (v[0] * v[0] + v[1] * v[1] + v[2] * v[2]);
            for j in (i + 1)..bodies.len() {
                let dx = bodies[i].position[0] - bodies[j].position[0];
                let dy = bodies[i].position[1] - bodies[j].position[1];
                let dz = bodies[i].position[2] - bodies[j].position[2];
                let distance = (dx * dx + dy * dy + dz * dz).sqrt();
                e -= (bodies[i].mass * bodies[j].mass) / distance;
            }
        }
        e
    }
}

fn create_solar_system() -> Vec<Body> {
    vec![
        Body {
            position: [0.0, 0.0, 0.0],
            velocity: [0.0, 0.0, 0.0],
            mass: SOLAR_MASS,
        },
        Body {
            position: [
                4.84143144246472090e+00,
                -1.16032004402742839e+00,
                -1.03622044471123109e-01,
            ],
            velocity: [
                1.66007664274403694e-03 * DAYS_PER_YEAR,
                7.69901118419740425e-03 * DAYS_PER_YEAR,
                -6.90460016972063023e-05 * DAYS_PER_YEAR,
            ],
            mass: 9.54791938424326609e-04 * SOLAR_MASS,
        },
        Body {
            position: [
                8.34336671824457987e+00,
                4.12479856412430479e+00,
                -4.03523417114321381e-01,
            ],
            velocity: [
                -2.76742510726862411e-03 * DAYS_PER_YEAR,
                4.99852801234917238e-03 * DAYS_PER_YEAR,
                2.30417297573763929e-05 * DAYS_PER_YEAR,
            ],
            mass: 2.85885980666130812e-04 * SOLAR_MASS,
        },
        Body {
            position: [
                1.28943695621391310e+01,
                -1.51111514016986312e+01,
                -2.23307578892655734e-01,
            ],
            velocity: [
                2.96460137564761618e-03 * DAYS_PER_YEAR,
                2.37847173959480950e-03 * DAYS_PER_YEAR,
                -2.96589568540237556e-05 * DAYS_PER_YEAR,
            ],
            mass: 4.36624404335156298e-05 * SOLAR_MASS,
        },
        Body {
            position: [
                1.53796971148509165e+01,
                -2.59193146099879641e+01,
                1.79258772950371181e-01,
            ],
            velocity: [
                2.68067772490389322e-03 * DAYS_PER_YEAR,
                1.62824170038242295e-03 * DAYS_PER_YEAR,
                -9.51592254519715870e-05 * DAYS_PER_YEAR,
            ],
            mass: 5.15138902046611451e-05 * SOLAR_MASS,
        },
    ]
}

fn main() {
    let args: Vec<String> = env::args().collect();
    let n = if args.len() > 1 {
        args[1].parse::<usize>().unwrap_or(1000)
    } else {
        1000
    };

    let mut bodies = create_solar_system();

    Body::offset_momentum(&mut bodies);

    println!("{:.9}", Body::energy(&bodies));
    Body::advance(&mut bodies, 0.01, n);
    println!("{:.9}", Body::energy(&bodies));
}

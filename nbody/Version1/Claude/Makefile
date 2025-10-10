c:
	gcc -O3 nbody-opt.c -lm

rust:
	cd *_rust; cargo build --release

run-c:
	./a.out 50000000

run-rust:
	./*_rust/target/release/nbody 50000000

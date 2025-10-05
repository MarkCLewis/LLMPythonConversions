c:
	gcc -O3 *.c -fopenmp -lm

rust:
	cd *_rust; cargo build --release

run-c:
	./a.out 5500

run-rust:
	./*_rust/target/release/*_rust 5500

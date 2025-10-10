c:
	gcc -O3 -fopenmp -march=native *.c -lm

rust:
	cd *_rust; cargo build --release

run-c:
	./a.out 16000

run-rust:
	./*_rust/target/release/*_rust 16000

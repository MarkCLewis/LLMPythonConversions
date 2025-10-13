c:
	gcc -O3 -march=native *.c -lfftw3 -lm -lfftw3_threads

rust:
	cd *_rust; cargo build --release

run-c:
	./a.out

run-rust:
	./*_rust/target/release/*_rust

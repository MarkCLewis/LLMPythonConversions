c:
	gcc -pipe -Wall -O3 -fomit-frame-pointer -march=native -fopenmp *.c -lm

rust:
	cd *_rust; cargo build --release

run-c:
	./a.out 5500

run-rust:
	./*_rust/target/release/*_rust 5500

run-python:
	python3 spectral-norm4.py 5500

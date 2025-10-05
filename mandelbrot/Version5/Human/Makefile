c:
	gcc -pipe -Wall -O3 -fomit-frame-pointer -march=native -mno-fma -fno-finite-math-only -fopenmp *.c -lm

rust:
	cd *_rust; cargo build --release

run-c:
	./a.out 16000

run-rust:
	./*_rust/target/release/*_rust 16000

run-python:
	python3 mandelbrot5.py 16000

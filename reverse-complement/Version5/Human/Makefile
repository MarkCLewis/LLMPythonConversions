c:
	gcc -pipe -Wall -O3 -fomit-frame-pointer -march=native -pthread *.c

rust:
	cd *_rust; cargo build --release

run-c:
	./a.out 0 < ../../revcomp-input.txt

run-rust:
	./*_rust/target/release/*_rust 0 < ../../revcomp-input.txt

run-python:
	python3 reverse-complement5.py 0 < ../../revcomp-input.txt

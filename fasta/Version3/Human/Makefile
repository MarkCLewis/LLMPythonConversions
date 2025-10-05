c:
	gcc -O3 *.c -lm

rust:
	cd *_rust; cargo build --release

run-c:
	./a.out 25000000

run-rust:
	./*_rust/target/release/*_rust 25000000

run-python:
	python3 fasta3.py 25000000

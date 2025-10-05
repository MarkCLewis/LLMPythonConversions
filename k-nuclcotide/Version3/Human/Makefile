c:
	gcc -pipe -Wall -O3 -fomit-frame-pointer -march=native -fopenmp *.c -lm

rust:
	cd *_rust; cargo build --release

run-c:
	./a.out 0 < ../../knucleotide-input25000000.txt

run-rust:
	./*_rust/target/release/*_rust 0 < ../../knucleotide-input25000000.txt

run-python:
	python3 knuclcotide3.py 0 < ../../knucleotide-input25000000.txt

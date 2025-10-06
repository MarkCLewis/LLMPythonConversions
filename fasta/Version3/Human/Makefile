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

generate-redux-input:
	./a.out 5000000 > ../../../regex-redux/regexredux-input5000000.txt

generate-k-nuclcotide-input:
	./a.out 25000000 > ../../../k-nuclcotide/knucleotide-input25000000.txt

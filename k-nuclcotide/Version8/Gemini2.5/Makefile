c:
	gcc -O3 *.c -lm

rust:
	cd *_rust; cargo build --release
run-c:
	./a.out 0 < ../../knucleotide-input25000000.txt

run-rust:
	./*_rust/target/release/k_nuclcotide8_rust 0 < ../../knucleotide-input25000000.txt

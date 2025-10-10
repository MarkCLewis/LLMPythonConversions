c:
	gcc -O3 *.c -lm

rust:
	cd k_nuclcotide3_rust; cargo build --release
run-c:
	./a.out 0 < ../../knucleotide-input25000000.txt

run-rust:
	./k_nuclcotide3_rust/target/release/k_nuclcotide3_rust 0 < ../../knucleotide-input25000000.txt

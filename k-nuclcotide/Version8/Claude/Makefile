#c:
#	gcc -O3 *.c -lm

rust:
	cd *_rust; cargo build --release
#run-c:
#	./a.out 0 < ../../knucleotide-input25000000.txt

run-rust:
	./*_rust/target/release/*_rust 0 < ../../knucleotide-input25000000.txt

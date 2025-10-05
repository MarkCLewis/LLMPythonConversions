c:
	gcc -O3 -march=native *.c -lpthread

rust:
	cd *_rust; cargo build --release

run-c:
	./a.out 0 < ../../revcomp-input.txt

run-rust:
	./*_rust/target/release/*_rust 0 < ../../revcomp-input.txt

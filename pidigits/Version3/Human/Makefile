c:
	gcc -pipe -Wall -O3 -fomit-frame-pointer -march=native *.c -lgmp

rust:
	cd *_rust; cargo build --release

run-c:
	./a.out 10000

run-rust:
	./*_rust/target/release/*_rust 10000

run-python:
	python3 pidigits3.py 10000

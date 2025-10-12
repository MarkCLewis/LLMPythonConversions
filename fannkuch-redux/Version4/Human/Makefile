c:
	gcc -pipe -Wall -O3 -fomit-frame-pointer -march=native -pthread -std=c11 *.c -pthread

rust:
	cd *_rust; cargo build --release

run-c:
	./a.out 12

run-rust:
	./*_rust/target/release/*_rust 12

run-python:
	python3 fannkuch-redux4.py 12

c:
	gcc -pipe -Wall -O3 -fomit-frame-pointer -march=native -fopenmp *.c -lpcre2-8

rust:
	cd *_rust; cargo build --release

run-c:
	./a.out 0 < ../../regexredux-input5000000.txt

run-rust:
	./*_rust/target/release/*_rust 0 < ../../regexredux-input5000000.txt

run-python:
	python3 regex-redux1.py 0 < ../../regexredux-input5000000.txt

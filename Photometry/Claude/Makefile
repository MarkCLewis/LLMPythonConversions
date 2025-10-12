c:
	gcc -O3 -march=native *.c -lfftw3 -lm -lfftw3_threads

rust:
	rustc -C opt-level=3 -C target-cpu=native -C codegen-units=1 nbody.rs

run-c:
	./a.out

run-rust:
	./nbody

run-python:
	python3 nbody.py 50000000

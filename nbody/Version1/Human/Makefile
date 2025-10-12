c:
	gcc -pipe -Wall -O3 -fomit-frame-pointer -march=native nbody.c -lm

rust:
	rustc -C opt-level=3 -C target-cpu=native -C codegen-units=1 nbody.rs

run-c:
	./a.out 50000000

run-rust:
	./nbody 50000000

run-python:
	python3 nbody3.py 50000000

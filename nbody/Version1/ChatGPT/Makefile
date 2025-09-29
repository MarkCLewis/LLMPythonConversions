c:	nbody.c
	gcc -O3 nbody.c -lm

rust:	nbody.rs
	rustc -C opt-level=3 -o a.out nbody.rs

run-c:
	./a.out 50000000

run-rust:
	./a.out 50000000

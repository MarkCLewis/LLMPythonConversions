c:
	gcc -O3 *.c -lm

rust:
	rustc -C opt-level=3 -C target-cpu=native -C codegen-units=1 *.rs

run-c:
	./a.out 21

run-rust:
	./binary-trees2 21

nbodyc:	nbody.c
	gcc -O3 -o nbodyc nbody.c -lm

nbodyr:	nbody.rs
	rustc -C opt-level=3 -o nbodyr nbody.rs

timec:
	time ./nbodyc 50000000

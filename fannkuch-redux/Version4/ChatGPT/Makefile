c:
	gcc -O3 -Wall -std=c11 *.c -pthread

rust:
	cd *-rust; cargo build --release

run-c:
	./a.out 12

run-rust:
	cd *-rust; ./target/release/fannkuch-rredux-rust 12

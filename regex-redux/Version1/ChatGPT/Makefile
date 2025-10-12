#c:
#	gcc -O3 *.c -lpcre2-8 -lpthread

rust:
	cd *_rust; cargo build --release

#run-c:
#	./a.out 0 < ../../regexredux-input5000000.txt

run-rust:
	./*_rust/target/release/*_rust 0 < ../../regexredux-input5000000.txt

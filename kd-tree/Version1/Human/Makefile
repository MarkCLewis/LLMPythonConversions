c:
	cd c-version; make

rust:
	cd *_rust; cargo build --release

run-c:
	./c-version/kdtree-sim 100000 10

run-rust:
	./*_rust/target/release/*_rust 100000 10

run-python:
	python3 Python/src/main.py 100000 10

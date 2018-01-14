.PHONY: build/debug build/release

CMAKE ?= /usr/bin/cmake

export CC=gcc
export CXX=g++

clean:
	rm -rf build;

build/debug:
	mkdir -p build/debug; cd build/debug; \
	$(CMAKE) ../.. -DCMAKE_BUILD_TYPE=Debug && make

build/release:
	mkdir -p build/release; cd build/release; \
	$(CMAKE) ../.. -DCMAKE_BUILD_TYPE=Release && make

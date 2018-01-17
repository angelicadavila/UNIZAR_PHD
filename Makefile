.PHONY: build/debug build/release

CMAKE ?= /usr/bin/cmake

export CC=gcc
export CXX=g++

DIR_DEBUG = build/debug
DIR_RELEASE = build/release

clean:
	rm -rf build;

dir/debug:
	mkdir -p $(DIR_DEBUG)

dir/release:
	mkdir -p $(DIR_RELEASE)

build/debug-clkernel: dir/debug
	cd $(DIR_DEBUG); \
	$(CMAKE) ../.. -DCMAKE_BUILD_TYPE=Debug -DCMAKE_EXPORT_COMPILE_COMMANDS=ON && make clkernel

build/debug: dir/debug
	cd $(DIR_DEBUG); \
	$(CMAKE) ../.. -DCMAKE_BUILD_TYPE=Debug -DCMAKE_EXPORT_COMPILE_COMMANDS=ON && make

build/release: dir/release
	cd $(DIR_RELEASE); \
	$(CMAKE) ../.. -DCMAKE_BUILD_TYPE=Release -DCMAKE_EXPORT_COMPILE_COMMANDS=ON && make






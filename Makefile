SHELL := /bin/bash

BUILD_TYPE ?= Debug
JOBS ?= $(shell nproc 2>/dev/null || echo 4)

.PHONY: all configure build debug release run test format lint clean re help

all: debug

configure:
	cmake -S . -B build -DCMAKE_BUILD_TYPE=$(BUILD_TYPE)

build: configure
	cmake --build build --parallel $(JOBS)

debug:
	$(MAKE) build BUILD_TYPE=Debug

release:
	$(MAKE) build BUILD_TYPE=Release

test: build
	ctest --test-dir build --output-on-failure

run: build
	./build/games/rtype/rtype

format:
	find engine games -type f \( -name '*.c' -o -name '*.h' -o -name '*.cpp' -o -name '*.hpp' \) -print0 | xargs -0 clang-format -i

lint: configure
	find engine games -type f \( -name '*.c' -o -name '*.cpp' \) -print0 | xargs -0 clang-tidy -p build

clean:
	rm -rf build
	rm -f CMakeUserPresets.json

re: clean all

help:
	@echo "make [debug|release|run|test|format|lint|clean|re]"

SHELL := /bin/bash

BUILD_TYPE ?= Debug
JOBS ?= $(shell nproc 2>/dev/null || echo 4)

CLIENT_BINARY := build/games/rtype/client/rtype_client
SERVER_BINARY := build/games/rtype/server/rtype_server
SIMULATION_BINARY := build/games/simulation/simulation
PERF_BUILD_DIR := build-perf
SIMULATION_PROFILE_BUILD_DIR := build-simulation-profile
PERF_FLAGS := -O3 -DNDEBUG -march=native -mtune=native -ffast-math -fno-semantic-interposition

.PHONY: all configure compile build debug release perf run run-server run-simulation run-simulation-perf test format lint clean re help

all: debug

configure:
	cmake -S . -B build -DCMAKE_BUILD_TYPE=$(BUILD_TYPE)

build: configure
	cmake --build build --parallel $(JOBS)

compile: build

debug:
	$(MAKE) build BUILD_TYPE=Debug

release:
	$(MAKE) build BUILD_TYPE=Release

perf:
	cmake -S . -B $(PERF_BUILD_DIR) \
		-DCMAKE_BUILD_TYPE=Release \
		-DCMAKE_INTERPROCEDURAL_OPTIMIZATION=ON \
		-DCMAKE_C_FLAGS_RELEASE="$(PERF_FLAGS)" \
		-DCMAKE_CXX_FLAGS_RELEASE="$(PERF_FLAGS)"
	cmake --build $(PERF_BUILD_DIR) --parallel $(JOBS)

test: build
	ctest --test-dir build --output-on-failure

run: build
	./$(CLIENT_BINARY)

run-server: build
	./$(SERVER_BINARY)

run-simulation:
	cmake -S . -B build \
		-DCMAKE_BUILD_TYPE=Release \
		-DCMAKE_INTERPROCEDURAL_OPTIMIZATION=ON \
		-DCMAKE_C_FLAGS_RELEASE="$(PERF_FLAGS)" \
		-DCMAKE_CXX_FLAGS_RELEASE="$(PERF_FLAGS)"
	cmake --build build --target simulation --parallel $(JOBS)
	./$(SIMULATION_BINARY)

run-simulation-perf:
	cmake -S . -B $(SIMULATION_PROFILE_BUILD_DIR) -DCMAKE_BUILD_TYPE=RelWithDebInfo
	cmake --build $(SIMULATION_PROFILE_BUILD_DIR) --target simulation --parallel $(JOBS)
	perf record -g --call-graph dwarf -- ./$(SIMULATION_PROFILE_BUILD_DIR)/games/simulation/simulation

format:
	find engine games -type f \( -name '*.c' -o -name '*.h' -o -name '*.cpp' -o -name '*.hpp' \) -print0 | xargs -0 clang-format -i

lint: configure
	find engine games -type f \( -name '*.c' -o -name '*.cpp' \) -print0 | xargs -0 clang-tidy -p build

clean:
	rm -rf build
	rm -rf $(PERF_BUILD_DIR)
	rm -rf $(SIMULATION_PROFILE_BUILD_DIR)
	rm -f CMakeUserPresets.json

re: clean all

help:
	@echo "make [debug|release|perf|run|run-server|run-simulation|run-simulation-perf|test|format|lint|clean|re|help]"

# RType Hello World Design

## Goal

Add the first C++23 game executable under `games/rtype` and prove that a game can consume
LECS through the repository build.

## Structure

- `games/rtype/CMakeLists.txt` defines the `rtype` executable and links `ecs::ecs`.
- `games/rtype/src/main.cpp` initializes LECS, registers one component, creates one entity,
  reads its component, prints a deterministic hello-world message, and finalizes LECS.
- The root `CMakeLists.txt` includes `games/rtype`.

Each future game can own the same small CMake boundary without adding a launcher or shared
game abstraction.

## Commands

The root Makefile provides `configure`, `build`, `debug`, `release`, `run`, `test`, `format`,
`lint`, `clean`, `re`, and `help`. Builds use the existing `build` directory and CMake build
type selected through `BUILD_TYPE`.

## Verification

CTest runs `rtype` as a smoke test and requires its deterministic output. Final verification
covers formatting, configuration, compilation, execution, tests, and clang-tidy. Logical line
counts use the same nonblank, non-comment-only method as the 7,658-line baseline.

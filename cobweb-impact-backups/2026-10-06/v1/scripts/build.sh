#!/usr/bin/env bash
set -euo pipefail
cd "$(dirname "$0")/.."
mkdir -p build
flags=(-std=c++17 -O2 -Wall -Wextra -Wpedantic -Werror -Iinclude)
core=(src/geometry.cpp src/fibers.cpp src/bake.cpp src/export.cpp)
"${CXX:-g++}" "${flags[@]}" src/impact_cli.cpp src/impact.cpp "${core[@]}" -o build/impact_demo
"${CXX:-g++}" "${flags[@]}" tests/impact_tests.cpp src/impact.cpp src/geometry.cpp src/fibers.cpp -o build/impact_tests
"${CXX:-g++}" "${flags[@]}" tests/impact_benchmark.cpp src/impact.cpp src/geometry.cpp src/fibers.cpp -o build/impact_benchmark

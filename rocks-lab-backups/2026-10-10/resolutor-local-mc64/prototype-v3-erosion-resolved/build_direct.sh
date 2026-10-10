#!/bin/bash
set -euo pipefail
cd "$(dirname "$0")"
mkdir -p build evidence
objects=()
for f in core/src/*.cpp; do
 o="build/$(basename "$f" .cpp).o"
 g++ -O2 -std=c++20 -Wall -Wextra -Wpedantic -Icore/include -c "$f" -o "$o"
 objects+=("$o")
done
ar rcs build/librocks_core.a "${objects[@]}"
g++ -O2 -std=c++20 -Icore/include cli/main.cpp build/librocks_core.a -o build/rocks_cli
for f in tests/*_tests.cpp; do
 name="$(basename "$f" .cpp)";g++ -O2 -std=c++20 -Icore/include "$f" build/librocks_core.a -o "build/$name"
done
g++ -O2 -std=c++20 tools/audit_intersections.cpp -o build/audit_intersections

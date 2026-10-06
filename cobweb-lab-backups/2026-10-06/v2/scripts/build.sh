#!/usr/bin/env bash
set -euo pipefail
cd "$(dirname "$0")/.."
mkdir -p build
"${CXX:-g++}" -std=c++17 -O2 -Wall -Wextra -Wpedantic -Werror -Iinclude src/*.cpp -o build/cobweb_lab
"${CXX:-g++}" -std=c++17 -O2 -Wall -Wextra -Wpedantic -Werror -Iinclude tests/core_tests.cpp src/{geometry,fibers,bake,export}.cpp -o build/core_tests

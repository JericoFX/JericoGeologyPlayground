#!/usr/bin/env bash
set -euo pipefail
cd "$(dirname "$0")/.."
mkdir -p build
"${CXX:-c++}" -std=c++17 -O2 -Wall -Wextra -Wpedantic -Werror -Iinclude -DFIXTURE_DIR='"tests/fixtures"' src/uv_clip.cpp tests/uv_clip_tests.cpp -o build/uv_clip_tests
./build/uv_clip_tests "$@"

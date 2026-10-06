#!/usr/bin/env bash
set -euo pipefail
cd "$(dirname "$0")/.."
mkdir -p build
"${CXX:-c++}" -std=c++17 -O1 -g -fno-omit-frame-pointer -fsanitize=address,undefined -Wall -Wextra -Wpedantic -Werror -Iinclude -DFIXTURE_DIR='"tests/fixtures"' src/uv_clip.cpp tests/uv_clip_tests.cpp -o build/uv_clip_tests_sanitized
ASAN_OPTIONS="${ASAN_OPTIONS:-detect_leaks=0}" ./build/uv_clip_tests_sanitized "$@"

#!/usr/bin/env bash
set -euo pipefail
cd "$(dirname "$0")/.."
lab_include="${1:-../JericoCobwebPhysicsLab/include}"
mkdir -p build
"${CXX:-c++}" -std=c++17 -O2 -Wall -Wextra -Wpedantic -Werror -I"$lab_include" -Iinclude src/uv_clip.cpp tests/integration_headers.cpp -o build/integration_headers
./build/integration_headers
printf 'PASS existing physics.hpp and uv_clip.hpp coexist with original cobweb.hpp\n'
